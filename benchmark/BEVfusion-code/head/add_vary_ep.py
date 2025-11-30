import onnx_graphsurgeon as gs
import onnx
import numpy as np
import copy

# --- Configuration ---
ONNX_MODEL_PATH = "optimized_model.optimized.onnx"
MODIFIED_ONNX_PATH = "model_very_advanced_moe.onnx"
NUM_EXPERTS = 4 # 我们将实现4种专家
EXPERT_TYPES = [0, 1, 2, 3] # 对应下面的四种类型

# --- Key Names (保持不变或根据需要调整) ---
HEAD_INPUT_TENSOR_NAME = "InsertedCast_1048"
# ... (其他来自您之前脚本的常量名称，如CONV1_WEIGHT_INPUT_TENSOR_NAME等)
CONV1_WEIGHT_INPUT_TENSOR_NAME = "InsertedCast_ConvBnFusion_W_1232"
CONV1_BIAS_INPUT_TENSOR_NAME = "InsertedCast_ConvBnFusion_BN_B_parent.heads.object.prediction_heads.0.heatmap.0.bn.bias"
CONV2_WEIGHT_INPUT_TENSOR_NAME = "InsertedCast_1234"
CONV2_BIAS_INPUT_TENSOR_NAME = "InsertedCast_1233"
ORIG_CONV1_NAME = "Conv_432"
ORIG_RELU1_NAME = "Relu_434"
ORIG_CONV2_NAME = "Conv_435"
ORIGINAL_HEAD_OUTPUT_TENSOR_NAME = "InsertedCast_1096"
NODE_AFTER_HEAD_OP_TYPE = "Sigmoid"
NODE_AFTER_HEAD_NAME = "Sigmoid_455"

# 假设所有专家输出统一的通道数和长度
# 这是关键假设，您需要根据 head_input_tensor 的实际情况来调整
# 假设 head_input_tensor 是 [Batch, Channels_In_Head, Seq_Len_Head] = [1, 256, 200]
CHANNELS_IN_HEAD = 256
SEQ_LEN_HEAD = 200
# 所有专家最终输出的通道数和序列长度/长度
EXPECTED_EXPERT_OUTPUT_CHANNELS = 64 # 示例值，需要您确定
EXPECTED_EXPERT_OUTPUT_LENGTH = SEQ_LEN_HEAD # 假设序列长度不变

# --- Helper Functions (get_actual_initializer_value, find_node_by_output_name - from previous) ---
def get_actual_initializer_value(graph, cast_output_tensor_name):
    tmap = graph.tensors()
    cast_output_tensor = tmap.get(cast_output_tensor_name)
    if not cast_output_tensor: raise ValueError(f"Tensor {cast_output_tensor_name} not found.")
    producer_node = next((node for node in graph.nodes if cast_output_tensor in node.outputs), None)
    if not producer_node:
        if isinstance(cast_output_tensor, gs.Constant): return cast_output_tensor.values, cast_output_tensor.dtype
        raise ValueError(f"Could not find producer node for {cast_output_tensor_name}")
    if producer_node.op != "Cast":
        if isinstance(cast_output_tensor, gs.Constant): return cast_output_tensor.values, cast_output_tensor.dtype
        raise ValueError(f"Producer of {cast_output_tensor_name} is {producer_node.op}, not 'Cast'.")
    original_initializer_tensor = producer_node.inputs[0]
    if not isinstance(original_initializer_tensor, gs.Constant):
        raise TypeError(f"Input to Cast for {cast_output_tensor_name} is not gs.Constant.")
    return original_initializer_tensor.values, original_initializer_tensor.dtype

def find_node_by_output_name(graph, tensor_name):
    return next((node for node in graph.nodes if any(out.name == tensor_name for out in node.outputs)), None)

# --- Expert Creation Functions ---

def _create_conv_block(graph, expert_id, type_id_str, block_num, input_tensor,
                       in_channels, out_channels, kernel_size, stride, padding,
                       conv_dtype=np.float32, add_relu=True):
    """Helper to create a Conv-(Cast)-ReLU block with random weights."""
    suffix = f"_expert{expert_id}_type{type_id_str}_block{block_num}"
    onnx_cast_to_type = onnx.TensorProto.FLOAT # Assuming conv weights are float32

    w_values = np.random.randn(out_channels, in_channels, kernel_size).astype(np.float32)
    b_values = np.random.randn(out_channels).astype(np.float32)
    conv_w = gs.Constant(name=f"ConvW{suffix}", values=w_values)
    conv_b = gs.Constant(name=f"ConvB{suffix}", values=b_values)

    # No Cast needed if weights are already float32 and conv_dtype is float32
    # casted_w = gs.Variable(name=f"CastedConvW{suffix}", dtype=conv_dtype)
    # cast_w_node = gs.Node("Cast", f"CastW{suffix}", inputs=[conv_w], outputs=[casted_w], attrs={"to": onnx_cast_to_type})
    # casted_b = gs.Variable(name=f"CastedConvB{suffix}", dtype=conv_dtype)
    # cast_b_node = gs.Node("Cast", f"CastB{suffix}", inputs=[conv_b], outputs=[casted_b], attrs={"to": onnx_cast_to_type})
    # graph.nodes.extend([cast_w_node, cast_b_node])
    # Using weights directly if they are already the correct type for Conv
    
    conv_out = gs.Variable(name=f"ConvOut{suffix}", dtype=conv_dtype)
    conv_node = gs.Node("Conv", f"Conv{suffix}",
                        inputs=[input_tensor, conv_w, conv_b], # Using new_conv_w, new_conv_b directly
                        outputs=[conv_out],
                        attrs={'kernel_shape': [kernel_size], 'strides': [stride],
                               'pads': [padding, padding], 'dilations': [1], 'group': 1}) # Assuming 1D Conv
    graph.nodes.append(conv_node)
    current_tensor = conv_out

    if add_relu:
        relu_out = gs.Variable(name=f"ReluOut{suffix}", dtype=conv_dtype)
        relu_node = gs.Node("Relu", f"Relu{suffix}", inputs=[current_tensor], outputs=[relu_out])
        graph.nodes.append(relu_node)
        current_tensor = relu_out
    return current_tensor


def create_expert_type0_original_like(graph, expert_id, expert_input_tensor):
    """Expert Type 0: Similar to the original Conv->ReLU->Conv structure."""
    print(f"Creating Expert Type 0 (Original Like, ID: {expert_id})")
    # This function is largely the same as your previous create_expert_type0_original_like
    # Ensure its final output matches EXPECTED_EXPERT_OUTPUT_CHANNELS and EXPECTED_EXPERT_OUTPUT_LENGTH
    # For brevity, I'll reuse the structure but emphasize the output channel adjustment.
    suffix = f"_expert{expert_id}_type0"
    dtype = np.float32
    cast_to_type = np.float32
    onnx_cast_to_type = onnx.TensorProto.FLOAT
    conv_dtype = cast_to_type

    # --- Conv1 (Mimic original Conv_432) ---
    conv1_w_values, _ = get_actual_initializer_value(graph, CONV1_WEIGHT_INPUT_TENSOR_NAME)
    conv1_b_values, _ = get_actual_initializer_value(graph, CONV1_BIAS_INPUT_TENSOR_NAME)
    # conv1_out_channels = conv1_w_values.shape[0] # Original output channels of Conv_432

    new_conv1_w = gs.Constant(name=f"Expert{expert_id}_T0_Conv1_W", values=np.copy(conv1_w_values))
    new_conv1_b = gs.Constant(name=f"Expert{expert_id}_T0_Conv1_B", values=np.copy(conv1_b_values))
    # No Cast for weights if they are already float32 (assuming conv_dtype is float32)

    exp_conv1_out = gs.Variable(name=f"Expert{expert_id}_T0_Conv1_Out", dtype=conv_dtype)
    exp_conv1_node = gs.Node(op="Conv", name=f"Expert{expert_id}_T0_Conv1",
                           inputs=[expert_input_tensor, new_conv1_w, new_conv1_b],
                           outputs=[exp_conv1_out])
    orig_conv1_node = next((n for n in graph.nodes if n.name == ORIG_CONV1_NAME), None)
    if orig_conv1_node: exp_conv1_node.attrs = copy.deepcopy(orig_conv1_node.attrs)
    else: exp_conv1_node.attrs = {'auto_pad': 'NOTSET', 'dilations': [1], 'group': 1, 'pads': [0, 0], 'strides': [1]} # Default for 1x1 Conv

    # --- Relu1 ---
    exp_relu1_out = gs.Variable(name=f"Expert{expert_id}_T0_Relu1_Out", dtype=conv_dtype)
    exp_relu1_node = gs.Node(op="Relu", name=f"Expert{expert_id}_T0_Relu1", inputs=[exp_conv1_out], outputs=[exp_relu1_out])

    # --- Conv2 (Adjusted to output EXPECTED_EXPERT_OUTPUT_CHANNELS) ---
    # Input channels to this Conv2 is conv1_out_channels (e.g. 256 if Conv_432 outputs 256)
    c_relu_out = conv1_w_values.shape[0]
    kernel_shape_conv2 = [1] # As per original Conv_435

    conv2_adj_w_values = np.random.randn(EXPECTED_EXPERT_OUTPUT_CHANNELS, c_relu_out, kernel_shape_conv2[0]).astype(np.float32)
    conv2_adj_b_values = np.random.randn(EXPECTED_EXPERT_OUTPUT_CHANNELS).astype(np.float32)
    new_conv2_adj_w = gs.Constant(name=f"Expert{expert_id}_T0_Conv2adj_W", values=conv2_adj_w_values)
    new_conv2_adj_b = gs.Constant(name=f"Expert{expert_id}_T0_Conv2adj_B", values=conv2_adj_b_values)

    exp_conv2_adj_out = gs.Variable(name=f"Expert{expert_id}_T0_FinalOut", dtype=conv_dtype) # Final output
    exp_conv2_adj_node = gs.Node(op="Conv", name=f"Expert{expert_id}_T0_Conv2adj",
                               inputs=[exp_relu1_out, new_conv2_adj_w, new_conv2_adj_b],
                               outputs=[exp_conv2_adj_out])
    orig_conv2_node = next((n for n in graph.nodes if n.name == ORIG_CONV2_NAME), None)
    if orig_conv2_node:
        exp_conv2_adj_node.attrs = copy.deepcopy(orig_conv2_node.attrs)
        exp_conv2_adj_node.attrs['kernel_shape'] = kernel_shape_conv2
    else:
        exp_conv2_adj_node.attrs = {'auto_pad': 'NOTSET', 'dilations': [1], 'group': 1, 'kernel_shape': kernel_shape_conv2, 'pads': [0, 0], 'strides': [1]}

    graph.nodes.extend([exp_conv1_node, exp_relu1_node, exp_conv2_adj_node])
    return exp_conv2_adj_out


def create_expert_type1_mlp(graph, expert_id, expert_input_tensor):
    """Expert Type 1: MLP (Multi-Layer Perceptron) Expert.
    Input: [Batch, C_in, L]. Output: [Batch, EXPECTED_EXPERT_OUTPUT_CHANNELS, L]
    MLP typically operates on flattened features per token/position.
    We can apply a 1x1 Conv (equivalent to FC per token) or Reshape -> MatMul -> Reshape.
    Let's use 1x1 Convs for simplicity to maintain the [B,C,L] structure.
    """
    print(f"Creating Expert Type 1 (MLP, ID: {expert_id})")
    suffix = f"_expert{expert_id}_type1_mlp"
    conv_dtype = np.float32 # Assuming float32 for MLP operations

    # Assume expert_input_tensor has shape [N, CHANNELS_IN_HEAD, SEQ_LEN_HEAD]
    # MLP Layer 1 (1x1 Conv)
    hidden_mlp_channels = 128 # Example hidden size
    mlp_conv1_out = _create_conv_block(graph, expert_id, "1_mlp", 1, expert_input_tensor,
                                     CHANNELS_IN_HEAD, hidden_mlp_channels,
                                     kernel_size=1, stride=1, padding=0,
                                     conv_dtype=conv_dtype, add_relu=True)

    # MLP Layer 2 (1x1 Conv to map to expected output channels)
    mlp_final_out = _create_conv_block(graph, expert_id, "1_mlp", 2, mlp_conv1_out,
                                     hidden_mlp_channels, EXPECTED_EXPERT_OUTPUT_CHANNELS,
                                     kernel_size=1, stride=1, padding=0,
                                     conv_dtype=conv_dtype, add_relu=False) # No ReLU on final expert output typically
    return mlp_final_out


def create_expert_type2_self_attention(graph, expert_id, expert_input_tensor):
    """Expert Type 2: Simplified Self-Attention Expert.
    Input: [Batch, C_in, L]. Output: [Batch, EXPECTED_EXPERT_OUTPUT_CHANNELS, L]
    A very basic single-head self-attention. Does not include LayerNorm or FFN for simplicity here.
    """
    print(f"Creating Expert Type 2 (Self-Attention, ID: {expert_id})")
    suffix = f"_expert{expert_id}_type2_sa"
    dtype = np.float32 # Assuming float32
    dtype_np = np.float32

    # Input shape: [Batch, Channels, SeqLen] e.g. [N, C_in=256, L=200]
    # We need to project C_in to an embedding dimension for Q, K, V.
    # Let d_k (key/query dim) and d_v (value dim) be, for example, 64.
    # For single head attention, d_model (output dim of attention) could be d_v.
    # The input to MatMul for Q,K,V should be [Tokens, Features_in] or [Batch*Tokens, Features_in]
    # Original input is [N, C, L]. We can permute to [N, L, C] for easier token-wise FC.
    # Or use 1x1 Convs to project, keeping [N,C,L] structure. Let's use 1x1 Convs.

    d_k = 64 # Dimension of K and Q
    d_v = 64 # Dimension of V
    
    # --- Q, K, V projections using 1x1 Convolutions ---
    # Input: expert_input_tensor [N, CHANNELS_IN_HEAD, SEQ_LEN_HEAD]
    # Q_proj: W_q [d_k, CHANNELS_IN_HEAD, 1], B_q [d_k]
    # K_proj: W_k [d_k, CHANNELS_IN_HEAD, 1], B_k [d_k]
    # V_proj: W_v [d_v, CHANNELS_IN_HEAD, 1], B_v [d_v]
    # Output of these convs will be [N, d_k, L], [N, d_k, L], [N, d_v, L]

    q_tensor = _create_conv_block(graph, expert_id, "2_sa_Q", 0, expert_input_tensor, CHANNELS_IN_HEAD, d_k, 1, 1, 0, dtype, add_relu=False)
    k_tensor = _create_conv_block(graph, expert_id, "2_sa_K", 1, expert_input_tensor, CHANNELS_IN_HEAD, d_k, 1, 1, 0, dtype, add_relu=False)
    v_tensor = _create_conv_block(graph, expert_id, "2_sa_V", 2, expert_input_tensor, CHANNELS_IN_HEAD, d_v, 1, 1, 0, dtype, add_relu=False)

    # --- Scaled Dot-Product Attention ---
    # Attention(Q,K,V) = softmax( (Q @ K.T) / sqrt(d_k) ) @ V
    # Q: [N, d_k, L]
    # K: [N, d_k, L] -> K.T (transpose last two dims): [N, L, d_k]
    # Q @ K.T : [N, d_k, L] @ [N, L, d_k] -> [N, d_k, d_k] ? No, this is wrong for sequence attention.
    # We want attention scores for each token in L with every other token in L.
    # Q should be [N, L, d_k], K should be [N, L, d_k], V should be [N, L, d_v]
    # This requires transposing the output of 1x1 Conv or the input.
    # Let's transpose output of 1x1 Conv: [N, d_k, L] -> [N, L, d_k]
    
    perm_NLC = [0, 2, 1] # Transpose from [N,C,L] to [N,L,C]
    q_transposed = gs.Variable(f"Q_transposed{suffix}", dtype=dtype)
    k_transposed = gs.Variable(f"K_transposed{suffix}", dtype=dtype)
    v_transposed = gs.Variable(f"V_transposed{suffix}", dtype=dtype)
    graph.nodes.append(gs.Node("Transpose", f"Transpose_Q{suffix}", inputs=[q_tensor], outputs=[q_transposed], attrs={"perm": perm_NLC}))
    graph.nodes.append(gs.Node("Transpose", f"Transpose_K{suffix}", inputs=[k_tensor], outputs=[k_transposed], attrs={"perm": perm_NLC}))
    graph.nodes.append(gs.Node("Transpose", f"Transpose_V{suffix}", inputs=[v_tensor], outputs=[v_transposed], attrs={"perm": perm_NLC}))
    # Now Q_transposed, K_transposed are [N, L, d_k], V_transposed is [N, L, d_v]

    # MatMul: Q_transposed [N,L,d_k] @ K_transposed.T [N,d_k,L] -> scores [N,L,L]
    # K_T_for_matmul needs K_transposed to be permuted as [N, d_k, L] for matmul with Q[N,L,d_k]
    # effectively K_tensor itself for this permutation.
    scores_unscaled = gs.Variable(f"ScoresUnscaled{suffix}", dtype=dtype)
    graph.nodes.append(gs.Node("MatMul", f"MatMul_QK{suffix}", inputs=[q_transposed, k_tensor], outputs=[scores_unscaled])) # Q[N,L,d_k] @ K[N,d_k,L] -> [N,L,L]

    # Scale scores
    scale_factor = np.sqrt(d_k).astype(dtype_np)
    scale_const = gs.Constant(f"ScaleFactor{suffix}", np.array(scale_factor))
    scaled_scores = gs.Variable(f"ScoresScaled{suffix}", dtype=dtype)
    graph.nodes.append(gs.Node("Div", f"ScaleScores{suffix}", inputs=[scores_unscaled, scale_const], outputs=[scaled_scores]))

    # Softmax over scores
    attention_probs = gs.Variable(f"AttentionProbs{suffix}", dtype=dtype)
    graph.nodes.append(gs.Node("Softmax", f"SoftmaxScores{suffix}", inputs=[scaled_scores], outputs=[attention_probs], attrs={"axis": -1})) # Softmax over last dim (L_keys)

    # MatMul: attention_probs [N,L,L] @ V_transposed [N,L,d_v] -> attention_output [N,L,d_v]
    attention_output_NLC = gs.Variable(f"AttentionOutput_NLC{suffix}", dtype=dtype)
    graph.nodes.append(gs.Node("MatMul", f"MatMul_AttnV{suffix}", inputs=[attention_probs, v_transposed], outputs=[attention_output_NLC]))
    # attention_output_NLC is [N, L, d_v]

    # Transpose back to [N, d_v, L]
    perm_NCL = [0, 2, 1]
    attention_output_NCL = gs.Variable(f"AttentionOutput_NCL{suffix}", dtype=dtype) # This is [N, d_v, L]
    graph.nodes.append(gs.Node("Transpose", f"Transpose_AttnOut{suffix}", inputs=[attention_output_NLC], outputs=[attention_output_NCL], attrs={"perm": perm_NCL}))

    # Final projection to EXPECTED_EXPERT_OUTPUT_CHANNELS if d_v is different
    # Using a 1x1 Conv. Input to this conv is attention_output_NCL [N, d_v, L]
    if d_v != EXPECTED_EXPERT_OUTPUT_CHANNELS:
        final_out = _create_conv_block(graph, expert_id, "2_sa_Proj", 3, attention_output_NCL,
                                       d_v, EXPECTED_EXPERT_OUTPUT_CHANNELS,
                                       kernel_size=1, stride=1, padding=0,
                                       conv_dtype=dtype, add_relu=False)
    else:
        final_out = attention_output_NCL # No projection needed if d_v matches

    return final_out


def create_expert_type3_depthwise_separable_conv(graph, expert_id, expert_input_tensor):
    """Expert Type 3: Depthwise Separable Convolution Expert.
    Input: [Batch, C_in, L]. Output: [Batch, EXPECTED_EXPERT_OUTPUT_CHANNELS, L]
    Consists of a Depthwise Conv, then a Pointwise Conv (1x1).
    """
    print(f"Creating Expert Type 3 (Depthwise Separable, ID: {expert_id})")
    suffix = f"_expert{expert_id}_type3_dwsep"
    dtype = np.float32 # Assuming float32
    onnx_dtype = onnx.TensorProto.FLOAT

    # Assuming expert_input_tensor is [N, CHANNELS_IN_HEAD, SEQ_LEN_HEAD]
    # CHANNELS_IN_HEAD = 256

    # --- Depthwise Convolution ---
    # groups = CHANNELS_IN_HEAD
    # output_channels_dw = CHANNELS_IN_HEAD (for standard depthwise, multiplier=1)
    # kernel_size_dw, e.g., 3 for 1D
    kernel_size_dw = 3
    padding_dw = 1 # for 'SAME' with kernel 3, stride 1

    dw_conv_w_values = np.random.randn(CHANNELS_IN_HEAD, 1, kernel_size_dw).astype(dtype) # Shape: [OutChannels=C_in, InChannelsPerGroup=1, Kernel]
    dw_conv_b_values = np.random.randn(CHANNELS_IN_HEAD).astype(dtype)
    dw_conv_w = gs.Constant(name=f"DWConvW{suffix}", values=dw_conv_w_values)
    dw_conv_b = gs.Constant(name=f"DWConvB{suffix}", values=dw_conv_b_values)

    dw_conv_out = gs.Variable(name=f"DWConvOut{suffix}", dtype=dtype)
    dw_conv_node = gs.Node("Conv", f"DWConv{suffix}",
                           inputs=[expert_input_tensor, dw_conv_w, dw_conv_b],
                           outputs=[dw_conv_out],
                           attrs={'kernel_shape': [kernel_size_dw], 'strides': [1],
                                  'pads': [padding_dw, padding_dw], 'dilations': [1],
                                  'group': CHANNELS_IN_HEAD}) # Key for depthwise
    graph.nodes.append(dw_conv_node)
    
    # Optional: ReLU after Depthwise
    dw_relu_out = gs.Variable(name=f"DWReluOut{suffix}", dtype=dtype)
    dw_relu_node = gs.Node("Relu", f"DWRelu{suffix}", inputs=[dw_conv_out], outputs=[dw_relu_out])
    graph.nodes.append(dw_relu_node)

    # --- Pointwise Convolution (1x1 Conv) ---
    # Input to pointwise is dw_relu_out [N, CHANNELS_IN_HEAD, L_after_dw]
    # Output channels: EXPECTED_EXPERT_OUTPUT_CHANNELS
    # Kernel size is 1
    pw_conv_w_values = np.random.randn(EXPECTED_EXPERT_OUTPUT_CHANNELS, CHANNELS_IN_HEAD, 1).astype(dtype)
    pw_conv_b_values = np.random.randn(EXPECTED_EXPERT_OUTPUT_CHANNELS).astype(dtype)
    pw_conv_w = gs.Constant(name=f"PWConvW{suffix}", values=pw_conv_w_values)
    pw_conv_b = gs.Constant(name=f"PWConvB{suffix}", values=pw_conv_b_values)

    pw_conv_out = gs.Variable(name=f"PWConvOut{suffix}", dtype=dtype) # Final output
    pw_conv_node = gs.Node("Conv", f"PWConv{suffix}",
                           inputs=[dw_relu_out, pw_conv_w, pw_conv_b],
                           outputs=[pw_conv_out],
                           attrs={'kernel_shape': [1], 'strides': [1],
                                  'pads': [0,0], 'dilations': [1], 'group': 1})
    graph.nodes.append(pw_conv_node)
    
    # Optional: ReLU after Pointwise if this isn't the final expert output layer
    # For now, let pw_conv_out be the output of this expert.

    return pw_conv_out


# --- Gating Network (copied and adapted from previous response) ---
def create_gating_network(graph, gating_input_tensor, num_experts_to_gate):
    print("Creating Gating Network...")
    gating_input_dtype = gating_input_tensor.dtype if gating_input_tensor.dtype is not None else np.float32
    dtype_np = np.float32 # Gating network will use float32

    # Assuming input is [N,C,L] like [1,256,200] (Batch, Channels, SequenceLength)
    rm_axes_val = np.array([2], dtype=np.int64)
    if gating_input_tensor.shape and len(gating_input_tensor.shape) == 4:
        rm_axes_val = np.array([2,3], dtype=np.int64)

    rm_axes = gs.Constant(name="Gating_ReduceMean_Axes", values=rm_axes_val)
    pooled_features = gs.Variable(name="Gating_Pooled_Features", dtype=gating_input_dtype)
    rm_node = gs.Node(op="ReduceMean", name="Gating_ReduceMean",
                      inputs=[gating_input_tensor, rm_axes],
                      outputs=[pooled_features], attrs={"keepdims": 0})

    # FC Layer
    c_in_fc = CHANNELS_IN_HEAD # Assuming this matches the input features to the head
    # If gating_input_tensor.shape is reliable:
    # if gating_input_tensor.shape and len(gating_input_tensor.shape) > 1:
    #    c_in_fc = gating_input_tensor.shape[1] # Channels dim

    fc_w_values = np.random.randn(c_in_fc, num_experts_to_gate).astype(dtype_np)
    fc_b_values = np.random.randn(num_experts_to_gate).astype(dtype_np)
    fc_w = gs.Constant(name="Gating_FC_W", values=fc_w_values)
    fc_b = gs.Constant(name="Gating_FC_B", values=fc_b_values)

    # Cast pooled_features to float32 if it's not already, for MatMul
    if pooled_features.dtype != dtype_np:
        casted_pooled_features = gs.Variable(name="Gating_Pooled_Casted", dtype=dtype_np)
        graph.nodes.append(gs.Node("Cast", "Cast_Gating_Pooled", inputs=[pooled_features], outputs=[casted_pooled_features], attrs={"to": onnx.TensorProto.FLOAT}))
        pooled_features_for_fc = casted_pooled_features
    else:
        pooled_features_for_fc = pooled_features


    matmul_output = gs.Variable(name="Gating_MatMul_Output", dtype=dtype_np)
    matmul_node = gs.Node(op="MatMul", name="Gating_MatMul", inputs=[pooled_features_for_fc, fc_w], outputs=[matmul_output])

    logits = gs.Variable(name="Gating_Logits", dtype=dtype_np)
    add_node = gs.Node(op="Add", name="Gating_Add_Bias", inputs=[matmul_output, fc_b], outputs=[logits])

    probs = gs.Variable(name="Gating_Probabilities", dtype=dtype_np)
    softmax_node = gs.Node(op="Softmax", name="Gating_Softmax", inputs=[logits], outputs=[probs], attrs={"axis": -1})

    graph.nodes.extend([rm_node, matmul_node, add_node, softmax_node])
    # We added cast node conditionally, so ensure it's part of the extend if created
    if pooled_features.dtype != dtype_np and 'cast_gating_pooled' in locals() : # check if cast_gating_pooled was created
         if cast_gating_pooled.outputs[0] not in [n.outputs[0] for n in graph.nodes if n.op == "Cast" and n.name == "Cast_Gating_Pooled"]: # avoid duplicate add
            graph.nodes.append(next(n for n in graph.nodes if n.name == "Cast_Gating_Pooled"))


    return probs


@gs.Graph.register()
def modify_for_all_expert_types_moe(self, expert_type_id_list): # Renamed for clarity
    num_total_experts = len(expert_type_id_list)
    print(f"Starting MoE with {num_total_experts} specified expert types...")

    # --- Get head input tensor and ensure dtype ---
    head_input_tensor = self.tensors().get(HEAD_INPUT_TENSOR_NAME)
    if not head_input_tensor: raise ValueError(f"Missing head input: {HEAD_INPUT_TENSOR_NAME}")
    if head_input_tensor.dtype is None:
        print(f"[Warning] Head input tensor {head_input_tensor.name} dtype None. Assuming float32.")
        head_input_tensor.dtype = np.float32
    # Ensure head_input_tensor is float32 for expert inputs (many experts assume this)
    if head_input_tensor.dtype != np.float32:
        print(f"Casting head_input_tensor {head_input_tensor.name} from {head_input_tensor.dtype} to float32")
        casted_head_input = gs.Variable(name=head_input_tensor.name + "_to_float32", dtype=np.float32)
        self.nodes.append(gs.Node("Cast", f"Cast_{head_input_tensor.name}_to_fp32",
                                  inputs=[head_input_tensor], outputs=[casted_head_input],
                                  attrs={"to": onnx.TensorProto.FLOAT}))
        head_input_for_experts = casted_head_input
    else:
        head_input_for_experts = head_input_tensor
    print(f"Head input for experts: {head_input_for_experts.name} (dtype: {head_input_for_experts.dtype}, shape: {head_input_for_experts.shape})")


    # --- Find original output and consuming node ---
    original_head_output_tensor = self.tensors().get(ORIGINAL_HEAD_OUTPUT_TENSOR_NAME)
    if not original_head_output_tensor: raise ValueError(f"Missing original head output: {ORIGINAL_HEAD_OUTPUT_TENSOR_NAME}")
    
    node_after_head = None
    for node in list(self.nodes): # Iterate over a copy if modifying the list
        if original_head_output_tensor in node.inputs:
            if node.name == NODE_AFTER_HEAD_NAME and node.op == NODE_AFTER_HEAD_OP_TYPE:
                node_after_head = node
                break
            elif node_after_head is None: # Fallback to first consumer
                node_after_head = node
    
    if not node_after_head:
        if original_head_output_tensor in self.outputs:
            raise ValueError(f"Tensor {ORIGINAL_HEAD_OUTPUT_TENSOR_NAME} is a graph output. This EP logic needs adjustment.")
        raise ValueError(f"No node found consuming {ORIGINAL_HEAD_OUTPUT_TENSOR_NAME}")
    
    original_input_idx = -1
    for idx, inp in enumerate(node_after_head.inputs):
        if inp.name == ORIGINAL_HEAD_OUTPUT_TENSOR_NAME:
            original_input_idx = idx
            break
    if original_input_idx == -1:
        raise ValueError(f"Could not find {ORIGINAL_HEAD_OUTPUT_TENSOR_NAME} in inputs of {node_after_head.name}")
    print(f"Node after head: {node_after_head.name}, op: {node_after_head.op}, input_idx: {original_input_idx}")


    # 1. Create Gating Network
    # Pass head_input_for_experts (which is ensured to be float32)
    gating_probs_tensor = create_gating_network(self, head_input_for_experts, num_total_experts)
    print(f"Gating probs tensor: {gating_probs_tensor.name} (dtype: {gating_probs_tensor.dtype}, shape: {gating_probs_tensor.shape})")


    # 2. Create Heterogeneous Experts
    expert_output_tensors = []
    for i, expert_type_id in enumerate(expert_type_id_list):
        print(f"  Creating expert {i} of Type {expert_type_id}...")
        if expert_type_id == 0:
            expert_out = create_expert_type0_original_like(self, i, head_input_for_experts)
        elif expert_type_id == 1:
            expert_out = create_expert_type1_mlp(self, i, head_input_for_experts)
        elif expert_type_id == 2:
            expert_out = create_expert_type2_self_attention(self, i, head_input_for_experts)
        elif expert_type_id == 3:
            expert_out = create_expert_type3_depthwise_separable_conv(self, i, head_input_for_experts)
        else:
            raise ValueError(f"Unknown expert_type_id: {expert_type_id}")
        expert_output_tensors.append(expert_out)
        print(f"  Expert {i} (Type {expert_type_id}) created, output: {expert_out.name} (dtype: {expert_out.dtype}, shape: {expert_out.shape})")


    # 3. Combine Expert Outputs (Weighted Sum)
    combine_dtype = np.float32 # Forcing combination in float32
    weighted_expert_outputs_final = []

    # Check if all experts successfully produced an output tensor
    if len(expert_output_tensors) != num_total_experts:
        raise RuntimeError("Mismatch between requested experts and created expert outputs.")

    for i in range(num_total_experts):
        expert_out_i = expert_output_tensors[i]
        if expert_out_i.dtype != combine_dtype:
            print(f"Casting expert {i} output {expert_out_i.name} from {expert_out_i.dtype} to {combine_dtype}")
            casted_expert_out_i = gs.Variable(name=expert_out_i.name + "_tocombine", dtype=combine_dtype)
            self.nodes.append(gs.Node("Cast", f"CastToExpCombine_Exp{i}", inputs=[expert_out_i], outputs=[casted_expert_out_i],
                                      attrs={"to": onnx.helper.np_dtype_to_tensor_dtype(combine_dtype)}))
            expert_out_i_for_weighting = casted_expert_out_i
        else:
            expert_out_i_for_weighting = expert_out_i
        
        # Slice gating_probs_tensor to get prob for current expert i: shape [B, 1]
        slice_starts_val = np.array([0, i], dtype=np.int64) # Slice BATCH_DIM_idx=0, EXPERT_DIM_idx=1
        slice_ends_val = np.array([1, i + 1], dtype=np.int64) # Assumes batch_size 1 for ends. Use large number for general batch.
                                                              # For dynamic batch: use large like np.iinfo(np.int64).max for batch dim
        slice_axes_val = np.array([0, 1], dtype=np.int64)
        
        slice_starts = gs.Constant(f"GatingSlice_starts_exp{i}", slice_starts_val)
        slice_ends = gs.Constant(f"GatingSlice_ends_exp{i}", np.array([np.iinfo(np.int64).max, i + 1], dtype=np.int64)) # Fix for dynamic batch
        slice_axes = gs.Constant(f"GatingSlice_axes_exp{i}", slice_axes_val)

        gating_prob_i = gs.Variable(f"GatingProb_exp{i}", dtype=gating_probs_tensor.dtype) # Should be float32
        self.nodes.append(gs.Node("Slice", f"GatingSlice_exp{i}",
                             inputs=[gating_probs_tensor, slice_starts, slice_ends, slice_axes],
                             outputs=[gating_prob_i]))

        # Unsqueeze gating_prob_i [B,1] to be broadcastable with expert_out_i [B, C_exp, L_exp]
        # Target shape for prob: [B,1,1] (if expert_out_i is rank 3 [B,C,L])
        # This will scale the whole [C,L] feature map for each batch item by the expert's weight.
        # The number of dimensions to add in unsqueeze_axes_val depends on the rank of expert_out_i.
        # If expert_out_i has rank R, and gating_prob_i is [B,1] (rank 2 after slice),
        # we need to add R-2 ones. Example: R=3 -> add 1 'one', R=4 -> add 2 'ones'.
        # The axes for unsqueezing would be [2], [2,3], etc.
        # For rank 3 expert output [B,C,L], and gating_prob_i [B,1], want [B,1,1]
        # Unsqueeze axes should be [1, 2] if gating_prob_i was just [B] (squeezed).
        # If gating_prob_i is [B,1], and expert_out is [B,C,L], we need to unsqueeze gating_prob_i at new axes for C and L.
        # Let's assume expert output rank is 3 ([B, C_exp, L_exp]). gating_prob_i has shape [B, 1].
        # Unsqueeze to [B, 1, 1]. Axes to add new dims: the new second and third dimension (indices 1 and 2 if starting from 0)
        # This is tricky. A simpler way:
        # If expert_out is [B,C,L], prob is [B,1]. Unsqueeze prob at axis 2 -> [B,1,1].
        unsqueeze_axes_val_for_bcl = np.array([2], dtype=np.int64) # For [B,1] -> [B,1,1] for [B,C,L]
        # If expert_out is [B,C,H,W], prob is [B,1]. Unsqueeze to [B,1,1,1]. Axes [2,3].
        # We need to know rank of expert_out_i. For now, assume rank 3.
        # It's often more robust to get the shape and build the unsqueeze axes dynamically.
        # But gs tensor.shape might be None.

        gating_prob_i_expanded = gs.Variable(f"GatingProbExpanded_exp{i}", dtype=gating_probs_tensor.dtype)
        # Assuming expert output is rank 3: [Batch, Channels, SeqLen]
        # gating_prob_i from slice is [Batch, 1]
        # To make it [Batch, 1, 1] for multiplying with [Batch, Channels, SeqLen] (element-wise over C, L)
        # we need to add one dimension at the end.
        axes_to_add = gs.Constant(f"UnsqueezeAxes_exp{i}", np.array([2], dtype=np.int64)) # Add a new axis at the end
        self.nodes.append(gs.Node("Unsqueeze", f"GatingUnsqueeze_exp{i}",
                                 inputs=[gating_prob_i, axes_to_add], # Input [B,1], Axes [2] -> Output [B,1,1]
                                 outputs=[gating_prob_i_expanded]))
        
        weighted_out_i = gs.Variable(f"WeightedOutput_exp{i}", dtype=combine_dtype)
        self.nodes.append(gs.Node("Mul", f"WeightingMul_exp{i}",
                           inputs=[expert_out_i_for_weighting, gating_prob_i_expanded],
                           outputs=[weighted_out_i]))
        weighted_expert_outputs_final.append(weighted_out_i)

    if not weighted_expert_outputs_final: raise ValueError("No weighted outputs.")
    current_weighted_sum = weighted_expert_outputs_final[0]
    for i in range(1, num_total_experts):
        sum_out_var = gs.Variable(f"moe_final_sum_out_{i}", dtype=combine_dtype)
        self.nodes.append(gs.Node("Add", f"MoE_FinalAdd_{i}",
                           inputs=[current_weighted_sum, weighted_expert_outputs_final[i]],
                           outputs=[sum_out_var]))
        current_weighted_sum = sum_out_var
    final_moe_output = current_weighted_sum
    print(f"MoE final combined output: {final_moe_output.name} (dtype: {final_moe_output.dtype}, shape: {final_moe_output.shape})")


    # 4. Reconnect
    # If the dtype of final_moe_output doesn't match original_head_output_tensor.dtype (which feeds into node_after_head)
    # we might need a final Cast.
    # original_head_output_tensor.dtype was set to float32 earlier, combine_dtype is float32. So should be fine.

    node_after_head.inputs[original_input_idx] = final_moe_output

    # 5. Cleanup
    if not any(out.name == ORIGINAL_HEAD_OUTPUT_TENSOR_NAME for out in self.outputs):
        original_producer_node = find_node_by_output_name(self, ORIGINAL_HEAD_OUTPUT_TENSOR_NAME)
        if original_producer_node:
            print(f"Cleaning consumers of original head output: {original_head_output_tensor.name}")
            if original_head_output_tensor in original_producer_node.outputs: # Ensure it's still listed as an output
                 # Clear consumers of this specific tensor instance
                original_head_output_tensor.outputs.clear()


    self.cleanup().toposort()
    print("Heterogeneous MoE EP modification finished.")
    return self

# --- Main Execution ---
if __name__ == "__main__":
    try:
        print(f"Loading ONNX model from: {ONNX_MODEL_PATH}")
        onnx_model = onnx.load(ONNX_MODEL_PATH)
        graph = gs.import_onnx(onnx_model)
        
        # It's good practice to infer shapes if possible before major modifications
        # graph.infer_shapes() # This might fail if the model is complex or has issues

        print(f"Original graph nodes: {len(graph.nodes)}")
        graph = graph.modify_for_all_expert_types_moe(EXPERT_TYPES)
        print(f"Modified graph nodes: {len(graph.nodes)}")
        
        graph.cleanup().toposort()
        # graph.infer_shapes() # Try inferring shapes again after modification
        modified_onnx_model = gs.export_onnx(graph)
        
        if not modified_onnx_model.opset_import:
            print("Adding default opset import.")
            if onnx_model.opset_import:
                original_opset = onnx_model.opset_import[0]
                modified_onnx_model.opset_import.extend([onnx.helper.make_opsetid(original_opset.domain, original_opset.version)])
            else: # Add a common opset if original had none
                modified_onnx_model.opset_import.extend([onnx.helper.make_opsetid("", 14)])


        onnx.save(modified_onnx_model, MODIFIED_ONNX_PATH)
        print(f"Modified Heterogeneous MoE model saved to: {MODIFIED_ONNX_PATH}")

        # # (Optional) Validate model - might fail if shapes are not perfectly set
        # print("Checking model...")
        # onnx.checker.check_model(modified_onnx_model)
        # print("Modified model checked successfully.")

    except Exception as e:
        print(f"An error occurred: {e}")
        import traceback
        traceback.print_exc()
