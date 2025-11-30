#include <torch/torch.h>
#include "camera_backbone.h"
#include "pipe_comm.h"
#include "apis_c.h"
InterChiplet::PipeComm global_pipe_comm;


// 定义ResNet-50的Bottleneck模块
struct BottleneckImpl : torch::nn::Module {
    torch::nn::Conv2d conv1{nullptr}, conv2{nullptr}, conv3{nullptr};
    torch::nn::BatchNorm2d bn1{nullptr}, bn2{nullptr}, bn3{nullptr};
    torch::nn::Sequential downsample{nullptr}; // 初始化为nullptr

    BottleneckImpl(int64_t inplanes, int64_t planes, int64_t stride = 1) {
        // 使用register_module注册子模块
        conv1 = register_module("conv1", torch::nn::Conv2d(
            torch::nn::Conv2dOptions(inplanes, planes, 1).stride(1).bias(false)
        ));
        bn1 = register_module("bn1", torch::nn::BatchNorm2d(planes));
        
        conv2 = register_module("conv2", torch::nn::Conv2d(
            torch::nn::Conv2dOptions(planes, planes, 3).stride(stride).padding(1).bias(false)
        ));
        bn2 = register_module("bn2", torch::nn::BatchNorm2d(planes));
        
        conv3 = register_module("conv3", torch::nn::Conv2d(
            torch::nn::Conv2dOptions(planes, planes * 4, 1).stride(1).bias(false)
        ));
        bn3 = register_module("bn3", torch::nn::BatchNorm2d(planes * 4));

        // 下采样模块
        if (stride != 1 || inplanes != planes * 4) {
            downsample = register_module("downsample", torch::nn::Sequential(
                torch::nn::Conv2d(torch::nn::Conv2dOptions(inplanes, planes * 4, 1).stride(stride).bias(false)),
                torch::nn::BatchNorm2d(planes * 4)
            ));
        }
    }

    torch::Tensor forward(torch::Tensor x) {
        auto identity = x.clone();

        x = conv1->forward(x);
        x = bn1->forward(x);
        x = torch::relu(x);

        x = conv2->forward(x);
        x = bn2->forward(x);
        x = torch::relu(x);

        x = conv3->forward(x);
        x = bn3->forward(x);

        if (!downsample.is_empty()) {
            identity = downsample->forward(identity);
        }

        x += identity;
        return torch::relu(x);
    }
};
TORCH_MODULE(Bottleneck);

// 定义ResNet-50主干网络
struct ResNet50Impl : torch::nn::Module {
    torch::nn::Conv2d conv1{nullptr};
    torch::nn::BatchNorm2d bn1{nullptr};
    torch::nn::Sequential layer1{nullptr}, layer2{nullptr}, layer3{nullptr}, layer4{nullptr};


    ResNet50Impl() {
        // 使用register_module显式注册
        conv1 = register_module("conv1", torch::nn::Conv2d(
            torch::nn::Conv2dOptions(3, 64, 7).stride(2).padding(3).bias(false)
        ));
        bn1 = register_module("bn1", torch::nn::BatchNorm2d(64));

        // 构建残差层
        layer1 = register_module("layer1", _make_layer(64, 64, 3, 1));
        layer2 = register_module("layer2", _make_layer(256, 128, 4, 2));
        layer3 = register_module("layer3", _make_layer(512, 256, 6, 2));
        layer4 = register_module("layer4", _make_layer(1024, 512, 3, 2));
    }

    torch::nn::Sequential _make_layer(int64_t inplanes, int64_t planes, int64_t blocks, int64_t stride) {
        torch::nn::Sequential layers;
        layers->push_back(Bottleneck(inplanes, planes, stride));
        for (int i = 1; i < blocks; i++) {
            layers->push_back(Bottleneck(planes * 4, planes));
        }
        return layers;
    }

    // 返回各阶段特征：[C2, C3, C4, C5]
    std::vector<torch::Tensor> forward(torch::Tensor x) {
        x = conv1->forward(x);
        x = bn1->forward(x);
        x = torch::relu(x);
        x = torch::max_pool2d(x, 3, 2, 1);  // [B, 64, 64, 64]（假设输入256x256）

        auto c2 = layer1->forward(x);       // [B, 256, 64, 64]
        auto c3 = layer2->forward(c2);      // [B, 512, 32, 32]
        auto c4 = layer3->forward(c3);      // [B, 1024, 16, 16]
        auto c5 = layer4->forward(c4);      // [B, 2048, 8, 8]

        return {c2, c3, c4, c5};
    }
};
TORCH_MODULE(ResNet50);

#include <torch/torch.h>

// 1. 修复ADP模块
struct ADPImpl : torch::nn::Module {
    torch::nn::Linear fc1{nullptr}, fc2{nullptr};
    int64_t channels;

    // 显式定义构造函数参数
    ADPImpl(int64_t channels, int64_t reduction_ratio = 16) : channels(channels) {
        int64_t reduced_channels = channels / reduction_ratio;
        fc1 = register_module("fc1", torch::nn::Linear(channels, reduced_channels));
        fc2 = register_module("fc2", torch::nn::Linear(reduced_channels, channels));
    }

    torch::Tensor forward(torch::Tensor x) {
        auto batch_size = x.size(0);
        auto squeeze = torch::adaptive_avg_pool2d(x, {1, 1}).view({batch_size, channels});
        auto excitation = torch::relu(fc1->forward(squeeze));
        excitation = torch::sigmoid(fc2->forward(excitation)).view({batch_size, channels, 1, 1});
        return x * excitation;
    }
};
TORCH_MODULE(ADP);  // 使用TORCH_MODULE宏定义ADP模块

// 2. 修复FPN模块
struct FPNWithADPImpl : torch::nn::Module {
    std::vector<torch::nn::Sequential> lateral_convs{};
    std::vector<torch::nn::Sequential> fpn_convs{};
    std::vector<ADP> adp_modules{};

    FPNWithADPImpl(std::vector<int64_t> in_channels_list, int64_t out_channels = 256) {
        for (auto in_channels : in_channels_list) {
            // 侧边卷积
            auto lateral_conv = register_module(
                "lateral_conv_" + std::to_string(in_channels),
                torch::nn::Sequential(
                    torch::nn::Conv2d(torch::nn::Conv2dOptions(in_channels, out_channels, 1).bias(false)),
                    torch::nn::BatchNorm2d(out_channels)
                )
            );
            lateral_convs.push_back(lateral_conv);

            // FPN卷积
            auto fpn_conv = register_module(
                "fpn_conv_" + std::to_string(in_channels),
                torch::nn::Sequential(
                    torch::nn::Conv2d(torch::nn::Conv2dOptions(out_channels, out_channels, 3).padding(1).bias(false)),
                    torch::nn::BatchNorm2d(out_channels),
                    torch::nn::ReLU()
                )
            );
            fpn_convs.push_back(fpn_conv);

            // 修复ADP模块初始化
            adp_modules.push_back(register_module(
                "adp_" + std::to_string(in_channels),
                ADP(out_channels, 16)  // 显式指定reduction_ratio
            ));
        }
    }

    std::vector<torch::Tensor> forward(std::vector<torch::Tensor> features) {
        std::vector<torch::Tensor> fpn_features;
        torch::Tensor prev_feature;

        // 自顶向下融合
        for (int i = features.size() - 1; i >= 0; i--) {
            auto lateral_feature = lateral_convs[i]->forward(features[i]);
            if (i != features.size() - 1) {
                // 修复上采样参数
                prev_feature = torch::upsample_bilinear2d(
                    prev_feature,
                    {lateral_feature.size(2), lateral_feature.size(3)},
                    /*align_corners=*/true
                );
                lateral_feature += prev_feature;
            }
            // 修复ADP调用方式
            prev_feature = adp_modules[i]->forward(lateral_feature);  // 直接调用forward
            prev_feature = fpn_convs[i]->forward(prev_feature);
            fpn_features.insert(fpn_features.begin(), prev_feature);
        }

        return fpn_features;
    }
};
TORCH_MODULE(FPNWithADP);  // 使用TORCH_MODULE宏定义FPNWithADP模块

// 2D-3D投影器（假设使用深度分布）
struct ProjectorImpl : torch::nn::Module {
    int64_t bev_height;  // BEV网格高度
    int64_t bev_width;   // BEV网格宽度
    int64_t out_channels; // 输出特征通道数
    int64_t in_channels;  // 输入特征通道数

    ProjectorImpl(int64_t bev_height, int64_t bev_width, int64_t out_channels, int64_t in_channels) 
        : bev_height(bev_height), bev_width(bev_width), out_channels(out_channels), in_channels(in_channels) {}

    torch::Tensor forward(torch::Tensor features, torch::Tensor depth_probs) {
        auto batch_size = features.size(0); // 应该是6 (B*num_cameras)
        auto device = features.device();

        // 1. 生成采样网格
        auto bev_x = torch::linspace(-1, 1, bev_width, torch::TensorOptions().device(device));
        auto bev_y = torch::linspace(-1, 1, bev_height, torch::TensorOptions().device(device));
        
        // 使用meshgrid生成采样点，注意使用"ij"模式
        auto grid = torch::meshgrid({bev_y, bev_x}, /*indexing=*/"ij");
        auto sample_grid = torch::stack({grid[1], grid[0]}, -1); // [H, W, 2]
        
        // 关键修改：扩展网格以匹配batch_size
        sample_grid = sample_grid.unsqueeze(0)  // 添加batch维度 [1, H, W, 2]
                                .expand({batch_size, -1, -1, -1}); // [B, H, W, 2]

        // 2. 使用grid_sample进行投影
        auto weighted_features = features * depth_probs; // [B, C, H, W]
        
        auto sampled_features = torch::nn::functional::grid_sample(
            weighted_features,  // [B, C, H, W]
            sample_grid,       // [B, H, W, 2]
            torch::nn::functional::GridSampleFuncOptions()
                .align_corners(true)
                .mode(torch::kBilinear)
        ); // [B, C, H, W]

        // 3. 调整通道数
        auto conv_weight = register_parameter(
            "conv_weight", 
            torch::randn({out_channels, in_channels, 1, 1})
        );
        
        auto output = torch::nn::functional::conv2d(
            sampled_features,
            conv_weight,
            torch::nn::functional::Conv2dFuncOptions().stride(1)
        );

        return output;
    }
};
TORCH_MODULE(Projector);

struct BEVEncoderImpl : torch::nn::Module {
    torch::nn::Sequential encoder{nullptr};

    BEVEncoderImpl(int64_t in_channels, int64_t out_channels = 256) {
        encoder = register_module("encoder", torch::nn::Sequential(
            torch::nn::Conv3d(torch::nn::Conv3dOptions(in_channels, out_channels, {1, 3, 3}).padding({0, 1, 1})),
            torch::nn::BatchNorm3d(out_channels),
            torch::nn::ReLU(),
            torch::nn::MaxPool3d(torch::nn::MaxPool3dOptions({1, 1, 1})),
            // 修复squeeze操作：使用lambda包装并指定dim参数
            torch::nn::Functional([](torch::Tensor x) {
                return torch::squeeze(x, 2);  // 明确指定dim=2
            }),
            torch::nn::Conv2d(torch::nn::Conv2dOptions(out_channels, out_channels, 3).padding(1)),
            torch::nn::BatchNorm2d(out_channels),
            torch::nn::ReLU()
        ));
    }

    torch::Tensor forward(torch::Tensor bev_feature) {
        // 输入: (B, C, H, W) -> 添加伪深度维度 (B, C, 1, H, W)
        auto x = bev_feature.unsqueeze(2); 
        x = encoder->forward(x); // 输出 (B, out_channels, H, W)
        return x;
    }
};
TORCH_MODULE(BEVEncoder);

struct CameraStreamImpl : torch::nn::Module {
    ResNet50 resnet{nullptr};
    FPNWithADP fpn{nullptr};
    Projector projector{nullptr};
    BEVEncoder bev_encoder{nullptr};
    
    // 新增成员变量
    const int num_cameras = 6;       // 对应输入中的6个摄像头
    const std::vector<int64_t> img_shape = {256, 704}; // 输入图像尺寸
    const int in_channels = 3;       // 输入通道数
    const int out_channels = 32;     // 匹配ONNX输出通道
    const int64_t bev_height = 88;   // BEV特征图高度
    const int64_t bev_width = 80;    // BEV特征图宽度

    CameraStreamImpl() {
        // 调整输入通道数匹配ONNX的3通道输入
        resnet = register_module("resnet", ResNet50());
        
        // 修改FPN输入通道列表匹配ResNet各阶段输出
        fpn = register_module("fpn", FPNWithADP(std::vector<int64_t>{256, 512, 1024, 2048}));
        
        // 调整Projector参数匹配实际输出
        projector = register_module("projector", 
            Projector(
                bev_height,  // 88
                bev_width,   // 80
                out_channels,  // 32
                256  // 输入通道
            ));
        
        // 修改BEV编码器输出通道
        bev_encoder = register_module("bev_encoder", 
            BEVEncoder(/*in_channels*/256, /*out_channels*/out_channels));
    }

    std::pair<torch::Tensor, torch::Tensor> forward(
        torch::Tensor img,      // [B, 6, 3, 256, 704]
        torch::Tensor depth     // [B, 6, 1, 256, 704]
    ) {
        auto batch_size = img.size(0);
        img = img.view({batch_size * num_cameras, in_channels, img_shape[0], img_shape[1]});
        depth = depth.view({batch_size * num_cameras, 1, img_shape[0], img_shape[1]});

        // 1. 特征提取
        auto features = resnet->forward(img); // [B*6, 2048, 8, 22]
        
        // 2. FPN特征融合
        auto fpn_features = fpn->forward(features);
        auto c5_feature = fpn_features.back(); // [B*6, 256, 8, 22]

        // 3. 深度处理
        auto depth_downsampled = torch::nn::functional::interpolate(
            depth,
            torch::nn::functional::InterpolateFuncOptions()
                .size(std::vector<int64_t>{8, 22})
                .mode(torch::kBilinear)
                .align_corners(true)
        ); // [B*6, 1, 8, 22]

        auto depth_weights = torch::sigmoid(depth_downsampled);
        auto weighted_feature = c5_feature * depth_weights;

        // 4. 投影到BEV空间
        auto bev_feature = projector->forward(
            weighted_feature,    // [B*6, 256, 8, 22]
            depth_weights       // [B*6, 1, 8, 22]
        ); // [B*6, 32, 88, 80]

        // 调试输出
        std::cout << "Input feature shape: " << weighted_feature.sizes() << std::endl;
        std::cout << "Input depth shape: " << depth_weights.sizes() << std::endl;
        std::cout << "Projector output shape: " << bev_feature.sizes() << std::endl;

        // 5. 调整输出形状以匹配ONNX
        // camera_feature: [6, 32, 88, 80]
        bev_feature = bev_feature.view({num_cameras, out_channels, bev_height, bev_width});

        // camera_depth_weights: [6, 118, 32, 88]
        auto depth_weights_out = depth_weights
            .view({num_cameras, 1, 8, 22})  // 移除batch维度
            .expand({-1, 118, -1, -1})      // 扩展到118通道
            .permute({0, 1, 2, 3});         // 调整维度顺序

        // 将深度权重调整为正确的形状 [6, 118, 32, 88]
        depth_weights_out = torch::nn::functional::interpolate(
            depth_weights_out,
            torch::nn::functional::InterpolateFuncOptions()
                .size(std::vector<int64_t>{32, 88})
                .mode(torch::kBilinear)
                .align_corners(true)
        );

        // std::cout << "Final feature shape: " << bev_feature.sizes() << std::endl;
        // std::cout << "Final depth weights shape: " << depth_weights_out.sizes() << std::endl;

        return {bev_feature, depth_weights_out};
    }
};
TORCH_MODULE(CameraStream);

void camera_backbone(float* img, float* depth, float* camera_features){
    auto img_tensor = torch::from_blob(img, {1, 6, 3, 256, 704}, torch::kFloat);
    auto depth_tensor = torch::from_blob(depth, {1, 6, 1, 256, 704}, torch::kFloat);

    CameraStream model;
    auto [feature, depth_weights] = model->forward(img_tensor, depth_tensor);
    std::cout << "Final feature shape: " << feature.sizes() << std::endl;
    std::cout<< "feature: " << feature[5][31][87][79] << std::endl;
    // 将feature转换为float*
    float* feature_ptr = feature.data_ptr<float>();
    std::cout << "feature_ptr: " << feature_ptr[6*32*88*80-1] << std::endl;
    memcpy(camera_features, feature_ptr, 6*32*88*80*sizeof(float));
}

int main(int argc, char** argv) { // (0,0)
    int idX = atoi(argv[1]);
    int idY = atoi(argv[2]);
    float* img = new float[1 * 6 * 3 * 256 * 704];
    float* depth = new float[1 * 6 * 1 * 256 * 704];
    float* camera_backbone_output = new float[6*32*88*80];
    long long unsigned int timeNow = 1;
    std::string fileName = InterChiplet::receiveSync(3, 3, idX, idY);
    global_pipe_comm.read_data(fileName.c_str(), img, 6 * 3 * 256 * 704 * sizeof(float));
    long long int time_end = InterChiplet::readSync(timeNow, 3, 3, idX, idY, 6 * 3 * 256 * 704 * sizeof(float), 0);
    std::cout<<"--------------------------------"<<std::endl;
    fileName = InterChiplet::receiveSync(3, 3, idX, idY);
    global_pipe_comm.read_data(fileName.c_str(), depth, 6 * 1 * 256 * 704 * sizeof(float));
    time_end = InterChiplet::readSync(timeNow, 3, 3, idX, idY, 6 * 1 * 256 * 704 * sizeof(float), 0);
    std::cout<<"--------------------------------"<<std::endl;
    camera_backbone(img, depth, camera_backbone_output);
    fileName = InterChiplet::sendSync(idX, idY, 3, 3);
    global_pipe_comm.write_data(fileName.c_str(), camera_backbone_output, 6 * 32 * 88 * 80 * sizeof(float));
    time_end = InterChiplet::writeSync(time_end, idX, idY, 3, 3, 6 * 32 * 88 * 80 * sizeof(float), 0);
    std::cout<<"--------------------------------"<<std::endl;
    return 0;
}

// int main() {
//     // 输入：2张256x256的RGB图像
//     auto input = torch::randn({2, 3, 256, 256}).to(torch::kCUDA);

//     // 初始化ResNet和FPN
//     ResNet50 resnet;
//     resnet->to(torch::kCUDA);
//     FPNWithADP fpn(std::vector<int64_t>{256, 512, 1024, 2048});
//     fpn->to(torch::kCUDA);

//     // 前向传播
//     auto resnet_features = resnet->forward(input);
//     auto fpn_features = fpn->forward(resnet_features);

//     // 打印ResNet输出形状
//     std::cout << "=== ResNet Features ===" << std::endl;
//     std::vector<std::string> layer_names = {"C2", "C3", "C4", "C5"};
//     for (size_t i = 0; i < resnet_features.size(); i++) {
//         std::cout << layer_names[i] << " shape: " << resnet_features[i].sizes() << std::endl;
//     }

//     // 打印FPN输出形状
//     std::cout << "\n=== FPN Features ===" << std::endl;
//     for (size_t i = 0; i < fpn_features.size(); i++) {
//         std::cout << "FPN Layer " << i + 1 << " shape: " << fpn_features[i].sizes() << std::endl;
//     }

//     return 0;
// }