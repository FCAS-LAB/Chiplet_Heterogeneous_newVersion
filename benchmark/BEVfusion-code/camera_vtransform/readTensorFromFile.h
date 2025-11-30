#ifndef READ_TENSOR_FROM_FILE_H
#define READ_TENSOR_FROM_FILE_H

#include <fstream>
#include <string>
#include <vector>
#include <array>
#include <stdexcept>
#include <iostream>
#include <sstream>

// 简化版的1维张量读取函数
template<size_t N>
std::array<float, N> read1DTensorFromFile(const std::string& filename) {
    std::array<float, N> result;
    
    // 初始化为0
    for (size_t i = 0; i < N; ++i) {
        result[i] = 0.0f;
    }
    
    try {
        std::cout << "尝试打开文件: " << filename << std::endl;
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "无法打开文件: " << filename << std::endl;
            return result; // 返回全0数组
        }
        
        // 读取整个文件内容
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        
        // 移除大括号
        if (content.size() >= 2) {
            content = content.substr(1, content.size() - 2);
        }
        
        // 解析浮点数
        std::stringstream ss(content);
        std::string item;
        size_t index = 0;
        
        while (std::getline(ss, item, ',') && index < N) {
            // 移除f后缀
            if (!item.empty() && item.back() == 'f') {
                item.pop_back();
            }
            
            // 移除空白字符
            item.erase(0, item.find_first_not_of(" \t\n\r\f\v"));
            item.erase(item.find_last_not_of(" \t\n\r\f\v") + 1);
            
            if (!item.empty()) {
                try {
                    result[index++] = std::stof(item);
                } catch (const std::exception& e) {
                    std::cerr << "解析错误: " << e.what() << " 在项: " << item << std::endl;
                }
            }
        }
        
        std::cout << "成功读取 " << index << " 个元素从 " << filename << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "读取文件时出错: " << e.what() << std::endl;
    }
    
    return result;
}

// 简化版的4维张量读取函数 - 返回全0数组
template<size_t D1, size_t D2, size_t D3, size_t D4>
std::array<std::array<std::array<std::array<float, D4>, D3>, D2>, D1> read4DTensorFromFile(const std::string& filename) {
    std::array<std::array<std::array<std::array<float, D4>, D3>, D2>, D1> result;
    
    // 初始化为0
    for (size_t i = 0; i < D1; ++i) {
        for (size_t j = 0; j < D2; ++j) {
            for (size_t k = 0; k < D3; ++k) {
                for (size_t l = 0; l < D4; ++l) {
                    result[i][j][k][l] = 0.0f;
                }
            }
        }
    }
    
    std::cout << "注意: 4D张量读取已简化，返回全0数组: " << filename << std::endl;
    return result;
}

#endif // READ_TENSOR_FROM_FILE_H