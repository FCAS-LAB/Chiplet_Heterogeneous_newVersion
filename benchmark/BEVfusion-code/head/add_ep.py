import onnx_graphsurgeon as gs
import onnx
import numpy as np
import copy # For deepcopying nodes and initializers

# --- Configuration ---
ONNX_MODEL_PATH = "optimized_model.optimized.onnx"
MODIFIED_ONNX_PATH = "model_ep.onnx"
NUM_EXPERTS = 2

# --- Key Names from model_info.txt ---
HEAD_INPUT_TENSOR_NAME = "InsertedCast_1048"

# Inputs to Conv_432 (Node 239)
CONV1_INPUT_DATA_TENSOR_NAME = "InsertedCast_1048" # This is the actual input from previous layer
CONV1_WEIGHT_INPUT_TENSOR_NAME = "InsertedCast_ConvBnFusion_W_1232" # Output of a Cast
CONV1_BIAS_INPUT_TENSOR_NAME = "InsertedCast_ConvBnFusion_BN_B_parent.heads.object.prediction_heads.0.heatmap.0.bn.bias" # Output of a Cast

# Inputs to Conv_435 (Node 241)
# Input data is output of Relu_434 (InsertedCast_1093)
CONV2_WEIGHT_INPUT_TENSOR_NAME = "InsertedCast_1234" # Output of a Cast
CONV2_BIAS_INPUT_TENSOR_NAME = "InsertedCast_1233" # Output of a Cast

# Original Nodes
EXPERT_CONV1_NAME = "Conv_432"
EXPERT_RELU1_NAME = "Relu_434" # Output: InsertedCast_1093
EXPERT_CONV2_NAME = "Conv_435"

ORIGINAL_HEAD_OUTPUT_TENSOR_NAME = "InsertedCast_1096"
NODE_AFTER_HEAD_OP_TYPE = "Sigmoid"
NODE_AFTER_HEAD_NAME = "Sigmoid_455"


def get_actual_initializer_value(graph, cast_output_tensor_name):
    """
    Traces back from a Cast node's output to get the original initializer's value.
    The 'cast_output_tensor_name' is the tensor name that is an input to a Conv,
    but is itself the output of a Cast operation.
    """
    tmap = graph.tensors()
    cast_output_tensor = tmap.get(cast_output_tensor_name)

    if not cast_output_tensor:
        raise ValueError(f"Tensor {cast_output_tensor_name} not found in graph.")

    # Find the node that produces this cast_output_tensor
    producer_node = None
    for node in graph.nodes:
        if cast_output_tensor in node.outputs:
            producer_node = node
            break
    
    if not producer_node:
        # This tensor might be a graph input itself if it's a weight
        if isinstance(cast_output_tensor, gs.Constant): # Unlikely if it's an "InsertedCast_*"
             print(f"[Info] Tensor {cast_output_tensor_name} is already a Constant.")
             return cast_output_tensor.values, cast_output_tensor.dtype

        # If it's a Variable and a graph input, we can't get its value directly here
        # unless it's truly an initializer passed as input (less common for weights)
        if cast_output_tensor in graph.inputs:
             raise ValueError(f"Tensor {cast_output_tensor_name} is a graph input and not produced by a Cast node as expected.")
        raise ValueError(f"Could not find producer node for {cast_output_tensor_name}")

    if producer_node.op != "Cast":
        # If the input to Conv is already a Constant (no Cast before it)
        if isinstance(cast_output_tensor, gs.Constant):
            return cast_output_tensor.values, cast_output_tensor.dtype
        raise ValueError(f"Producer of {cast_output_tensor_name} is {producer_node.op}, not 'Cast'. Original tensor type: {type(cast_output_tensor)}")

    # The input to the Cast node should be the actual initializer (Constant)
    original_initializer_tensor = producer_node.inputs[0]
    if not isinstance(original_initializer_tensor, gs.Constant):
        raise TypeError(f"Input to Cast node '{producer_node.name}' (tensor: {original_initializer_tensor.name}) is not a gs.Constant, but {type(original_initializer_tensor)}. Cannot get initializer value.")
    
    return original_initializer_tensor.values, original_initializer_tensor.dtype


def find_node_by_output_name(graph, tensor_name):
    # Utility to find a node by one of its output tensor names
    for node in graph.nodes:
        for out_tensor in node.outputs:
            if out_tensor.name == tensor_name:
                return node
    return None


def create_expert_subgraph(graph, expert_id, head_input_tensor):
    suffix = f"_expert{expert_id}"

    # 1. Get actual values and dtypes of weights/biases by looking before the Cast ops
    # For Conv1 (e.g., Conv_432)
    conv1_w_values, conv1_w_dtype = get_actual_initializer_value(graph, CONV1_WEIGHT_INPUT_TENSOR_NAME)
    conv1_b_values, conv1_b_dtype = get_actual_initializer_value(graph, CONV1_BIAS_INPUT_TENSOR_NAME)

    # For Conv2 (e.g., Conv_435)
    conv2_w_values, conv2_w_dtype = get_actual_initializer_value(graph, CONV2_WEIGHT_INPUT_TENSOR_NAME)
    conv2_b_values, conv2_b_dtype = get_actual_initializer_value(graph, CONV2_BIAS_INPUT_TENSOR_NAME)

    # Create new gs.Constant for the expert's weights
    # Note: The name of the gs.Constant should be unique.
    new_conv1_w_name = CONV1_WEIGHT_INPUT_TENSOR_NAME.replace("InsertedCast_", "") + "_val" + suffix
    new_conv1_b_name = CONV1_BIAS_INPUT_TENSOR_NAME.replace("InsertedCast_", "") + "_val" + suffix
    new_conv2_w_name = CONV2_WEIGHT_INPUT_TENSOR_NAME.replace("InsertedCast_", "") + "_val" + suffix
    new_conv2_b_name = CONV2_BIAS_INPUT_TENSOR_NAME.replace("InsertedCast_", "") + "_val" + suffix


    new_conv1_w = gs.Constant(name=new_conv1_w_name, values=np.copy(conv1_w_values))
    new_conv1_b = gs.Constant(name=new_conv1_b_name, values=np.copy(conv1_b_values))
    new_conv2_w = gs.Constant(name=new_conv2_w_name, values=np.copy(conv2_w_values))
    new_conv2_b = gs.Constant(name=new_conv2_b_name, values=np.copy(conv2_b_values))


    # 2. Create new Cast nodes for the expert's weights/biases to mimic original structure if needed
    #    Or, if the Conv can take the original dtype directly, we can skip this.
    #    The original model has Cast nodes before Conv inputs, so we replicate this.
    #    The target 'to' type for the Cast should be FLOAT16 (10) or FLOAT (1) based on model_info.txt
    #    From model_info.txt, the 'to' attribute for these Casts is 1 (FLOAT).
    #    However, the graph inputs/outputs are FLOAT16. Let's check the dtype of head_input_tensor.
    #    The error log showed head_input_tensor.dtype as None, which is an issue.
    #    We'll assume the Conv expects FLOAT (as per original Cast nodes 'to: 1')
    #    If head_input_tensor.dtype is None, we need to infer or set a default.
    #    The graph output dtypes (score, rot etc.) are FLOAT16.
    #    The very first Cast for 'middle' (Node 22) is 'to: 1 (INT)', this is incorrect.
    #    It seems the dtypes are not correctly inferred by gs.
    #    Let's assume intermediate convs work with FLOAT32 (1) as suggested by original Casts' 'to' attribute.

    cast_to_type = np.float32 # Assuming ONNX type 1 (FLOAT) for Conv inputs based on original Casts

    # Cast for Conv1 Weights
    casted_conv1_w_tensor = gs.Variable(name=CONV1_WEIGHT_INPUT_TENSOR_NAME + suffix, dtype=cast_to_type)
    cast_node_conv1_w = gs.Node(op="Cast", name="Cast_" + new_conv1_w_name, inputs=[new_conv1_w], outputs=[casted_conv1_w_tensor], attrs={"to": onnx.TensorProto.FLOAT})
    
    # Cast for Conv1 Bias
    casted_conv1_b_tensor = gs.Variable(name=CONV1_BIAS_INPUT_TENSOR_NAME + suffix, dtype=cast_to_type)
    cast_node_conv1_b = gs.Node(op="Cast", name="Cast_" + new_conv1_b_name, inputs=[new_conv1_b], outputs=[casted_conv1_b_tensor], attrs={"to": onnx.TensorProto.FLOAT})

    # Cast for Conv2 Weights
    casted_conv2_w_tensor = gs.Variable(name=CONV2_WEIGHT_INPUT_TENSOR_NAME + suffix, dtype=cast_to_type)
    cast_node_conv2_w = gs.Node(op="Cast", name="Cast_" + new_conv2_w_name, inputs=[new_conv2_w], outputs=[casted_conv2_w_tensor], attrs={"to": onnx.TensorProto.FLOAT})

    # Cast for Conv2 Bias
    casted_conv2_b_tensor = gs.Variable(name=CONV2_BIAS_INPUT_TENSOR_NAME + suffix, dtype=cast_to_type)
    cast_node_conv2_b = gs.Node(op="Cast", name="Cast_" + new_conv2_b_name, inputs=[new_conv2_b], outputs=[casted_conv2_b_tensor], attrs={"to": onnx.TensorProto.FLOAT})
    
    graph.nodes.extend([cast_node_conv1_w, cast_node_conv1_b, cast_node_conv2_w, cast_node_conv2_b])


    # 3. Create Expert Conv1
    #    The dtype of head_input_tensor was None. This is problematic.
    #    The model output is FLOAT16. The Cast nodes for weights target FLOAT (1).
    #    Let's assume the main data path for these convs is FLOAT32.
    #    If head_input_tensor.dtype is None, we must assign it or the graph is invalid.
    #    The input 'middle' is FLOAT16. 'InsertedCast_1048' comes much later.
    #    Let's assume head_input_tensor should be FLOAT32 for these convs,
    #    or match the type of the casted weights.

    conv_dtype = cast_to_type # Use the same dtype as casted weights for conv outputs

    expert_conv1_out_tensor_name = EXPERT_CONV1_NAME + "_output" + suffix
    expert_conv1_out_tensor = gs.Variable(name=expert_conv1_out_tensor_name, dtype=conv_dtype)
    expert_conv1 = gs.Node(op="Conv", name=EXPERT_CONV1_NAME + suffix,
                           inputs=[head_input_tensor, casted_conv1_w_tensor, casted_conv1_b_tensor],
                           outputs=[expert_conv1_out_tensor])
    
    original_conv1_node_candidate = [n for n in graph.nodes if n.name == EXPERT_CONV1_NAME]
    if original_conv1_node_candidate:
        expert_conv1.attrs = copy.deepcopy(original_conv1_node_candidate[0].attrs)
    else:
        print(f"[Warning] Could not find original node {EXPERT_CONV1_NAME} to copy attributes. Setting manually.")
        # From Node 239 (Conv_432)
        expert_conv1.attrs = {'auto_pad': 'NOTSET', 'dilations': [1], 'group': 1, 'pads': [0, 0], 'strides': [1]}


    # 4. Expert Relu1
    expert_relu1_out_tensor_name = EXPERT_RELU1_NAME + "_output" + suffix
    expert_relu1_out_tensor = gs.Variable(name=expert_relu1_out_tensor_name, dtype=conv_dtype)
    expert_relu1 = gs.Node(op="Relu", name=EXPERT_RELU1_NAME + suffix,
                            inputs=[expert_conv1_out_tensor],
                            outputs=[expert_relu1_out_tensor])
    # Relu attrs are usually not critical or non-existent

    # 5. Expert Conv2
    expert_conv2_out_tensor_name = EXPERT_CONV2_NAME + "_final_output" + suffix # Renamed to avoid clash if EXPERT_CONV2_NAME_output is used elsewhere
    expert_conv2_out_tensor = gs.Variable(name=expert_conv2_out_tensor_name, dtype=conv_dtype)
    expert_conv2 = gs.Node(op="Conv", name=EXPERT_CONV2_NAME + suffix,
                           inputs=[expert_relu1_out_tensor, casted_conv2_w_tensor, casted_conv2_b_tensor],
                           outputs=[expert_conv2_out_tensor])
    
    original_conv2_node_candidate = [n for n in graph.nodes if n.name == EXPERT_CONV2_NAME]
    if original_conv2_node_candidate:
        expert_conv2.attrs = copy.deepcopy(original_conv2_node_candidate[0].attrs)
    else:
        print(f"[Warning] Could not find original node {EXPERT_CONV2_NAME} to copy attributes. Setting manually.")
        # From Node 241 (Conv_435)
        expert_conv2.attrs = {'auto_pad': 'NOTSET', 'dilations': [1], 'group': 1, 'pads': [0, 0], 'strides': [1]}

    graph.nodes.extend([expert_conv1, expert_relu1, expert_conv2])
    return expert_conv2_out_tensor


@gs.Graph.register()
def modify_for_ep(self):
    print("Starting EP modification...")

    # Infer graph dtypes if possible, or set defaults
    # This is a workaround because gs sometimes doesn't populate dtype for all tensors on import
    # For a robust solution, ONNX model should have type information.
    for tensor in self.tensors().values():
        if tensor.dtype is None:
            # Attempt to infer from producers/consumers, or set a common default like np.float32
            # This is risky; best if original ONNX has type info.
            # Based on model_info, graph output is FLOAT16, but intermediate Casts go to FLOAT (1)
            # Let's assume most intermediate ops are FLOAT32 if not specified.
            # print(f"[Warning] Tensor {tensor.name} has no dtype. Defaulting might be needed.")
            pass


    head_input_tensor = self.tensors().get(HEAD_INPUT_TENSOR_NAME, None)
    if not head_input_tensor:
        raise ValueError(f"Could not find head input tensor: {HEAD_INPUT_TENSOR_NAME}")
    # Manually set dtype if None, based on expectation (e.g., from Cast nodes' target type)
    if head_input_tensor.dtype is None:
        print(f"[Warning] Head input tensor {head_input_tensor.name} has no dtype. Assuming float32 for Conv.")
        head_input_tensor.dtype = np.float32 # Matching the 'to: 1' (FLOAT) of Casts for weights

    print(f"Found head input tensor: {head_input_tensor.name} (dtype: {head_input_tensor.dtype})")


    original_head_output_tensor = self.tensors().get(ORIGINAL_HEAD_OUTPUT_TENSOR_NAME, None)
    if not original_head_output_tensor:
        raise ValueError(f"Could not find original head output tensor: {ORIGINAL_HEAD_OUTPUT_TENSOR_NAME}")
    if original_head_output_tensor.dtype is None:
        print(f"[Warning] Original head output tensor {original_head_output_tensor.name} has no dtype. Assuming float32.")
        original_head_output_tensor.dtype = np.float32 # Should match conv_dtype
    print(f"Found original head output tensor: {original_head_output_tensor.name} (dtype: {original_head_output_tensor.dtype})")


    node_after_head = None
    # Find the node that consumes original_head_output_tensor
    # Iterate through a copy of nodes list if modifying it
    for node in list(self.nodes):
        if original_head_output_tensor in node.inputs:
            if node.name == NODE_AFTER_HEAD_NAME and node.op == NODE_AFTER_HEAD_OP_TYPE:
                node_after_head = node
                break
            elif node_after_head is None: # pick first consumer as fallback
                node_after_head = node

    if not node_after_head:
        if original_head_output_tensor in self.outputs:
             raise ValueError(f"Tensor {ORIGINAL_HEAD_OUTPUT_TENSOR_NAME} is a graph output. This EP logic needs adjustment if the head's direct output is also a graph output.")
        raise ValueError(f"No node found consuming {ORIGINAL_HEAD_OUTPUT_TENSOR_NAME}")

    print(f"Node after head is: {node_after_head.name}, op: {node_after_head.op}")

    original_input_idx_for_node_after_head = -1
    for idx, inp_tensor in enumerate(node_after_head.inputs):
        if inp_tensor.name == ORIGINAL_HEAD_OUTPUT_TENSOR_NAME:
            original_input_idx_for_node_after_head = idx
            break
    if original_input_idx_for_node_after_head == -1:
        raise ValueError(f"Could not find {ORIGINAL_HEAD_OUTPUT_TENSOR_NAME} in inputs of {node_after_head.name}")

    expert_output_tensors = []
    print(f"Creating {NUM_EXPERTS} experts...")
    for i in range(NUM_EXPERTS):
        print(f"  Creating expert {i}...")
        expert_out = create_expert_subgraph(self, i, head_input_tensor)
        expert_output_tensors.append(expert_out)
        print(f"  Expert {i} created, output tensor: {expert_out.name} (dtype: {expert_out.dtype})")

    if NUM_EXPERTS == 0:
        raise ValueError("NUM_EXPERTS must be at least 1.")
    elif NUM_EXPERTS == 1:
        combined_expert_output = expert_output_tensors[0]
        print(f"Using output from single expert: {combined_expert_output.name}")
    else:
        print(f"Combining outputs from {NUM_EXPERTS} experts...")
        
        # Determine a consistent dtype for Add and Div operations
        # Prefer a more precise type if available, otherwise default
        combine_dtype = np.float32 # Default
        if all(eo.dtype is not None for eo in expert_output_tensors):
            # If all expert outputs have a dtype, try to use it
            # This simple logic takes the first one, assumes they are consistent
            if expert_output_tensors[0].dtype == np.float16:
                combine_dtype = np.float16
            elif expert_output_tensors[0].dtype == np.float32:
                combine_dtype = np.float32
            # Add more types if necessary
        print(f"Using {combine_dtype} for combining expert outputs.")


        current_sum = expert_output_tensors[0]
        # Ensure first tensor in sum has the target combine_dtype
        if current_sum.dtype != combine_dtype:
            print(f"Casting first expert output {current_sum.name} from {current_sum.dtype} to {combine_dtype} for summation.")
            prev_sum_casted = gs.Variable(name=current_sum.name + "_tocombinecast", dtype=combine_dtype)
            cast_to_combine_node = gs.Node(op="Cast", name="CastToCombineSum_" + current_sum.name, inputs=[current_sum], outputs=[prev_sum_casted], attrs={"to": onnx.helper.np_dtype_to_tensor_dtype(combine_dtype)})
            self.nodes.append(cast_to_combine_node)
            current_sum = prev_sum_casted


        for i in range(1, NUM_EXPERTS):
            expert_i_output = expert_output_tensors[i]
            if expert_i_output.dtype != combine_dtype:
                print(f"Casting expert {i} output {expert_i_output.name} from {expert_i_output.dtype} to {combine_dtype} for summation.")
                expert_i_casted = gs.Variable(name=expert_i_output.name + "_tocombinecast", dtype=combine_dtype)
                cast_to_combine_node_i = gs.Node(op="Cast", name="CastToCombineSum_" + expert_i_output.name, inputs=[expert_i_output], outputs=[expert_i_casted], attrs={"to": onnx.helper.np_dtype_to_tensor_dtype(combine_dtype)})
                self.nodes.append(cast_to_combine_node_i)
                expert_i_output = expert_i_casted

            add_node_name = f"ExpertsAdd_{i-1}_{i}"
            add_out_tensor_name = f"experts_sum_out_{i}"
            add_out_tensor = gs.Variable(name=add_out_tensor_name, dtype=combine_dtype)
            add_node = gs.Node(op="Add", name=add_node_name, inputs=[current_sum, expert_i_output], outputs=[add_out_tensor])
            self.nodes.append(add_node)
            current_sum = add_out_tensor
        
        summed_experts_output = current_sum
        divisor = gs.Constant(name="expert_divisor", values=np.array(NUM_EXPERTS, dtype=combine_dtype))
        averaged_output_tensor = gs.Variable(name="experts_averaged_output", dtype=combine_dtype)
        div_node = gs.Node(op="Div", name="ExpertsAverageDiv", inputs=[summed_experts_output, divisor], outputs=[averaged_output_tensor])
        self.nodes.append(div_node)
        combined_expert_output = averaged_output_tensor
        print(f"Combined expert output (averaged): {combined_expert_output.name} (dtype: {combined_expert_output.dtype})")

    print(f"Reconnecting combined output {combined_expert_output.name} to node: {node_after_head.name} at input index {original_input_idx_for_node_after_head}")
    node_after_head.inputs[original_input_idx_for_node_after_head] = combined_expert_output
    
    # Mark the original head's output tensor as having no consumers IF it's not a graph output
    # This helps graph.cleanup() remove the original (now disconnected) head nodes.
    is_original_output_a_graph_output = False
    for graph_out_idx, graph_out_tensor in enumerate(self.outputs):
        if graph_out_tensor.name == ORIGINAL_HEAD_OUTPUT_TENSOR_NAME:
            is_original_output_a_graph_output = True
            # If it IS a graph output, we might need to update self.outputs[graph_out_idx] = combined_expert_output
            # For now, assuming it's not a direct graph output.
            print(f"[Info] Original head output {ORIGINAL_HEAD_OUTPUT_TENSOR_NAME} is also a graph output. EP modification might need to update graph outputs.")
            break

    if not is_original_output_a_graph_output:
        original_producer_node = find_node_by_output_name(self, ORIGINAL_HEAD_OUTPUT_TENSOR_NAME)
        if original_producer_node:
            # Check if original_head_output_tensor is still in original_producer_node.outputs
            # It should be, but after reconnections, its consumers list should be empty (or only node_after_head if we didn't clear it first)
            # Forcing it to be 'dangling' if not a graph output
            print(f"Cleaning consumers of original head output: {original_head_output_tensor.name}")
            original_head_output_tensor.outputs.clear() # Disconnect it from any downstream nodes it might still be linked to in gs's internal state

    self.cleanup().toposort()
    print("EP modification finished and graph cleaned up.")
    return self

# --- Main Execution ---
if __name__ == "__main__":
    try:
        print(f"Loading ONNX model from: {ONNX_MODEL_PATH}")
        onnx_model = onnx.load(ONNX_MODEL_PATH)
        graph = gs.import_onnx(onnx_model)
        
        print(f"Original graph: {len(graph.nodes)} nodes, {len(graph.inputs)} inputs, {len(graph.outputs)} outputs.")

        graph = graph.modify_for_ep()

        print(f"Modified graph: {len(graph.nodes)} nodes, {len(graph.inputs)} inputs, {len(graph.outputs)} outputs.")
        
        graph.cleanup().toposort()
        modified_onnx_model = gs.export_onnx(graph)
        
        # Add Opset Import if missing (gs sometimes removes it)
        if not modified_onnx_model.opset_import:
            # Add default opset or copy from original model
            print("Adding default opset import to modified model.")
            original_opset = onnx_model.opset_import[0] # Assuming at least one opset
            modified_onnx_model.opset_import.extend([onnx.helper.make_opsetid(original_opset.domain, original_opset.version)])

        onnx.save(modified_onnx_model, MODIFIED_ONNX_PATH)
        print(f"Modified model saved to: {MODIFIED_ONNX_PATH}")

        # (Optional) Validate model
        # print("Checking model...")
        # onnx.checker.check_model(modified_onnx_model)
        # print("Modified model checked successfully.")

    except Exception as e:
        print(f"An error occurred: {e}")
        import traceback
        traceback.print_exc()
