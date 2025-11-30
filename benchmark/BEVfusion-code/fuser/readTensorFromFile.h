#include <string>
#include <array>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cctype>
#include <algorithm>

// 一维张量读取函数声明
template <size_t N1>
std::array<float, N1> read1DTensorFromFile(const std::string& filename);

// 二维张量读取函数声明
template <size_t N1, size_t N2>
std::array<std::array<float, N2>, N1> read2DTensorFromFile(const std::string& filename);

// 三维张量读取函数声明
template <size_t N1, size_t N2, size_t N3>
std::array<std::array<std::array<float, N3>, N2>, N1> read3DTensorFromFile(const std::string& filename);

// 四维张量读取函数声明
template <size_t N1, size_t N2, size_t N3, size_t N4>
std::array<std::array<std::array<std::array<float, N4>, N3>, N2>, N1> read4DTensorFromFile(const std::string& filename);

// 五维张量读取函数声明
template <size_t N1, size_t N2, size_t N3, size_t N4, size_t N5>
std::array<std::array<std::array<std::array<std::array<float, N5>, N4>, N3>, N2>, N1> read5DTensorFromFile(const std::string& filename);


// 一维张量读取函数定义
template <size_t N1>
std::array<float, N1> read1DTensorFromFile(const std::string& filename) {
    std::ifstream file(filename);
    std::array<float, N1> tensor{};

    if (file.is_open()) {
        std::string line;
        size_t i = 0;
        if (std::getline(file, line)) {
            // 移除大括号和多余空格
            line.erase(std::remove_if(line.begin(), line.end(), [](char c) {
                return c == '{' || c == '}' || std::isspace(c);
            }), line.end());

            std::stringstream ss(line);
            std::string value;

            while (std::getline(ss, value, ',') && i < N1) {
                if (!value.empty()) {
                    try {
                        tensor[i] = std::stof(value);
                        i++;
                    } catch (const std::invalid_argument& e) {
                        std::cerr << "文件中的数据无效: " << e.what() << std::endl;
                    } catch (const std::out_of_range& e) {
                        std::cerr << "数据超出范围: " << e.what() << std::endl;
                    }
                }
            }

            if (i!= N1) {
                std::cerr << "错误: 数据数量与张量的第一维大小不匹配。" << std::endl;
            }
        }
        file.close();
    } else {
        std::cerr << "无法打开文件: " << filename << std::endl;
    }

    return tensor;
}

// 二维张量读取函数定义
template <size_t N1, size_t N2>
std::array<std::array<float, N2>, N1> read2DTensorFromFile(const std::string& filename) {
    std::ifstream file(filename);
    std::array<std::array<float, N2>, N1> tensor{};

    if (file.is_open()) {
        std::string line;
        // 读取并跳过第一行
        std::getline(file, line);
        size_t i = 0;
        while (std::getline(file, line) && i < N1) {
            // 移除大括号和多余空格
            line.erase(std::remove_if(line.begin(), line.end(), [](char c) {
                return c == '{' || c == '}' || std::isspace(c);
            }), line.end());

            std::stringstream ss(line);
            std::string value;
            size_t j = 0;

            while (std::getline(ss, value, ',') && j < N2) {
                if (!value.empty()) {
                    try {
                        tensor[i][j] = std::stof(value);
                        j++;
                    } catch (const std::invalid_argument& e) {
                        std::cerr << "文件中的数据无效: " << e.what() << std::endl;
                    } catch (const std::out_of_range& e) {
                        std::cerr << "数据超出范围: " << e.what() << std::endl;
                    }
                }
            }

            if (j!= N2) {
                std::cerr << "错误: 行数据的数量与张量的第二维大小不匹配。" << std::endl;
            }
            i++;
        }

        file.close();

        if (i!= N1) {
            std::cerr << "错误: 读取的数据行数与张量的第一维大小不匹配。" << std::endl;
        }
    } else {
        std::cerr << "无法打开文件: " << filename << std::endl;
    }

    return tensor;
}

// 三维张量读取函数定义
template <size_t N1, size_t N2, size_t N3>
std::array<std::array<std::array<float, N3>, N2>, N1> read3DTensorFromFile(const std::string& filename) {
    std::ifstream file(filename);
    std::array<std::array<std::array<float, N3>, N2>, N1> tensor{};

    if (file.is_open()) {
        std::string line;
        // 读取并丢弃第一行
        std::getline(file, line);
        size_t i = 0;
        while (std::getline(file, line) && i < N1) {
            size_t j = 0;
            // 直接开始处理数据行
            while (std::getline(file, line) && j < N2) {
                // 移除大括号和多余空格
                line.erase(std::remove_if(line.begin(), line.end(), [](char c) {
                    return c == '{' || c == '}' || std::isspace(c);
                }), line.end());

                std::stringstream ss(line);
                std::string value;
                size_t k = 0;

                while (std::getline(ss, value, ',') && k < N3) {
                    if (!value.empty()) {
                        try {
                            tensor[i][j][k] = std::stof(value);
                            k++;
                        } catch (const std::invalid_argument& e) {
                            std::cerr << "文件中的数据无效: " << e.what() << std::endl;
                        } catch (const std::out_of_range& e) {
                            std::cerr << "数据超出范围: " << e.what() << std::endl;
                        }
                    }
                }

                if (k!= N3) {
                    std::cerr << "错误: 行数据的数量与张量的第三维大小不匹配。" << std::endl;
                }
                j++;
            }

            if (j!= N2) {
                std::cerr << "错误: 读取的数据行数与张量的第二维大小不匹配。" << std::endl;
            }
            i++;
        }

        file.close();

        if (i!= N1) {
            std::cerr << "错误: 读取的数据层数与张量的第一维大小不匹配。" << std::endl;
        }
    } else {
        std::cerr << "无法打开文件: " << filename << std::endl;
    }

    return tensor;
}

// 四维张量读取函数定义
template <size_t N1, size_t N2, size_t N3, size_t N4>
std::array<std::array<std::array<std::array<float, N4>, N3>, N2>, N1> read4DTensorFromFile(const std::string& filename) {
    std::ifstream file(filename);
    std::array<std::array<std::array<std::array<float, N4>, N3>, N2>, N1> tensor{};

    if (file.is_open()) {
        std::string line;
        // 读取并丢弃第一行
        std::getline(file, line);
        size_t i = 0;
        while (std::getline(file, line) && i < N1) {
            size_t j = 0;
            while (std::getline(file, line) && j < N2) {
                size_t k = 0;
                while (std::getline(file, line) && k < N3) {
                    // 移除大括号和多余空格
                    line.erase(std::remove_if(line.begin(), line.end(), [](char c) {
                        return c == '{' || c == '}' || std::isspace(c);
                    }), line.end());

                    std::stringstream ss(line);
                    std::string value;
                    size_t l = 0;

                    while (std::getline(ss, value, ',') && l < N4) {
                        if (!value.empty()) {
                            try {
                                tensor[i][j][k][l] = std::stof(value);
                                l++;
                            } catch (const std::invalid_argument& e) {
                                std::cerr << "文件中的数据无效: " << e.what() << std::endl;
                            } catch (const std::out_of_range& e) {
                                std::cerr << "数据超出范围: " << e.what() << std::endl;
                            }
                        }
                    }

                    if (l!= N4) {
                        std::cerr << "错误: 行数据的数量与张量的第四维大小不匹配。" << std::endl;
                    }
                    k++;
                }

                if (k!= N3) {
                    std::cerr << "错误: 读取的数据行数与张量的第三维大小不匹配。" << std::endl;
                }
                j++;
            }

            if (j!= N2) {
                std::cerr << "错误: 读取的数据行数与张量的第二维大小不匹配。" << std::endl;
            }
            i++;
        }

        file.close();

        if (i!= N1) {
            std::cerr << "错误: 读取的数据层数与张量的第一维大小不匹配。" << std::endl;
        }
    } else {
        std::cerr << "无法打开文件: " << filename << std::endl;
    }

    return tensor;
}
// 五维张量读取函数定义
template <size_t N1, size_t N2, size_t N3, size_t N4, size_t N5>
std::array<std::array<std::array<std::array<std::array<float, N5>, N4>, N3>, N2>, N1> read5DTensorFromFile(const std::string& filename) {
    std::ifstream file(filename);
    std::array<std::array<std::array<std::array<std::array<float, N5>, N4>, N3>, N2>, N1> tensor{};

    if (file.is_open()) {
        std::string line;
        // 读取并丢弃第一行
        std::getline(file, line);
        size_t i = 0;
        while (std::getline(file, line) && i < N1) {
            size_t j = 0;
            while (std::getline(file, line) && j < N2) {
                size_t k = 0;
                while (std::getline(file, line) && k < N3) {
                    size_t m = 0;
                    while (std::getline(file, line) && m < N4) {
                        // 移除大括号和多余空格
                        line.erase(std::remove_if(line.begin(), line.end(), [](char c) {
                            return c == '{' || c == '}' || std::isspace(c);
                        }), line.end());

                        std::stringstream ss(line);
                        std::string value;
                        size_t n = 0;

                        while (std::getline(ss, value, ',') && n < N5) {
                            if (!value.empty()) {
                                try {
                                    tensor[i][j][k][m][n] = std::stof(value);
                                    n++;
                                } catch (const std::invalid_argument& e) {
                                    std::cerr << "文件中的数据无效: " << e.what() << std::endl;
                                } catch (const std::out_of_range& e) {
                                    std::cerr << "数据超出范围: " << e.what() << std::endl;
                                }
                            }
                        }

                        if (n!= N5) {
                            std::cerr << "错误: 行数据的数量与张量的第五维大小不匹配。" << std::endl;
                        }
                        m++;
                    }

                    if (m!= N4) {
                        std::cerr << "错误: 读取的数据行数与张量的第四维大小不匹配。" << std::endl;
                    }
                    k++;
                }

                if (k!= N3) {
                    std::cerr << "错误: 读取的数据行数与张量的第三维大小不匹配。" << std::endl;
                }
                j++;
            }

            if (j!= N2) {
                std::cerr << "错误: 读取的数据行数与张量的第二维大小不匹配。" << std::endl;
            }
            i++;
        }

        file.close();

        if (i!= N1) {
            std::cerr << "错误: 读取的数据层数与张量的第一维大小不匹配。" << std::endl;
        }
    } else {
        std::cerr << "无法打开文件: " << filename << std::endl;
    }

    return tensor;
}

