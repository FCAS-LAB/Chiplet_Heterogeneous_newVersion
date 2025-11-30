#include <iostream>
#include <cstdint>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <cassert>
#include <cstdio>
#include <stdlib.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <string>
#include "apis_c.h"
#include "../../interchiplet/includes/pipe_comm.h"
InterChiplet::PipeComm global_pipe_comm;

void resize(int64_t* src, int originalWidth, int originalHeight, int channels, int64_t* dest, int newWidth, int newHeight) {
    // 计算缩放比例
    double scaleX = static_cast<double>(originalWidth) / newWidth;
    double scaleY = static_cast<double>(originalHeight) / newHeight;

    // 遍历目标图像的每个像素
    for (int c = 0; c < channels; ++c) {
        for (int newY = 0; newY < newHeight; ++newY) {
            for (int newX = 0; newX < newWidth; ++newX) {
                // 计算在原始图像中的对应位置
                int srcX = static_cast<int>(newX * scaleX);
                int srcY = static_cast<int>(newY * scaleY);

                // 将原始图像对应位置的像素值赋给目标图像
                dest[(c * newHeight + newY) * newWidth + newX] = src[(c * originalHeight + srcY) * originalWidth + srcX];
            }
        }
    }
}

int main(int argc, char** argv) {
    int idX = atoi(argv[1]);
    int idY = atoi(argv[2]);
    // 假设原始图像大小为 112x112，通道数为 64
    const int originalWidth = 112;
    const int originalHeight = 112;
    const int channels = 64;
    int64_t* M = new int64_t[originalWidth * originalHeight * channels];

    // 接收原始图像数据
    long long unsigned int timeNow = 1;
    std::string fileName = InterChiplet::receiveSync(5, 5, idX, idY);
    global_pipe_comm.read_data(fileName.c_str(), M, originalWidth * originalHeight * channels * sizeof(int64_t));
    long long int time_end = InterChiplet::readSync(timeNow, 5, 5, idX, idY, originalWidth * originalHeight * channels * sizeof(int64_t), 0);

    // 设置新的图像大小为 56x56，通道数不变
    const int newWidth = 56;
    const int newHeight = 56;
    int64_t* resizedM = new int64_t[newWidth * newHeight * channels];

    // 调用缩放函数
    resize(M, originalWidth, originalHeight, channels, resizedM, newWidth, newHeight);

    // 发送缩放后的图像数据
    fileName = InterChiplet::sendSync(idX, idY, 5, 5);
    global_pipe_comm.write_data(fileName.c_str(), resizedM, newWidth * newHeight * channels * sizeof(int64_t));
    time_end = InterChiplet::writeSync(time_end, idX, idY, 5, 5, newWidth * newHeight * channels * sizeof(int64_t), 0);

    // 输出缩放后的图像数据（仅输出第一个通道的数据）
    std::cout << "Resized Image (Channel 0):" << std::endl;
    for (int y = 0; y < newHeight; ++y) {
        for (int x = 0; x < newWidth; ++x) {
            std::cout << resizedM[y * newWidth + x] << " ";
        }
        std::cout << std::endl;
    }

    // 释放内存
    delete[] M;
    delete[] resizedM;

    return 0;
}
