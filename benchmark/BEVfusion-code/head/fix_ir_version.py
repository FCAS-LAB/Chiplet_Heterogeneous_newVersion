import onnx

input_model_path = "/home/ting/SourceCode/Chiplet_Heterogeneous_newVersion/benchmark/BEVfusion-code/head/model_metabev_moe_simplified.onnx"
# 将修复后的模型保存为新文件名
output_model_path = "/home/ting/SourceCode/Chiplet_Heterogeneous_newVersion/benchmark/BEVfusion-code/head/model_metabev_moe_simplified_ir_fixed.onnx"

# ONNX Runtime 支持的最大 IR 版本是 10。
# 原始模型的 IR 版本是 7。为了安全起见，我们可以设置为 7、9 或 10。
target_ir_version = 10 # 或者 9, 或者 7

try:
    # 加载模型
    model = onnx.load(input_model_path)
    print(f"原始模型 IR 版本: {model.ir_version}")

    # 修改 IR 版本
    model.ir_version = target_ir_version
    print(f"设置模型 IR 版本为: {model.ir_version}")

    # 保存修改后的模型
    onnx.save(model, output_model_path)
    print(f"具有 IR 版本 {model.ir_version} 的模型已保存到: {output_model_path}")

except Exception as e:
    print(f"处理模型时发生错误: {e}")
