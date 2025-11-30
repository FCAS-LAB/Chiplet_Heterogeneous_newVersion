#include <math.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <nlohmann/json.hpp>
#include <random>
#include <thread>
#include <cstdlib>  // for std::getenv
#include <string>
#include <vector>

#include "apis_c.h"

using json = nlohmann::json;
// using namespace InterChiplet;

#define MAX_POINT_NUM  1024
#define OUTCH_NUM 1
#define INCH_NUM 3
#define RANGE 1
#define CFG_CNN_SIZE_X 3
#define CFG_CNN_SIZE_Y 3
#define CFG_CNN_SIZE_Z 3
#define WT_DATA_SIZE 32*4

void random_init(int64_t * data, int size){
    int i;
    for (i = 0; i < size; i++){
        //data[i] = rand();
        data[i] = 128;
    }
}

void random_init_uint16(uint16_t * data, int size, int max_value){
    int i;
    for (i = 0; i < size; i++){
        //data[i] = rand() % max_value;
        data[i] = 107 % max_value;
    }
}

void write_int8_txt(const char * filename, int8_t *data, int size){
    FILE * file_out = fopen(filename, "w"); 
    int i;
    for (i = 0; i < size; i++){
        fprintf(file_out, "%02x\n", (uint8_t)data[i]);
    } 
    printf("[Info:] Writing data to %s . \n", filename);
    fclose(file_out); // 关闭文件  
}

int srcX = 0, srcY = 0;
int input_size = 0;
std::vector<int> hidden_size;
int output_size = 0;
std::vector<std::vector<std::vector<double>>> weight, biases;
std::vector<int> layer_sizes;
std::vector<std::vector<std::vector<double>>> zs, activations;
std::vector<std::vector<double>> a1;
std::vector<std::vector<double>> Randn(int line, int column);
void mkfile(char* fileName) {
    FILE* file = fopen(fileName, "w");
    if (file != NULL) std::cout << fileName << "创建成功" << std::endl;
    fclose(file);
}
bool checkfile(int srcX, int srcY, int dstX, int dstY) {
    char* fileName = new char[100];
    sprintf(fileName, "./cpuRead%d_%d_%d_%d", srcX, srcY, dstX, dstY);
    FILE* file = fopen(fileName, "r");
    delete[] fileName;
    if (file == NULL)
        return 0;
    else
        return 1;
}
void delfile(char* fileName) {
    if (remove(fileName) == 0) {
        printf("文件 \"%s\" 已成功删除。\n", fileName);
    }
}

void BPNeuralNetwork(int Input_size, std::vector<int> Hidden_size, int Output_size, int SrcX,
                     int SrcY) {
    srcX = SrcX;
    srcY = SrcY;
    hidden_size = Hidden_size;
    input_size = Input_size;
    output_size = Output_size;
    layer_sizes.push_back(input_size);
    for (size_t i = 0; i < hidden_size.size(); i++) {
        layer_sizes.push_back(hidden_size[i]);
    }
    layer_sizes.push_back(output_size);
    for (size_t i = 0; i < layer_sizes.size() - 1; i++) {  // He初始化
        std::vector<std::vector<double>> rand_arr = Randn(layer_sizes[i + 1], layer_sizes[i]);
        for (size_t i = 0; i < rand_arr.size(); i++) {
            for (size_t j = 0; j < rand_arr[i].size(); j++) {
                rand_arr[i][j] *= sqrt(2.0 / (layer_sizes[i] > 1 ? layer_sizes[i] : 1));
                std::cout << rand_arr[i][j] << " ";
            }
            std::cout << std::endl;
        }
        std::cout << "##########################################" << std::endl;
        weight.push_back(rand_arr);
        biases.push_back(Randn(layer_sizes[i + 1], 1));
    }
}
std::vector<std::vector<double>> doubleToVector(std::vector<std::vector<double>> V,
                                                double* weight_i) {
    for (size_t i = 0; i < V.size(); i++) {
        // std::vector<double> temp;
        for (size_t j = 0; j < V[i].size(); j++) {
            V[i][j] = (weight_i[j + i * V[0].size()]);
        }
        // V.push_back(temp);
    }
    return V;
}
std::vector<std::vector<double>> Randn(int line, int column) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> distribution(0.0, 1.0);  // 均值为0，标准差为1的正态分布
    std::vector<std::vector<double>> result;
    for (int i = 0; i < line; i++) {
        std::vector<double> result_i;
        for (int j = 0; j < column; j++) {
            result_i.push_back(distribution(gen));
        }
        result.push_back(result_i);
    }
    return result;
}

void vectorToDouble(std::vector<std::vector<double>> V, double* weight_i) {
    for (size_t i = 0; i < V.size(); i++) {
        for (size_t j = 0; j < V[i].size(); j++) {
            weight_i[j + i * V[0].size()] = V[i][j];
        }
    }
}

void DoubleToInt(double* mat1, int64_t* mat2, int size) {
    int64_t time = std::pow(10, 8);
    for (int i = 0; i < size; i++) {
        mat2[i] = mat1[i] * time;
    }
}
void IntToDouble(double* mat1, int64_t* mat2, int size) {
    double time = std::pow(10, 16);
    for (int i = 0; i < size; i++) {
        mat1[i] = mat2[i] / time;
    }
}

// void Transpose_GPU(double* mat1,int fst_Row,int fst_Col){

// }

void GpuMultiply(double* mat1, double* mat2, int fst_Row, int fst_Col, int sec_Row, int sec_Col,
                 std::vector<std::vector<double>>& Res, int dstX, int dstY) {
    int64_t* Mat1 = new int64_t[fst_Row * fst_Col];
    int64_t* Mat2 = new int64_t[sec_Row * sec_Col];
    int64_t* Mat1_size = new int64_t[2];
    int64_t* Mat2_size = new int64_t[2];
    Mat1_size[0] = fst_Row;
    Mat1_size[1] = fst_Col;
    Mat2_size[0] = sec_Row;
    Mat2_size[1] = sec_Col;
    DoubleToInt(mat1, Mat1, fst_Row * fst_Col);
    DoubleToInt(mat2, Mat2, sec_Row * sec_Col);
    std::cout << "fst_Row: "<<fst_Row<<" fst_Col: "<<fst_Col << std::endl;
    InterChiplet::sendMessage(dstX, dstY, srcX, srcY, Mat1_size, 2 * sizeof(int64_t));
    std::cout << "sec_Row: "<<sec_Row<<" sec_Col: "<<sec_Col << std::endl;
    InterChiplet::sendMessage(dstX, dstY, srcX, srcY, Mat2_size, 2 * sizeof(int64_t));
    std::cout << "##########################################" << std::endl;
    InterChiplet::sendMessage(dstX, dstY, srcX, srcY, Mat1, fst_Col * fst_Row * sizeof(int64_t));
    std::cout << "##########################################" << std::endl;
    InterChiplet::sendMessage(dstX, dstY, srcX, srcY, Mat2, sec_Row * sec_Col * sizeof(int64_t));
    std::cout << "##########################################" << std::endl;
    bool file = 1;
    while (file == 0) {
        file = checkfile(dstX, dstY, srcX, srcY);
    }

    double* result = new double[fst_Row * sec_Col];
    int64_t* Result_2 = new int64_t[fst_Row * sec_Col];
    InterChiplet::receiveMessage(srcX, srcY, dstX, dstY, Result_2,
                                 fst_Row * sec_Col * sizeof(int64_t));
    IntToDouble(result, Result_2, fst_Row * sec_Col);
    // std::lock_guard<std::mutex> lock(mtx);
    std::vector<std::vector<double>> res(fst_Row, std::vector<double>(sec_Col));
    Res = doubleToVector(res, result);
    // delete[] fileName;
    // fileName=NULL;
    delete[] result;
    result = NULL;
    delete[] Result_2;
    Result_2 = NULL;
    delete[] Mat1;
    delete[] Mat2;
    delete[] Mat1_size;
    delete[] Mat2_size;
    delete[] mat1;
    delete[] mat2;
    Mat1 = NULL;
    Mat2 = NULL;
    Mat1_size = NULL;
    Mat2_size = NULL;
}
void ToGPU(double* mat1, double* mat2, int fst_Row, int fst_Col, int sec_Row, int sec_Col,
           std::vector<std::vector<double>>& Res, int gpu_num) {
    std::vector<std::vector<std::vector<double>>> dev1, dev2;
    std::vector<std::vector<double>> dev1_, dev2_;
    int Col_per_GPU = fst_Col / 2 + fst_Col % 2;
    for (int start = 0; start < fst_Col; start += Col_per_GPU) {
        for (int i = 0; i < fst_Row; i++) {
            std::vector<double> dev_temp;
            for (int j = start; j < fst_Col && j < start + Col_per_GPU; j++) {
                dev_temp.push_back(mat1[i * fst_Col + j]);
            }
            dev1_.push_back(dev_temp);
        }
        dev1.push_back(dev1_);
        dev1_.clear();
    }
    for (int i = 0; i < sec_Row; i++) {
        std::vector<double> dev_temp;
        for (int j = 0; j < sec_Col; j++) {
            dev_temp.push_back(mat2[i * sec_Col + j]);
        }
        dev2_.push_back(dev_temp);
        if ((i + 1) % Col_per_GPU == 0 || i == sec_Row - 1) {
            dev2.push_back(dev2_);
            dev2_.clear();
        }
    }
    int dstX = 0;
    if (gpu_num == 2) {
        dstX = 1;
    }
    std::vector<std::thread> THREAD;
    std::vector<std::vector<std::vector<double>>> res;
    for (size_t i = 0; i < dev1.size(); i++) {
        std::vector<std::vector<double>> res_temp(dev1[i].size(),
                                                  std::vector<double>(dev2[i][0].size()));
        res.push_back(res_temp);
    }
    for (size_t i = 0; i < dev1.size(); i++) {
        double* Dev1 = new double[dev1[i].size() * dev1[i][0].size()];
        vectorToDouble(dev1[i], Dev1);
        double* Dev2 = new double[dev2[i].size() * dev2[i][0].size()];
        vectorToDouble(dev2[i], Dev2);
        GpuMultiply(Dev1,Dev2,dev1[i].size(),dev1[i][0].size(),dev2[i].size(),dev2[i][0].size(),std::ref(res[i]),dstX,i+1);
        // std::thread
        // t(&BPNeuralNetwork::GpuMultiply,this,Dev1,Dev2,dev1[i].size(),dev1[i][0].size(),dev2[i].size(),dev2[i][0].size(),std::ref(res[i]),dstX,i+1);
        // THREAD.push_back(std::thread(GpuMultiply, Dev1, Dev2, dev1[i].size(), dev1[i][0].size(),
        //                              dev2[i].size(), dev2[i][0].size(), std::ref(res[i]), dstX,
        //                              i + 1));
    }
    // for (auto& i : THREAD) {
    //     i.join();
    // }
    Res = res[0];
    for (size_t i = 1; i < res.size(); i++) {
        for (size_t j = 0; j < res[i].size(); j++) {
            for (size_t z = 0; z < res[i][j].size(); z++) {
                Res[j][z] += res[i][j][z];
            }
        }
    }
}
std::vector<std::vector<double>> activate_function(std::vector<std::vector<double>> x) {
    for (size_t i = 0; i < x.size(); i++) {
        for (size_t j = 0; j < x[i].size(); j++) {
            if (x[i][j] <= 0) x[i][j] = 0.01 * x[i][j];
        }
    }
    return x;
}

std::vector<std::vector<double>> activate_function_derivative(std::vector<std::vector<double>> x) {
    for (size_t i = 0; i < x.size(); i++) {
        for (size_t j = 0; j < x[i].size(); j++) {
            if (x[i][j] <= 0)
                x[i][j] = 0.01;
            else
                x[i][j] = 1;
        }
    }
    return x;
}

void T(double* a, std::vector<std::vector<double>> x, int Row, int Col) {
    std::cout<<"#########################test T####################"<<std::endl;
    double *x1 = new double[Row * Col];
    vectorToDouble(x, x1);
    int64_t *size_A = new int64_t[2];
    size_A[0] = Row;
    size_A[1] = Col;
    InterChiplet::sendMessage(2, 0, srcX, srcY, size_A, 2 * sizeof(int64_t));
    std::cout<<"Row: "<<Row<<" Col: "<<Col<<std::endl;
    InterChiplet::sendMessage(2, 0, srcX, srcY, x1, Col * Row * sizeof(double));
    InterChiplet::receiveMessage(srcX, srcY, 2, 0, a, Col * Row * sizeof(double));
    std::cout<<"#########################test T over####################"<<std::endl;
    delete[] size_A;
    for (int j = 0; j < Col; j++) {
        for (int i = 0; i < Row; i++) {
            // a[i + j * Row] = x[i][j];
            a1[j][i] = x[i][j];
        }
    }
}

void T2(double* a, std::vector<std::vector<double>> x, int Row, int Col) {
    for (int j = 0; j < Col; j++) {
        for (int i = 0; i < Row; i++) {
            a[i + j * Row] = x[i][j];
        }
    }
    // vectorToDouble(x,a);
    // Transpose_GPU(a,x.size(),x[0].size());
}

double c_norm(const std::vector<std::vector<double>>& vec) {
    double sum_of_squares = 0.0;
    for (const auto& row : vec) {
        for (double x : row) {
            sum_of_squares += x * x;
        }
    }
    return std::sqrt(sum_of_squares);
}

// 计算二维向量沿着指定轴的和，并保持维度
std::vector<std::vector<double>> sum_axis(const std::vector<std::vector<double>>& vec, int axis,
                                          int m) {
    std::vector<std::vector<double>> result;

    if (axis == 0) {
        for (size_t j = 0; j < vec[0].size(); ++j) {
            double sum = 0.0;
            for (size_t i = 0; i < vec.size(); ++i) {
                sum += vec[i][j];
            }
            result.push_back({sum / m});
        }
    } else if (axis == 1) {
        for (const auto& row : vec) {
            double sum = 0.0;
            for (double x : row) {
                sum += x;
            }
            result.push_back({sum / m});
        }
    }
    return result;
}
std::vector<std::vector<double>> forward(std::vector<std::vector<double>>& x) {
    int Row = x.size();
    int Col = x[0].size();
    // std::cout<<"#############################################"<<std::endl;
    double* A = new double[Col * Row];
    for (int i = 0; i < Col; i++) {
        std::vector<double> a2;
        for (int j = 0; j < Row; j++) {
            a2.push_back(0);
        }
        a1.push_back(a2);
    }
    T(A, x, Row, Col);
    // std::cout<<"#############################################"<<std::endl;
    activations.push_back(a1);
    for (size_t i = 0; i < weight.size(); i++) {
        double* Weight = new double[weight[i].size() * weight[i][0].size()];
        vectorToDouble(weight[i], Weight);
        std::vector<std::vector<double>> DotRns;
        ToGPU(Weight, A, weight[i].size(), weight[i][0].size(), Col, Row, DotRns, 1);
        std::cout << "#############################################" << std::endl;
        for (size_t m = 0; m < DotRns.size(); m++) {
            for (size_t n = 0; n < DotRns[m].size(); n++) {
                DotRns[m][n] += biases[i][m][0];
            }
        }
        std::vector<std::vector<double>> z = DotRns;
        a1 = activate_function(z);
        Col = a1.size();
        Row = a1[0].size();
        delete[] A;
        A = new double[Row * Col];
        vectorToDouble(a1, A);
        zs.push_back(z);
        activations.push_back(a1);
        delete[] Weight;
        Weight = NULL;
    }
    // delete[] A;
    return a1;
}

void backward(std::vector<std::vector<double>> x, std::vector<std::vector<double>>& y,
              double learning_rate) {
    int m = x.size();  // 获取第一个维度大小
    std::vector<std::vector<double>> y_hat = activations[activations.size() - 1];
    double* a = new double[y.size() * y[0].size()];
    int Row = y.size();
    int Col = y[0].size();
    T2(a, y, Row, Col);

    std::vector<std::vector<std::vector<double>>> deltas;  // 用于存储每一层的误差项
    std::vector<std::vector<double>> d_temp;
    for (size_t i = 0; i < y_hat.size(); i++) {
        std::vector<double> temp;
        for (size_t j = 0; j < y_hat[i].size(); j++) {
            temp.push_back(y_hat[i][j] - a[i * y_hat[i].size() + j]);
        }
        d_temp.push_back(temp);
    }
    deltas.push_back(d_temp);

    std::vector<std::vector<std::vector<double>>> grads_weights, grads_biases;
    for (int i = weight.size() - 1; i >= 0; i--) {
        // double* Deltas=new
        // double[deltas[deltas.size()-1].size()*deltas[deltas.size()-1][0].size()];
        // vectorToDouble(deltas[deltas.size()-1],Deltas);
        std::vector<std::vector<double>> act_F = activate_function_derivative(zs[i]);
        std::vector<std::vector<double>> dz(act_F.size(), std::vector<double>(act_F[0].size()));
        for (size_t m = 0; m < act_F.size(); m++) {
            for (size_t n = 0; n < act_F[m].size(); n++) {
                dz[m][n] = act_F[m][n] * deltas[deltas.size() - 1][m][n];
            }
        }

        // delete[] Deltas;delete[] Act_F;
        int max_grad_norm = 10;  // 设置梯度的最大范数
        double norm = c_norm(dz);
        if (norm > max_grad_norm) {
            for (size_t i = 0; i < dz.size(); i++) {
                for (size_t j = 0; j < dz[i].size(); j++) {
                    dz[i][j] *= max_grad_norm / norm;
                }
            }
        }

        double* Dz = new double[dz.size() * dz[0].size()];
        vectorToDouble(dz, Dz);
        double* Activations_i = new double[activations[i].size() * activations[i][0].size()];
        T2(Activations_i, activations[i], activations[i].size(), activations[i][0].size());

        double* Weight = new double[weight[i].size() * weight[i][0].size()];
        T2(Weight, weight[i], weight[i].size(), weight[i][0].size());
        std::vector<std::vector<double>> deltas_pre;
        std::vector<std::vector<double>> dw;
        // std::thread t1(ToGPU, Weight, Dz, weight[i][0].size(), weight[i].size(), dz.size(),
        //                dz[0].size(), std::ref(deltas_pre), 1);
        // GpuMultiply(Weight,Dz,weight[i][0].size(),weight[i].size(),dz.size(),dz[0].size(),std::ref(deltas_pre),1);
        ToGPU(Weight, Dz, weight[i][0].size(), weight[i].size(), dz.size(),
                       dz[0].size(), std::ref(deltas_pre), 1);
        // std::thread t2(ToGPU, Dz, Activations_i, dz.size(), dz[0].size(), activations[i][0].size(),
        //                activations[i].size(), std::ref(dw), 2);
        ToGPU(Dz, Activations_i, dz.size(), dz[0].size(), activations[i][0].size(),
                       activations[i].size(), std::ref(dw), 2);
        std::cout
            << "***********************************************************************************"
            << i << std::endl;
        // GpuMultiply(Dz,Activations_i,dz.size(),dz[0].size(),activations[i][0].size(),activations[i].size(),std::ref(dw),1);
        // t1.join();
        // t2.join();
        deltas.push_back(deltas_pre);
        for (size_t i = 0; i < dw.size(); i++) {
            for (size_t j = 0; j < dw[i].size(); j++) {
                dw[i][j] *= 1 / m;
            }
        }

        delete[] Activations_i;
        Activations_i = NULL;
        std::vector<std::vector<double>> db = sum_axis(dz, 1, m);
        grads_weights.push_back(dw);
        grads_biases.push_back(db);
        delete[] Weight;
        delete[] Dz;
        Weight = NULL;
        Dz = NULL;
    }
    std::reverse(grads_weights.begin(), grads_weights.end());
    std::reverse(grads_biases.begin(), grads_biases.end());

    // 跟新权重和偏置值
    for (size_t i = 0; i < weight.size(); i++) {
        for (size_t j = 0; j < weight[i].size(); j++) {
            for (size_t k = 0; k < weight[i][j].size(); k++) {
                weight[i][j][k] -= learning_rate * grads_weights[i][j][k];
            }
        }
    }
    for (size_t i = 0; i < biases.size(); i++) {
        for (size_t j = 0; j < biases[i].size(); j++) {
            for (size_t k = 0; k < biases[i][j].size(); k++) {
                biases[i][j][k] -= learning_rate * grads_biases[i][j][k];
            }
        }
    }
}

void train(std::vector<std::vector<double>>& x, std::vector<std::vector<double>> y,
           int num_iterations = 1000, double learning_rate = 0.1) {
    for (int i = 0; i < num_iterations; i++) {
        forward(x);
        backward(x, y, learning_rate);
    }
}

void predict_classify(std::vector<std::vector<double>> x, int y_size, std::vector<double> y) {
    std::vector<std::vector<double>> y_hat;
    y_hat = forward(x);
    double* Y_hat = new double[y_hat.size() * y_hat[0].size()];
    T(Y_hat, y_hat, y_hat.size(), y_hat[0].size());
    y_hat = doubleToVector(y_hat, Y_hat);
    delete[] Y_hat;
    Y_hat = NULL;
    std::vector<int> predictions;
    if (y_size > 2) {
        for (size_t i = 0; i < y_hat.size(); i++) {
            double Max = y_hat[i][0];
            int max_index = 1;
            for (size_t j = 0; j < y_hat[i].size(); j++) {
                if (Max > y_hat[i][j]) {
                    Max = y_hat[i][j];
                    max_index = j + 1;
                }
            }
            predictions.push_back(max_index);
        }
    } else {
        for (size_t i = 0; i < y_hat.size(); i++) {
            int max_index = 0;
            for (size_t j = 0; j < y_hat[i].size(); j++) {
                if (y_hat[i][j] > 0.5) {
                    max_index = 1;
                }
            }
            predictions.push_back(max_index);
        }
    }
    for (auto i : predictions) {
        std::cout << i << " ";
    }
    std::cout << std::endl;
    for (auto i : y) {
        std::cout << i << " ";
    }
    std::cout << std::endl;
}

bool readDataFromJSON(const std::string& filename, std::vector<std::vector<double>>& x_train,
                      std::vector<std::vector<double>>& x_test,
                      std::vector<std::vector<double>>& y_train,
                      std::vector<std::vector<double>>& y_test) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening JSON file." << std::endl;
        return false;
    }

    json data;
    file >> data;

    for (const auto& row : data["x_train"]) {
        std::vector<double> row_vec;
        for (const auto& val : row) {
            row_vec.push_back(val);
            std::cout << val << " ";
        }
        std::cout << std::endl;
        x_train.push_back(row_vec);
    }

    for (const auto& row : data["x_test"]) {
        std::vector<double> row_vec;
        for (const auto& val : row) {
            row_vec.push_back(val);
            std::cout << val << " ";
        }
        std::cout << std::endl;
        x_test.push_back(row_vec);
    }

    // Extract target data
    for (const auto& val : data["y_train"]) {
        std::vector<double> temp;
        if (val == 1) {
            temp.push_back(1);
            temp.push_back(0);
            temp.push_back(0);
        } else if (val == 2) {
            temp.push_back(0);
            temp.push_back(1);
            temp.push_back(0);
        } else {
            temp.push_back(0);
            temp.push_back(0);
            temp.push_back(1);
        }
        y_train.push_back(temp);
        std::cout << val << " ";
    }
    std::cout << std::endl;
    for (const auto& val : data["y_test"]) {
        std::vector<double> temp;
        if (val == 1) {
            temp.push_back(1);
            temp.push_back(0);
            temp.push_back(0);
        } else if (val == 2) {
            temp.push_back(0);
            temp.push_back(1);
            temp.push_back(0);
        } else {
            temp.push_back(0);
            temp.push_back(0);
            temp.push_back(1);
        }
        y_test.push_back(temp);
        std::cout << val << " ";
    }
    return true;
}

std::string getSimulatorRootPath() {
    const char* env_p = std::getenv("SIMULATOR_ROOT");
    if (env_p == nullptr) {
        std::cerr << "Error: SIMULATOR_ROOT environment variable is not set." << std::endl;
        exit(EXIT_FAILURE);
    }
    return std::string(env_p) + "/benchmark/MLP_NVL/";
}

int main(int argc, char** argv) {
    std::string json_path = getSimulatorRootPath()+ "temp_data.json";

    int srcX = atoi(argv[1]);
    int srcY = atoi(argv[2]);
    int size = 12*12*8;
    int64_t *test = new int64_t[size];
    int64_t *test_ans = new int64_t[size];
    random_init(test, size);
    InterChiplet::sendMessage(0, 3, srcX, srcY, test, size*sizeof(int64_t));
    InterChiplet::receiveMessage(srcX, srcY, 0, 3, test_ans, size*sizeof(int64_t));
    delete[] test;
    delete[] test_ans;
    std::cout<<"-------------------------------------mnsim over-------------------------------------"<<std::endl;
    std::vector<int> hidden_size = {10, 15};
    std::vector<std::vector<double>> x_train, x_test;
    std::vector<std::vector<double>> y_train, y_test;

    if (!readDataFromJSON(json_path, x_train, x_test, y_train, y_test)) {
        std::cerr << "Data reading error" << std::endl;
    }

    BPNeuralNetwork(13, hidden_size, 3, srcX, srcY);
    train(x_train, y_train, 1);

    // 通知所有进程结束
    std::cout<<"send end"<<std::endl;
    int64_t* Mat1_size = new int64_t[2];
    int64_t* Mat2_size = new int64_t[2];
    Mat1_size[0] = -1;
    Mat1_size[1] = -1;
    Mat2_size[0] = -1;
    Mat2_size[1] = -1;

    for(int i=0;i<2;i++){
        for(int j=1;j<=2;j++){
            InterChiplet::sendMessage(i, j, srcX, srcY, Mat1_size, 2 * sizeof(int64_t));
            InterChiplet::sendMessage(i, j, srcX, srcY, Mat2_size, 2 * sizeof(int64_t));
        }
    }
    InterChiplet::sendMessage(2, 0, srcX, srcY, Mat1_size, 2 * sizeof(int64_t));

    delete[] Mat1_size;
    delete[] Mat2_size;
    std::cout<<"end"<<std::endl;
    return 0;
}
