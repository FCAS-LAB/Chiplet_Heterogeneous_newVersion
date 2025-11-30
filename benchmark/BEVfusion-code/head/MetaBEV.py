import onnx_graphsurgeon as gs
import onnx
import numpy as np
import copy

# --- 配置参数 ---
ONNX_MODEL_PATH = "optimized_model.optimized.onnx" # 您的原始模型路径
MODIFIED_ONNX_PATH = "model_metabev_moe_simplified.onnx" # 修改后模型的保存路径
NUM_EXPERTS = 3  # 您希望创建的MLP专家数量 (例如，3个)

# --- 从 model_info.txt 和之前脚本中获取的关键名称 ---
HEAD_INPUT_TENSOR_NAME = "InsertedCast_1048"
ORIGINAL_HEAD_OUTPUT_TENSOR_NAME = "InsertedCast_1096" # MoE的输出将替代这个张量
NODE_AFTER_HEAD_OP_TYPE = "Sigmoid" # 消耗 ORIGINAL_HEAD_OUTPUT_TENSOR_NAME 的节点类型
NODE_AFTER_HEAD_NAME = "Sigmoid_455"    # 消耗 ORIGINAL_HEAD_OUTPUT_TENSOR_NAME 的节点名称

# --- 形状和类型假设 (非常重要，请根据您的模型精确调整) ---
# 假设 HEAD_INPUT_TENSOR_NAME (`InsertedCast_1048`) 的形状是 [Batch, Channels, SeqLen]
# 根据之前的分析，Conv_435 (输出 InsertedCast_1096) 的权重是 [10, 256, 1]，
# 这意味着其输入 (InsertedCast_1093，来自Relu_434) 有256个通道。
# 我们假设 HEAD_INPUT_TENSOR_NAME 也有128个通道。
CHANNELS_IN_HEAD = 128
SEQ_LEN_HEAD = 200 # 假设序列长度/空间维度为200

# MoE模块最终输出的通道数和序列长度。
# 应与 ORIGINAL_HEAD_OUTPUT_TENSOR_NAME (InsertedCast_1096) 的形状一致。
# InsertedCast_1096 是 score 输出 (shape [1,10,200]) 的前置张量，所以通道数为10。
EXPECTED_MOE_OUTPUT_CHANNELS = 10
EXPECTED_MOE_OUTPUT_LENGTH = SEQ_LEN_HEAD # 通常保持序列长度不变

# --- 辅助函数 ---
def find_node_by_output_name(graph, tensor_name):
    """通过输出张量名称查找节点"""
    for node in graph.nodes:
        for out_tensor in node.outputs:
            if out_tensor.name == tensor_name:
                return node
    return None

def _create_mlp_layer_1dconv(graph, expert_id, mlp_block_id, input_tensor,
                             in_features, out_features, add_relu=True,
                             dtype_np=np.float32, suffix_prefix=""):
    """
    辅助函数：使用1x1卷积为 [Batch, Channels, SeqLen] 输入创建单个MLP层。
    输出形状为 [Batch, out_features, SeqLen]。
    """
    suffix = f"{suffix_prefix}_expert{expert_id}_mlp_block{mlp_block_id}"

    # 1x1卷积的权重形状: [out_features, in_features, 1]
    w_values = np.random.randn(out_features, in_features, 1).astype(dtype_np)
    b_values = np.random.randn(out_features).astype(dtype_np) #偏置形状: [out_features]
    
    mlp_w = gs.Constant(name=f"MLPW{suffix}", values=w_values)
    mlp_b = gs.Constant(name=f"MLPB{suffix}", values=b_values)

    conv_out_tensor = gs.Variable(name=f"MLPConvOut{suffix}", dtype=dtype_np, 
                                 shape=[None, out_features, None] # 部分指定形状
                                 ) 
    conv_node = gs.Node(op="Conv", name=f"MLPConv{suffix}",
                        inputs=[input_tensor, mlp_w, mlp_b],
                        outputs=[conv_out_tensor],
                        attrs={'kernel_shape': [1], 'strides': [1], 'pads': [0, 0]})
    graph.nodes.append(conv_node)
    current_tensor = conv_out_tensor

    if add_relu:
        relu_out_tensor = gs.Variable(name=f"MLPReluOut{suffix}", dtype=dtype_np,
                                     shape=[None, out_features, None])
        relu_node = gs.Node(op="Relu", name=f"MLPRelu{suffix}", inputs=[current_tensor], outputs=[relu_out_tensor])
        graph.nodes.append(relu_node)
        current_tensor = relu_out_tensor
    return current_tensor

def create_mlp_expert(graph, expert_id, expert_input_tensor,
                      input_channels, hidden_features_list, output_channels,
                      dtype_np=np.float32):
    """
    创建一个MLP专家。
    expert_input_tensor: 输入张量，形状 [Batch, input_channels, SeqLen]
    hidden_features_list: 一个列表，指定每个隐藏层的大小。例如 [256, 128]
    output_channels: 此专家最终输出的通道数。
    输出形状为 [Batch, output_channels, SeqLen]。
    """
    print(f"  Creating MLP Expert (ID: {expert_id}) with hidden layers: {hidden_features_list}, output_channels: {output_channels}")
    current_tensor = expert_input_tensor
    current_in_channels = input_channels

    # 隐藏层
    for i, hidden_features in enumerate(hidden_features_list):
        current_tensor = _create_mlp_layer_1dconv(graph, expert_id, i, current_tensor,
                                                  current_in_channels, hidden_features, add_relu=True,
                                                  dtype_np=dtype_np, suffix_prefix="MLPExp")
        current_in_channels = hidden_features # 下一层的输入通道是当前层的输出通道
    
    # 输出层 (通常在FFN的最后没有ReLU，除非它是MoE中专家输出后还有共享层)
    # MetaBEV论文中的 M^2oE-FFN 结构图（Fig 4 I, II）显示专家输出后可能直接加权求和或融合。
    final_out_tensor = _create_mlp_layer_1dconv(graph, expert_id, "output", current_tensor,
                                                current_in_channels, output_channels, add_relu=False, # 通常专家输出层不加ReLU
                                                dtype_np=dtype_np, suffix_prefix="MLPExp")
    return final_out_tensor


def create_gating_network_for_moe(graph, gating_input_tensor, num_total_experts,
                                  input_channels_for_gating, # 这是池化后输入给门控FC层的特征维度
                                  dtype_np=np.float32):
    """
    创建一个简化的门控网络。
    gating_input_tensor: 输入张量，形状 [Batch, Channels, SeqLen]
    input_channels_for_gating: 经过池化后，输入到门控FC层的特征维度 (应等于Channels)
    输出形状为 [Batch, num_total_experts]。
    """
    print("  Creating Gating Network for MoE...")

    # 1. 全局平均池化 (或ReduceMean) 将 [B, C, L] 转换为 [B, C]
    # 假设 gating_input_tensor 的形状是 [Batch, input_channels_for_gating, SeqLen]
    # 我们在 SeqLen 维度 (axis=2) 上进行 ReduceMean
    reduce_mean_axes_attr = [2] # 默认为 [N,C,L] -> [N,C] 的情况，规约最后一个维度
    if gating_input_tensor.shape and len(gating_input_tensor.shape) == 4: # 如果是 [N,C,H,W]
        reduce_mean_axes_attr = [2, 3] # 对 H, W 进行平均
    
    # 输出形状 [Batch, input_channels_for_gating]
    # 尝试保留批处理维度（如果它是动态的 None）
    batch_dim_shape = None
    if gating_input_tensor.shape and gating_input_tensor.shape[0] is None:
        batch_dim_shape = None
    elif gating_input_tensor.shape:
        batch_dim_shape = gating_input_tensor.shape[0]
        
    pooled_features_shape = [batch_dim_shape, input_channels_for_gating]

    pooled_features_var = gs.Variable(name="GatingMoE_Pooled_Features", 
                                     dtype=gating_input_tensor.dtype, # 使用输入张量的数据类型
                                     shape=pooled_features_shape
                                     ) 
    # 关键修改：移除了 rm_axes_const，并将 axes 作为属性传递
    rm_node = gs.Node(op="ReduceMean", name="GatingMoE_ReduceMean",
                      inputs=[gating_input_tensor], # 只有一个输入：需要被规约的张量
                      outputs=[pooled_features_var], 
                      attrs={"axes": reduce_mean_axes_attr, "keepdims": 0}) # axes 现在是属性
    graph.nodes.append(rm_node)

    # 确保输入到FC层的是 float32
    fc_input_tensor = pooled_features_var
    if pooled_features_var.dtype != dtype_np:
        # pooled_features_var.shape 可能在gs中不完全确定，尤其是动态维度
        # 我们期望的形状是 [batch_dim_shape, input_channels_for_gating]
        casted_pooled_shape = pooled_features_var.shape 
        # 检查形状是否有效且符合预期
        if casted_pooled_shape is None or \
           len(casted_pooled_shape) != 2 or \
           casted_pooled_shape[0] is None or \
           casted_pooled_shape[1] is None: 
            casted_pooled_shape = [None, input_channels_for_gating] # 使用一个合理的备选形状
            print(f"[Warning] Shape for GatingMoE_Pooled_CastedToFP32 was incomplete or invalid, using {casted_pooled_shape}")


        casted_pooled_features_var = gs.Variable(name="GatingMoE_Pooled_CastedToFP32", dtype=dtype_np,
                                                shape=casted_pooled_shape)
        cast_pooled_node = gs.Node("Cast", "Cast_GatingMoE_PooledToFP32", 
                                   inputs=[pooled_features_var], outputs=[casted_pooled_features_var], 
                                   attrs={"to": onnx.TensorProto.FLOAT})
        graph.nodes.append(cast_pooled_node)
        fc_input_tensor = casted_pooled_features_var
    
    # 2. 全连接层 (MatMul + Add) 将 [B, C] 转换为 [B, num_total_experts] (logits)
    fc_w_values = np.random.randn(input_channels_for_gating, num_total_experts).astype(dtype_np)
    fc_b_values = np.random.randn(num_total_experts).astype(dtype_np)
    
    fc_w_const = gs.Constant(name="GatingMoE_FC_W", values=fc_w_values)
    fc_b_const = gs.Constant(name="GatingMoE_FC_B", values=fc_b_values)

    # MatMul 输出形状 [batch_dim_shape, num_total_experts]
    matmul_out_shape = [batch_dim_shape, num_total_experts]
    matmul_out_var = gs.Variable(name="GatingMoE_MatMul_Output", dtype=dtype_np,
                                shape=matmul_out_shape)
    matmul_node = gs.Node(op="MatMul", name="GatingMoE_MatMul", 
                          inputs=[fc_input_tensor, fc_w_const], outputs=[matmul_out_var])
    graph.nodes.append(matmul_node)

    # Add 输出形状与 MatMul 输出形状相同
    logits_var = gs.Variable(name="GatingMoE_Logits", dtype=dtype_np,
                            shape=matmul_out_shape)
    add_bias_node = gs.Node(op="Add", name="GatingMoE_Add_Bias", 
                            inputs=[matmul_out_var, fc_b_const], outputs=[logits_var])
    graph.nodes.append(add_bias_node)

    # 3. Softmax 得到每个专家的概率/权重
    # Softmax 输出形状与输入形状相同
    probs_var = gs.Variable(name="GatingMoE_Probabilities", dtype=dtype_np,
                           shape=matmul_out_shape)
    softmax_node = gs.Node(op="Softmax", name="GatingMoE_Softmax", 
                           inputs=[logits_var], outputs=[probs_var], attrs={"axis": -1}) # Softmax over experts
    graph.nodes.append(softmax_node)
    
    return probs_var

@gs.Graph.register()
def add_metabev_like_moe_to_head(self, num_moe_experts):
    """
    在 head 模块中加入一个简化的、类似 MetaBEV M^2oE 思想的 MoE 结构。
    MoE 由一个门控网络和多个MLP专家组成。
    """
    print(f"Starting MetaBEV-like MoE modification with {num_moe_experts} MLP experts...")

    # --- 1. 确定输入、输出和后续节点 ---
    head_input_tensor = self.tensors().get(HEAD_INPUT_TENSOR_NAME)
    if not head_input_tensor: 
        raise ValueError(f"Head input tensor '{HEAD_INPUT_TENSOR_NAME}' not found.")
    
    # 确保MoE的输入是 float32
    if head_input_tensor.dtype is None: # 有时gs加载模型后dtype可能是None
        print(f"[Warning] Head input tensor '{head_input_tensor.name}' dtype is None. Assuming float32 for MoE.")
        head_input_tensor.dtype = np.float32 
        # 注意：如果原始输入不是float32，这里可能需要一个显式的Cast节点，但我们先假设后续操作会处理
        # 或者在传递给专家和门控前进行Cast

    input_for_moe = head_input_tensor
    if head_input_tensor.dtype != np.float32:
        print(f"Casting head input tensor '{head_input_tensor.name}' from {head_input_tensor.dtype} to float32 for MoE.")
        casted_input_var = gs.Variable(name=head_input_tensor.name + "_to_fp32_for_moe", dtype=np.float32,
                                       shape=head_input_tensor.shape) # 尝试保留形状
        cast_node = gs.Node(op="Cast", name=f"Cast_{head_input_tensor.name}_ToFP32",
                            inputs=[head_input_tensor], outputs=[casted_input_var],
                            attrs={"to": onnx.TensorProto.FLOAT})
        self.nodes.append(cast_node)
        input_for_moe = casted_input_var
    
    print(f"Input for MoE (experts & gating): {input_for_moe.name} (dtype: {input_for_moe.dtype}, shape: {input_for_moe.shape})")


    original_head_output_tensor = self.tensors().get(ORIGINAL_HEAD_OUTPUT_TENSOR_NAME)
    if not original_head_output_tensor: 
        raise ValueError(f"Original head output tensor '{ORIGINAL_HEAD_OUTPUT_TENSOR_NAME}' not found.")

    node_after_head = None
    # 查找消耗 ORIGINAL_HEAD_OUTPUT_TENSOR_NAME 的节点
    for node_iter in list(self.nodes): # 使用list副本以允许在迭代中修改图
        if original_head_output_tensor in node_iter.inputs:
            if node_iter.name == NODE_AFTER_HEAD_NAME and node_iter.op == NODE_AFTER_HEAD_OP_TYPE:
                node_after_head = node_iter
                break
            elif node_after_head is None: # 备选：第一个消耗者
                node_after_head = node_iter
    
    if not node_after_head:
        if original_head_output_tensor in self.outputs: # 检查是否是图输出
            # 如果是图输出，处理逻辑会不同，MoE的输出需要成为新的图输出
            print(f"[Warning] Original head output '{ORIGINAL_HEAD_OUTPUT_TENSOR_NAME}' is a graph output. MoE output will replace it.")
            # 记录它是哪个图输出索引
            graph_output_idx_to_replace = -1
            for idx, go_tensor in enumerate(self.outputs):
                if go_tensor.name == ORIGINAL_HEAD_OUTPUT_TENSOR_NAME:
                    graph_output_idx_to_replace = idx
                    break
            if graph_output_idx_to_replace == -1: # 理论上不应发生
                 raise ValueError(f"'{ORIGINAL_HEAD_OUTPUT_TENSOR_NAME}' is a graph output but couldn't find its index.")
        else:
            raise ValueError(f"No node found consuming '{ORIGINAL_HEAD_OUTPUT_TENSOR_NAME}' and it's not a graph output.")

    original_input_idx_in_node_after_head = -1
    if node_after_head: # 仅当它不是图输出时，才查找在后续节点中的输入索引
        for idx, inp_t in enumerate(node_after_head.inputs):
            if inp_t.name == ORIGINAL_HEAD_OUTPUT_TENSOR_NAME:
                original_input_idx_in_node_after_head = idx
                break
        if original_input_idx_in_node_after_head == -1:
            raise ValueError(f"Could not find '{ORIGINAL_HEAD_OUTPUT_TENSOR_NAME}' in inputs of '{node_after_head.name}'.")
        print(f"Node after head: '{node_after_head.name}' (op: {node_after_head.op}), input index to replace: {original_input_idx_in_node_after_head}")
    else: # original_head_output_tensor 是图输出
        print(f"Original head output '{ORIGINAL_HEAD_OUTPUT_TENSOR_NAME}' is graph output at index {graph_output_idx_to_replace}.")


    # --- 2. 创建门控网络 ---
    # 门控网络的输入通道数是 input_for_moe 的通道数
    gating_probs_tensor = create_gating_network_for_moe(self, input_for_moe, num_moe_experts, CHANNELS_IN_HEAD)
    print(f"Gating probabilities tensor: {gating_probs_tensor.name} (dtype: {gating_probs_tensor.dtype}, shape: {gating_probs_tensor.shape})")

    # --- 3. 创建 MLP 专家 ---
    expert_output_tensors = []
    # 示例：为不同的专家配置不同的隐藏层结构
    mlp_expert_hidden_configs = [
        [128],          # 专家0: 1个隐藏层，128单元
        [256, 128],     # 专家1: 2个隐藏层
        [64]            # 专家2: 1个隐藏层，64单元
    ]
    if num_moe_experts > len(mlp_expert_hidden_configs): # 如果需要的专家数更多，重复最后一个配置
        mlp_expert_hidden_configs.extend([mlp_expert_hidden_configs[-1]] * (num_moe_experts - len(mlp_expert_hidden_configs)))

    for i in range(num_moe_experts):
        expert_hidden_config = mlp_expert_hidden_configs[i]
        expert_out_tensor = create_mlp_expert(self, i, input_for_moe,
                                              CHANNELS_IN_HEAD, 
                                              expert_hidden_config, 
                                              EXPECTED_MOE_OUTPUT_CHANNELS) # 所有专家输出相同的通道数
        expert_output_tensors.append(expert_out_tensor)
        print(f"  MLP Expert {i} created, output: {expert_out_tensor.name} (dtype: {expert_out_tensor.dtype}, shape: {expert_out_tensor.shape})")

    # --- 4. 加权合并专家输出 ---
    combine_dtype = np.float32 # 加权和合并时使用 float32
    weighted_expert_outputs = []

    if len(expert_output_tensors) != num_moe_experts:
        raise RuntimeError("Mismatch between requested number of experts and created expert outputs.")

    for i in range(num_moe_experts):
        expert_out_i = expert_output_tensors[i]
        
        # 确保专家输出是 combine_dtype (float32)
        expert_out_i_for_weighting = expert_out_i
        if expert_out_i.dtype != combine_dtype:
            print(f"Casting expert {i} output '{expert_out_i.name}' from {expert_out_i.dtype} to {combine_dtype} for weighting.")
            casted_expert_out_var = gs.Variable(name=expert_out_i.name + "_tocombine_moe", dtype=combine_dtype,
                                                shape=expert_out_i.shape)
            cast_expert_node = gs.Node("Cast", f"Cast_Exp{i}_ToCombineMoE", 
                                       inputs=[expert_out_i], outputs=[casted_expert_out_var],
                                       attrs={"to": onnx.helper.np_dtype_to_tensor_dtype(combine_dtype)})
            self.nodes.append(cast_expert_node)
            expert_out_i_for_weighting = casted_expert_out_var
        
        # 从 gating_probs_tensor [Batch, num_experts] 中 Slice 出第 i 个专家的概率 [Batch, 1]
        slice_starts_val = np.array([0, i], dtype=np.int64) # Batch维度从0开始, Expert维度从i开始
        slice_ends_val = np.array([np.iinfo(np.int64).max, i + 1], dtype=np.int64) # Batch维度取完, Expert维度取1个
        slice_axes_val = np.array([0, 1], dtype=np.int64) # 作用于Batch和Expert维度
        
        slice_starts_const = gs.Constant(f"GatingMoESlice_starts_exp{i}", slice_starts_val)
        slice_ends_const = gs.Constant(f"GatingMoESlice_ends_exp{i}", slice_ends_val)
        slice_axes_const = gs.Constant(f"GatingMoESlice_axes_exp{i}", slice_axes_val)

        gating_prob_for_expert_i = gs.Variable(name=f"GatingMoEProb_for_Exp{i}", dtype=gating_probs_tensor.dtype,
                                               shape=[None, 1]) # 形状 [Batch, 1]
        slice_node = gs.Node(op="Slice", name=f"GatingMoESlice_Exp{i}",
                             inputs=[gating_probs_tensor, slice_starts_const, slice_ends_const, slice_axes_const],
                             outputs=[gating_prob_for_expert_i])
        self.nodes.append(slice_node)

        # 将门控概率 gating_prob_for_expert_i [B,1] Unsqueeze 以匹配专家输出 expert_out_i_for_weighting [B, C_out, L_out]
        # 目标是 [B,1,1] 以便广播。在第2个维度（新的最后一个维度）添加新轴。
        unsqueeze_axes_val = np.array([2], dtype=np.int64) # 从 [B,1] 变为 [B,1,1]
        unsqueeze_axes_const = gs.Constant(name=f"UnsqueezeAxesMoE_Exp{i}", values=unsqueeze_axes_val)
        
        gating_prob_expanded = gs.Variable(name=f"GatingMoEProbExpanded_Exp{i}", dtype=gating_probs_tensor.dtype,
                                           shape=[None, 1, 1])
        unsqueeze_node = gs.Node(op="Unsqueeze", name=f"GatingMoEUnsqueeze_Exp{i}",
                                 inputs=[gating_prob_for_expert_i, unsqueeze_axes_const],
                                 outputs=[gating_prob_expanded])
        self.nodes.append(unsqueeze_node)
        
        # 加权专家输出: expert_out * gating_prob_expanded
        weighted_out_i = gs.Variable(name=f"WeightedMoEOutput_Exp{i}", dtype=combine_dtype,
                                     shape=expert_out_i_for_weighting.shape) # 输出形状与专家输出一致
        mul_node = gs.Node(op="Mul", name=f"WeightingMoEMul_Exp{i}",
                           inputs=[expert_out_i_for_weighting, gating_prob_expanded],
                           outputs=[weighted_out_i])
        self.nodes.append(mul_node)
        weighted_expert_outputs.append(weighted_out_i)

    # 将所有加权后的专家输出相加
    if not weighted_expert_outputs: 
        raise ValueError("No weighted expert outputs to sum for MoE.")
    
    current_sum_tensor = weighted_expert_outputs[0]
    for i in range(1, num_moe_experts):
        sum_out_var = gs.Variable(name=f"MoE_FinalSum_Accumulated_{i}", dtype=combine_dtype,
                                  shape=current_sum_tensor.shape)
        add_node = gs.Node(op="Add", name=f"MoE_FinalAdd_Accumulated_{i}",
                           inputs=[current_sum_tensor, weighted_expert_outputs[i]],
                           outputs=[sum_out_var])
        self.nodes.append(add_node)
        current_sum_tensor = sum_out_var
    
    final_moe_output_tensor = current_sum_tensor
    print(f"Final MoE combined output: {final_moe_output_tensor.name} (dtype: {final_moe_output_tensor.dtype}, shape: {final_moe_output_tensor.shape})")

    # --- 5. 重新连接 ---
    if node_after_head: # 如果原始输出不是图输出
        print(f"Reconnecting MoE output to input {original_input_idx_in_node_after_head} of node '{node_after_head.name}'")
        node_after_head.inputs[original_input_idx_in_node_after_head] = final_moe_output_tensor
    else: # 如果原始输出是图输出
        print(f"Replacing graph output at index {graph_output_idx_to_replace} with MoE output.")
        self.outputs[graph_output_idx_to_replace] = final_moe_output_tensor


    # --- 6. 清理 ---
    # 标记原始的 head 输出张量不再被使用（除非它是图输出，此时已被替换）
    # 以便 graph.cleanup() 可以移除原始的、现在已断开连接的 head 计算路径
    if node_after_head: # 只有当它不是图输出时，才清空其消费者
        is_still_a_graph_output = any(go.name == ORIGINAL_HEAD_OUTPUT_TENSOR_NAME for go in self.outputs)
        if not is_still_a_graph_output:
            original_producer_node = find_node_by_output_name(self, ORIGINAL_HEAD_OUTPUT_TENSOR_NAME)
            if original_producer_node and original_head_output_tensor in original_producer_node.outputs:
                print(f"Clearing consumers of original head output tensor: '{original_head_output_tensor.name}'")
                original_head_output_tensor.outputs.clear() # 断开它作为任何节点输出的连接

    self.cleanup().toposort()
    print("Simplified MetaBEV-like MoE modification finished.")
    return self

# --- 主执行流程 ---
if __name__ == "__main__":
    try:
        print(f"Loading ONNX model from: {ONNX_MODEL_PATH}")
        onnx_model = onnx.load(ONNX_MODEL_PATH)
        graph = gs.import_onnx(onnx_model)
        
        print(f"Original graph nodes: {len(graph.nodes)}")
        # 应用 MoE 修改
        graph = graph.add_metabev_like_moe_to_head(NUM_EXPERTS)
        print(f"Modified graph nodes: {len(graph.nodes)}")
        
        graph.cleanup().toposort() # 再次清理和排序
        
        # (可选) 尝试推断形状，这有助于调试，但如果模型复杂或有动态形状可能失败
        # try:
        #     print("Inferring shapes of the modified graph...")
        #     graph.infer_shapes()
        #     print("Shape inference successful.")
        # except Exception as e_shape:
        #     print(f"[Warning] Shape inference failed after modification: {e_shape}")


        modified_onnx_model_proto = gs.export_onnx(graph)
        
        # 确保 Opset Import 存在
        if not modified_onnx_model_proto.opset_import:
            print("Adding default opset import to modified model.")
            if onnx_model.opset_import: # 从原始模型复制
                original_opset_info = onnx_model.opset_import[0]
                opset_id = onnx.helper.make_opsetid(original_opset_info.domain, original_opset_info.version)
            else: # 如果原始模型也没有，则添加一个通用的
                opset_id = onnx.helper.make_opsetid("", 14) # 例如 Opset 14
            modified_onnx_model_proto.opset_import.extend([opset_id])

        onnx.save(modified_onnx_model_proto, MODIFIED_ONNX_PATH)
        print(f"Modified MetaBEV-like MoE model saved to: {MODIFIED_ONNX_PATH}")

        # (可选) 验证模型结构
        # print("Checking modified model structure...")
        # onnx.checker.check_model(modified_onnx_model_proto)
        # print("Modified model structure check passed.")

    except Exception as e:
        print(f"An error occurred: {e}")
        import traceback
        traceback.print_exc()
