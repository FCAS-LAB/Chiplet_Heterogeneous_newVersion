#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <chrono>

#include "apis_c.h"

int main(int argc, char** argv) {
    int idX = atoi(argv[1]);
    int idY = atoi(argv[2]);
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
            // camera_backbone(img, depth,camera_features);
            InterChiplet::sendMessage(0, 0, idX, idY, img, 1 * 6 * 3 * 256 * 704 * sizeof(float));
            std::cout<<"--------------------------------"<<std::endl;
            InterChiplet::sendMessage(0, 0, idX, idY, depth, 1 * 6 * 1 * 256 * 704 * sizeof(float));

            InterChiplet::receiveMessage(idX, idY, 0, 0, camera_features, 6*32*88*80 * sizeof(float));
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
        float* camera_bev_features = new float[1*80*180*180];
        {
            std::cout << "处理相机视角变换 (6×32×88×80 -> 1×80×180×180)..." << std::endl;
            
            // 调用相机视角变换模块
            // camera_bev_features = camera_vtransform();
            InterChiplet::sendMessage(0, 1, idX, idY, camera_features,  6 * 32 * 88 * 80 * sizeof(float));
            InterChiplet::receiveMessage(idX, idY, 0, 1, camera_bev_features, 1*80*180*180 * sizeof(float));
            
            if (!camera_bev_features) {
                std::cerr << "相机视角变换失败!" << std::endl;
                return 1;
            }
        }

        // 3. LiDAR骨干网络 (LiDAR Backbone)
        float* lidar_features = new float[1*256*180*180];
        float* input = new float[3 * 5];
        for(int i = 0; i < 3 * 5; i++){
            input[i] = 1.0f;
        }
        {
            std::cout << " 处理LiDAR骨干网络 (1×5 -> 1×256×180×180)..." << std::endl;
            
            // 调用LiDAR骨干网络
            // lidar_features = lidar_backbone();
            InterChiplet::sendMessage(0, 2, idX, idY, input, 3 * 5 * sizeof(float));
            InterChiplet::receiveMessage(idX, idY, 0, 2, lidar_features, 1*256*180*180 * sizeof(float));
            
            if (!lidar_features) {
                std::cerr << "LiDAR骨干网络处理失败!" << std::endl;
                return 1;
            }
        }

        // 4. 特征融合 (Fuser)
        float *fused_features = new float[1*512*180*180];
        {
            std::cout << "处理特征融合 (1×80×180×180 + 1×256×180×180 -> 1×512×180×180)..." << std::endl;
            
            // fused_features = fuser(camera_bev_features, lidar_features);
            InterChiplet::sendMessage(0, 3, idX, idY, camera_bev_features, 1*80*180*180 * sizeof(float));
            InterChiplet::sendMessage(0, 3, idX, idY, lidar_features, 1*256*180*180 * sizeof(float));
            InterChiplet::receiveMessage(idX, idY, 0, 3, fused_features, 1*512*180*180 * sizeof(float));
        }

        // 5. 检测头 (Head)
        {
            std::cout << "处理检测头 (1×512×180×180 -> 多个输出)..." << std::endl;
            bool* finished = new bool[1];
            // 调用检测头
            // head(fused_features);
            InterChiplet::sendMessage(1, 3, idX, idY, fused_features, 1*512*180*180 * sizeof(float));
            InterChiplet::receiveMessage(idX, idY, 1, 3, finished, 1 * sizeof(bool));
            if(finished[0]){
                std::cout << "检测头处理完成!" << std::endl;
            }
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