#pragma once
#include <torch/torch.h>
#include "sparse_conv.h"

class LidarBackboneImpl : public torch::nn::Module {
public:
    LidarBackboneImpl() {
        auto create_conv = [](int64_t in_channels, int64_t out_channels, 
                             std::vector<int64_t> kernel_size,
                             std::vector<int64_t> padding,
                             bool subm = true) {
            auto conv = SubMConv3d(std::make_shared<SubMConv3dImpl>(
                in_channels, 
                out_channels,
                torch::ExpandingArray<3>(kernel_size),
                torch::ExpandingArray<3>(padding)
            ));
            if(!subm) conv->set_stride({2,2,2});
            return conv;
        };

        // Block 1 (subm=True)
        conv0_ = register_module("conv0", create_conv(5, 16, {3,3,3}, {1,1,1}));
        conv1_ = register_module("conv1", create_conv(16, 16, {3,3,3}, {1,1,1}));
        conv2_ = register_module("conv2", create_conv(16, 16, {3,3,3}, {1,1,1}));
        conv3_ = register_module("conv3", create_conv(16, 16, {3,3,3}, {1,1,1}));
        conv4_ = register_module("conv4", create_conv(16, 16, {3,3,3}, {1,1,1}));

        // Block 2 (subm=False, stride=2)
        conv5_ = register_module("conv5", create_conv(16, 32, {3,3,3}, {1,1,1}, false));
        // 根据ONNX设置stride=2
        conv5_->set_stride({2,2,2});
        conv6_ = register_module("conv6", create_conv(32, 32, {3,3,3}, {1,1,1}));
        conv7_ = register_module("conv7", create_conv(32, 32, {3,3,3}, {1,1,1}));
        conv8_ = register_module("conv8", create_conv(32, 32, {3,3,3}, {1,1,1}));
        conv9_ = register_module("conv9", create_conv(32, 32, {3,3,3}, {1,1,1}));

        // 第三个block: 64通道
        conv10_ = register_module("conv10", create_conv(32, 64, {3,3,3}, {1,1,1}));
        conv11_ = register_module("conv11", create_conv(64, 64, {3,3,3}, {1,1,1}));
        conv12_ = register_module("conv12", create_conv(64, 64, {3,3,3}, {1,1,1}));
        conv13_ = register_module("conv13", create_conv(64, 64, {3,3,3}, {1,1,1}));
        conv14_ = register_module("conv14", create_conv(64, 64, {3,3,3}, {1,1,1}));

        // 第四个block: 128通道
        conv15_ = register_module("conv15", create_conv(64, 128, {3,3,3}, {1,1,1}));
        conv16_ = register_module("conv16", create_conv(128, 128, {3,3,3}, {1,1,1}));
        conv17_ = register_module("conv17", create_conv(128, 128, {3,3,3}, {1,1,1}));
        conv18_ = register_module("conv18", create_conv(128, 128, {3,3,3}, {1,1,1}));
        conv19_ = register_module("conv19", create_conv(128, 128, {3,3,3}, {1,1,1}));

        // 最后的1x1x3卷积
        conv20_ = register_module("conv20", create_conv(128, 128, {1,1,3}, {0,0,0}, false));
        // 根据ONNX设置stride=2
        conv20_->set_stride({1,1,2});
    }

    torch::Tensor forward(torch::Tensor indices, 
                         torch::Tensor values,
                         c10::ArrayRef<int64_t> spatial_size) {
        // 初始空间尺寸 [1440, 1440, 41]
        std::vector<int64_t> curr_size = {1440, 1440, 41};
        
        // Block 1
        auto x1 = conv0_->forward(indices, values, curr_size);
        auto x2 = conv1_->forward(x1.indices(), x1.values(), curr_size);
        auto x3 = conv2_->forward(x2.indices(), x2.values(), curr_size);
        x3 = x3 + x1;  // residual connection
        x3 = torch::relu(x3);
        
        auto x4 = conv3_->forward(x3.indices(), x3.values(), curr_size);
        auto x5 = conv4_->forward(x4.indices(), x4.values(), curr_size);
        x5 = x5 + x3;  // residual connection
        x5 = torch::relu(x5);
        std::cout << "Block 1 output shape: " << x5.sizes() << std::endl;

        // Block 2 - 调整padding保持尺寸匹配
        curr_size = {720, 720, 21};
        auto x6 = conv5_->forward(x5.indices(), x5.values(), curr_size);
        auto x7 = conv6_->forward(x6.indices(), x6.values(), curr_size);
        auto x8 = conv7_->forward(x7.indices(), x7.values(), curr_size);
        
        // 添加尺寸调整操作
        if (x8.sizes()[0] != x6.sizes()[0]) {
            x6 = adjust_spatial_size(x6, x8.sizes().slice(0,3));
        }
        x8 = x8 + x6;  // residual connection
        x8 = torch::relu(x8);

        auto x9 = conv8_->forward(x8.indices(), x8.values(), curr_size);
        auto x10 = conv9_->forward(x9.indices(), x9.values(), curr_size);
        x10 = x10 + x8;  // residual connection
        x10 = torch::relu(x10);
        std::cout << "Block 2 output shape: " << x10.sizes() << std::endl;

        // Block 3
        curr_size = {360, 360, 11};
        auto x11 = conv10_->forward(x10.indices(), x10.values(), curr_size);
        auto x12 = conv11_->forward(x11.indices(), x11.values(), curr_size);
        auto x13 = conv12_->forward(x12.indices(), x12.values(), curr_size);
        x13 = x13 + x11;  // residual connection
        x13 = torch::relu(x13);

        auto x14 = conv13_->forward(x13.indices(), x13.values(), curr_size);
        auto x15 = conv14_->forward(x14.indices(), x14.values(), curr_size);
        x15 = x15 + x13;  // residual connection
        x15 = torch::relu(x15);
        std::cout << "Block 3 output shape: " << x15.sizes() << std::endl;

        // Block 4
        curr_size = {180, 180, 5};
        auto x16 = conv15_->forward(x15.indices(), x15.values(), curr_size);
        auto x17 = conv16_->forward(x16.indices(), x16.values(), curr_size);
        auto x18 = conv17_->forward(x17.indices(), x17.values(), curr_size);
        x18 = x18 + x16;  // residual connection
        x18 = torch::relu(x18);

        auto x19 = conv18_->forward(x18.indices(), x18.values(), curr_size);
        auto x20 = conv19_->forward(x19.indices(), x19.values(), curr_size);
        x20 = x20 + x18;  // residual connection
        x20 = torch::relu(x20);
        std::cout << "Block 4 output shape: " << x20.sizes() << std::endl;

        // Final 1x1 conv
        auto x21 = conv20_->forward(x20.indices(), x20.values(), curr_size);
        std::cout << "Block 5 output shape: " << x21.sizes() << std::endl;

        // 最终输出处理
        auto dense_output = sparse_to_dense(x21, {180, 180, 2});
        std::cout << "dense_output output shape: " << dense_output.sizes() << std::endl;
        auto output = dense_output.permute({0, 4, 1, 2, 3}); // [1, C, D, H, W]
        std::cout << "output output shape: " << output.sizes() << std::endl;
        output = output.reshape({1, 256, 180, 180}); // 最终reshape到目标形状
        std::cout << "output_reshape output shape: " << output.sizes() << std::endl;

        return output;
    }

private:
    SubMConv3d conv0_{nullptr}, conv1_{nullptr}, conv2_{nullptr}, conv3_{nullptr}, conv4_{nullptr};
    SubMConv3d conv5_{nullptr}, conv6_{nullptr}, conv7_{nullptr}, conv8_{nullptr}, conv9_{nullptr};
    SubMConv3d conv10_{nullptr}, conv11_{nullptr}, conv12_{nullptr}, conv13_{nullptr}, conv14_{nullptr};
    SubMConv3d conv15_{nullptr}, conv16_{nullptr}, conv17_{nullptr}, conv18_{nullptr}, conv19_{nullptr};
    SubMConv3d conv20_{nullptr};

    // Helper function to convert sparse tensor to dense
    torch::Tensor sparse_to_dense(const torch::Tensor& sparse_tensor, 
                                c10::ArrayRef<int64_t> spatial_size) {
        auto indices = sparse_tensor.indices();
        auto values = sparse_tensor.values();
        
        // 创建5维输出张量 [1, D, H, W, C]
        auto output_shape = std::vector<int64_t>{1}; // batch dimension
        output_shape.insert(output_shape.end(), 
                          spatial_size.begin(), spatial_size.end());
        output_shape.push_back(values.size(1)); // channels
        
        auto dense = torch::zeros(output_shape);
        
        for (int64_t i = 0; i < indices.size(1); ++i) {
            auto z = indices[0][i].item<int64_t>();
            auto y = indices[1][i].item<int64_t>();
            auto x = indices[2][i].item<int64_t>();
            dense.index({0, z, y, x}) = values[i];
        }
        return dense;
    }

    // 修改 adjust_spatial_size 函数
    torch::Tensor adjust_spatial_size(const torch::Tensor& input, 
                                    c10::ArrayRef<int64_t> target_size) {
        auto indices = input.indices();
        auto values = input.values();
        
        auto mask = (indices[0] < target_size[0]) & 
                   (indices[1] < target_size[1]) & 
                   (indices[2] < target_size[2]);
        
        auto filtered_indices = indices.masked_select(mask).view({3, -1});
        auto filtered_values = values.masked_select(
            mask.unsqueeze(1).expand({-1, values.size(1)})).view({-1, values.size(1)});
        
        // 创建4维稀疏张量 [D, H, W, C]
        std::vector<int64_t> output_shape = {
            target_size[0], target_size[1], target_size[2], values.size(1)
        };
        
        return torch::sparse_coo_tensor(
            filtered_indices,
            filtered_values,
            output_shape,
            torch::kFloat
        ).coalesce();
    }
};

TORCH_MODULE(LidarBackbone);

float* lidar_backbone();