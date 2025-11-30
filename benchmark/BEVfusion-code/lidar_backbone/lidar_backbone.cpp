#include "lidar_backbone.h"
#include "pipe_comm.h"
#include "apis_c.h"

InterChiplet::PipeComm global_pipe_comm;

float* lidar_backbone(float* input){
    // 创建输入 - 使用正确的空间尺寸
    auto indices = torch::tensor({
        {0, 0, 0},      // batch index
        {720, 720, 720}, // z
        {720, 720, 720}, // y
        {20, 20, 20}     // x
    }, torch::kLong);
    
    auto values = torch::from_blob(input, {3, 5}); // 5个输入通道
    std::vector<int64_t> spatial_size = {1440, 1440, 41}; // 初始空间尺寸

    // 创建模型
    auto model = LidarBackbone();
    
    // 前向传播
    auto output = model->forward(indices, values, spatial_size);
    
    // 验证输出形状是否符合预期 [1, 256, 180, 180]
    std::cout << "Output shape: " << output.sizes() << std::endl;

    // 将output转换为float*
    float* output_ptr = output.data_ptr<float>();
    float* output_ptr_copy = (float*)malloc(1 * 256 * 180 * 180 * sizeof(float));
    memcpy(output_ptr_copy, output_ptr, 1 * 256 * 180 * 180 * sizeof(float));
    
    return output_ptr_copy;
}

int main(int argc, char** argv) {
    int idX = atoi(argv[1]);
    int idY = atoi(argv[2]);
    float* input = new float[3 * 5];
    long long unsigned int timeNow = 1;
    std::string fileName = InterChiplet::receiveSync(3, 3, idX, idY);
    global_pipe_comm.read_data(fileName.c_str(), input, 3 * 5 * sizeof(float));
    long long int time_end = InterChiplet::readSync(timeNow, 3, 3, idX, idY, 3 * 5 * sizeof(float), 0);
    std::cout<<"--------------------------------"<<std::endl;  
    float* lidar_backbone_output = lidar_backbone(input);
    fileName = InterChiplet::sendSync(idX, idY, 3, 3);
    global_pipe_comm.write_data(fileName.c_str(), lidar_backbone_output, 1 * 256 * 180 * 180 * sizeof(float));
    time_end = InterChiplet::writeSync(time_end, idX, idY, 3, 3, 1 * 256 * 180 * 180 * sizeof(float), 0);
    return 0;
}
