#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <chrono>
// #include <torch/torch.h>

// 包含各个模块的头文件
#include "camera_backbone/camera_backbone.h"
#include "camera_vtransform/camera_vtransform.h"
#include "lidar_backbone/lidar_backbone.h"
#include "fuser/fuser.h"
#include "head/head.h"


int main() {
    std::cout << "启动 BEVfusion 推理流程..." << std::endl;

    // try {
        // 1. 相机骨干网络 (Camera Backbone)
        float* camera_features = new float[6*32*88*80];
        {
            std::cout << "处理相机骨干网络 (1×6×3×256×704 -> 6×32×88×80)..." << std::endl;
            
            // 创建随机输入数据
            float* img = new float[1 * 6 * 3 * 256 * 704];
            float* depth = new float[1 * 6 * 1 * 256 * 704];
            
            // 初始化输入数据
            for (int i = 0; i < 1 * 6 * 3 * 256 * 704; ++i) {
                img[i] = 0.1f;  // 示例数据
            }
            
            for (int i = 0; i < 1 * 6 * 1 * 256 * 704; ++i) {
                depth[i] = 0.2f;  // 示例数据
            }
            
            // 调用相机骨干网络
            camera_backbone(img, depth,camera_features);
            // std::cout << "camera_features: " << camera_features[6*32*88] << std::endl;
            
            if (!camera_features) {
                std::cerr << "相机骨干网络处理失败!" << std::endl;
                return 1;
            }
            
            // 清理输入数据
            delete[] img;
            delete[] depth;
        }

        // 2. 相机视角变换 (Camera VTransform)
        float* camera_bev_features = nullptr;
        {
            std::cout << "处理相机视角变换 (6×32×88×80 -> 1×80×180×180)..." << std::endl;
            float* camera_features_tmp = new float[1*80*360*360];
            for(int i = 0; i < 1*80*360*360; i++){
                camera_features_tmp[i] = 1.0f;
            }
            memcpy(camera_features_tmp, camera_features, 6*32*88*80*sizeof(float));
            
            // 调用相机视角变换模块
            camera_bev_features = camera_vtransform();
            
            if (!camera_bev_features) {
                std::cerr << "相机视角变换失败!" << std::endl;
                return 1;
            }
            delete[] camera_features_tmp;
        }

        // 3. LiDAR骨干网络 (LiDAR Backbone)
        float* lidar_features = nullptr;
        {
            std::cout << " 处理LiDAR骨干网络 (1×5 -> 1×256×180×180)..." << std::endl;
            
            // 调用LiDAR骨干网络
            lidar_features = lidar_backbone();
            
            if (!lidar_features) {
                std::cerr << "LiDAR骨干网络处理失败!" << std::endl;
                return 1;
            }
        }

        // 4. 特征融合 (Fuser)
        float *fused_features = nullptr;
        {
            std::cout << "处理特征融合 (1×80×180×180 + 1×256×180×180 -> 1×512×180×180)..." << std::endl;
            
            fused_features = fuser(camera_bev_features, lidar_features);
        }

        // 5. 检测头 (Head)
        {
            std::cout << "处理检测头 (1×512×180×180 -> 多个输出)..." << std::endl;
            
            // 调用检测头
            head(fused_features);
        }

        // 释放内存
        if (camera_features) delete[] camera_features;
        if (camera_bev_features) delete[] camera_bev_features;
        if (fused_features) delete[] fused_features;
        if (lidar_features) delete[] lidar_features;
        

        std::cout << " BEVfusion 推理完成!" << std::endl;
        
    // } catch (const std::exception& e) {
    //     std::cerr << "❌ 错误: " << e.what() << std::endl;
    //     return 1;
    // }
    
    return 0;
}