#include <torch/torch.h>
#include <iostream>
#include <vector>

class SubMConv3dImpl : public torch::nn::Module {
public:
    SubMConv3dImpl(int64_t in_channels, int64_t out_channels, 
                   torch::ExpandingArray<3> kernel_size,
                   torch::ExpandingArray<3> padding,
                   torch::ExpandingArray<3> stride = {1,1,1})
        : kernel_size_(kernel_size),
          padding_(padding),
          stride_(stride) 
    {
        // 正确访问ExpandingArray元素的方式
        auto kRef = kernel_size_.operator c10::ArrayRef<int64_t>();
        auto pRef = padding_.operator c10::ArrayRef<int64_t>();
        auto sRef = stride_.operator c10::ArrayRef<int64_t>();

        // 确保 stride = 1， dilation = 1（适用于 SubM 卷积）
        dilation_ = {1, 1, 1}; 

        // 计算输出空间尺寸
        output_size_.resize(3);
        for (size_t i = 0; i < 3; ++i) {
            output_size_[i] = (1 + (0 + 2 * pRef[i] - (kRef[i] - 1) - 1) / sRef[i]);
        }

        // 设定权重大小: [out_channels, in_channels, kd, kh, kw]
        std::vector<int64_t> weight_size = {
            out_channels,
            in_channels,
            kRef[0], kRef[1], kRef[2]
        };

        // 注册可学习参数
        weight_ = register_parameter("weight", torch::randn(weight_size));
        bias_ = register_parameter("bias", torch::randn({out_channels}));
    }

    void set_stride(torch::ExpandingArray<3> stride) {
        stride_ = stride;
    }

    torch::Tensor forward(torch::Tensor indices, 
                          torch::Tensor values,
                          c10::ArrayRef<int64_t> spatial_size)
    {
        auto kRef = kernel_size_.operator c10::ArrayRef<int64_t>();
        auto pRef = padding_.operator c10::ArrayRef<int64_t>();
        auto sRef = stride_.operator c10::ArrayRef<int64_t>();

        const int64_t D_in = spatial_size[0];
        const int64_t H_in = spatial_size[1];
        const int64_t W_in = spatial_size[2];
        
        int64_t D_out = (D_in + 2*pRef[0] - kRef[0]) / sRef[0] + 1;
        int64_t H_out = (H_in + 2*pRef[1] - kRef[1]) / sRef[1] + 1;
        int64_t W_out = (W_in + 2*pRef[2] - kRef[2]) / sRef[2] + 1;

        // 输出通道数
        int64_t C_out = weight_.size(0);

        // 修改：添加通道维度作为第四维
        std::vector<int64_t> output_shape = {D_out, H_out, W_out, C_out};

        // 检查输入是否为空
        if (indices.size(1) == 0) {
            return torch::sparse_coo_tensor(
                torch::zeros({3, 0}, torch::kLong),
                torch::zeros({0, C_out}),
                output_shape,
                torch::kFloat
            );
        }

        // 存储输出的 indices 和 values
        std::vector<torch::Tensor> out_indices;
        std::vector<torch::Tensor> out_values;

        // 访问输入 indices
        auto indices_a = indices.accessor<int64_t, 2>();
        auto batch_idx = indices_a[0][0];  // 获取batch索引

        // 遍历所有非零坐标
        for (int64_t n = 0; n < indices.size(1); ++n) {
            const int64_t z = indices_a[1][n];  // 注意：indices现在是4维的，第一维是batch
            const int64_t y = indices_a[2][n];
            const int64_t x = indices_a[3][n];

            // 遍历 3D 卷积窗口
            for (int64_t kz = 0; kz < kRef[0]; ++kz) {
                for (int64_t ky = 0; ky < kRef[1]; ++ky) {
                    for (int64_t kx = 0; kx < kRef[2]; ++kx) {
                        // 计算输出坐标 (考虑 padding)
                        const int64_t z_out = z - kz + pRef[0];
                        const int64_t y_out = y - ky + pRef[1];
                        const int64_t x_out = x - kx + pRef[2];

                        // 边界检查
                        if (z_out >= 0 && z_out < D_out &&
                            y_out >= 0 && y_out < H_out &&
                            x_out >= 0 && x_out < W_out)
                        {
                            // 修改权重切片方式
                            torch::Tensor w = weight_
                                .narrow(2, kz, 1)
                                .narrow(3, ky, 1)
                                .narrow(4, kx, 1)
                                .squeeze();

                            // 调整矩阵乘法
                            torch::Tensor val = torch::matmul(
                                values[n].view({1, -1}),  // [1, C_in]
                                w.transpose(0, 1)         // [C_in, C_out]
                            ).squeeze(0);  // [C_out]

                            // 加上 bias
                            val += bias_;

                            // 保存坐标和特征
                            out_indices.push_back(
                                torch::tensor({z_out, y_out, x_out}, torch::kLong)
                                    .unsqueeze(1)
                            );
                            out_values.push_back(val);
                        }
                    }
                }
            }
        }

        // 检查是否有有效输出
        if (out_indices.empty()) {
            return torch::sparse_coo_tensor(
                torch::zeros({3, 0}, torch::kLong),
                torch::zeros({0, C_out}),
                output_shape,
                torch::kFloat
            );
        }

        // 合并输出
        auto out_indices_tensor = torch::cat(out_indices, 1);  // [3, N]
        auto out_values_tensor = torch::stack(out_values, 0);  // [N, C_out]

        // 创建稀疏张量，使用4维形状
        return torch::sparse_coo_tensor(
            out_indices_tensor,
            out_values_tensor,
            output_shape,
            torch::kFloat
        ).coalesce();
    }

private:
    torch::ExpandingArray<3> kernel_size_, padding_, stride_;
    std::vector<int64_t> output_size_;
    std::vector<int64_t> dilation_;

    torch::Tensor weight_, bias_;
};

TORCH_MODULE(SubMConv3d);

// // **main 函数保持不变**
// int main() {
//     // 修改输入参数以匹配期望维度
//     int64_t D = 1440, H = 1440, W = 41;
//     int64_t C_in = 16, C_out = 16;
//     int64_t batch_size = 1;

//     torch::ExpandingArray<3> kernel_size{3, 3, 3};
//     torch::ExpandingArray<3> padding{1, 1, 1};

//     // 构造稀疏输入，确保包含batch维度
//     auto indices = torch::tensor({
//         {0},    // batch index
//         {720},  // z
//         {720},  // y
//         {20}    // x
//     }, torch::kLong);  // [4, 1]
//     auto values = torch::randn({1, C_in});  // [1, 16]

//     auto conv = SubMConv3d(C_in, C_out, kernel_size, padding);

//     // 前向传播
//     auto output = conv->forward(indices, values, {D, H, W});

//     std::cout << "Output size: " << output.sizes() << std::endl;
//     std::cout << "Number of non-zero outputs: " << output._nnz() << std::endl;

//     return 0;
// }
