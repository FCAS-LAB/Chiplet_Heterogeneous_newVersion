#include <math.h>
#include <stdio.h>
#include <sys/time.h>
#include <cstdlib> 
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>
#include "apis_cu.h"
#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#define BLOCK_DIM 10

__global__ void matrix_mul_gpu(int64_t *M, int64_t *N, int64_t *P, int64_t widthA, int64_t heightA,
                               int64_t widthB) {
    int64_t i = threadIdx.x + blockDim.x * blockIdx.x;
    int64_t j = threadIdx.y + blockDim.y * blockIdx.y;
    if (i < widthB && j < heightA) {
        int64_t sum = 0;
        for (int64_t k = 0; k < widthA; k++) {
            int64_t a = M[j * widthA + k];
            int64_t b = N[k * widthB + i];
            sum += a * b;
        }
        P[j * widthB + i] = sum;
    }
}

int Row_A = 0, Col_A = 0, Row_B = 0, Col_B = 0;
int main(int argc, char **argv) {
    // 读取本进程所代表的chiplet编号
    int srcX = atoi(argv[1]);
    int srcY = atoi(argv[2]);
    while (1) {
        int64_t *size_A = new int64_t[2];
        int64_t *size_B = new int64_t[2];
        int64_t *Size_A, *Size_B;
        cudaMalloc((void **)&Size_A, sizeof(int64_t) * 2);
        cudaMalloc((void **)&Size_B, sizeof(int64_t) * 2);

        receiveMessage(srcX, srcY, 5, 5, Size_A, sizeof(int64_t) * 2);
        receiveMessage(srcX, srcY, 5, 5, Size_B, sizeof(int64_t) * 2);

        cudaMemcpy(size_A, Size_A, sizeof(int64_t) * 2, cudaMemcpyDeviceToHost);
        cudaMemcpy(size_B, Size_B, sizeof(int64_t) * 2, cudaMemcpyDeviceToHost);
        Row_A = size_A[0];
        Col_A = size_A[1];
        Row_B = size_B[0];
        Col_B = size_B[1];
        if(Row_A == -1 && Col_A == -1 && Row_B == -1 && Col_B == -1){
            cudaFree(Size_A);
            cudaFree(Size_B);

            delete[] size_A;
            delete[] size_B;
            break;
        }
        int64_t *C = (int64_t *)malloc(sizeof(int64_t) * Col_B * Row_A);
        int64_t *A = (int64_t *)malloc(sizeof(int64_t) * Row_A * Col_A);

        int64_t *d_dataA, *d_dataB, *d_dataC;
        cudaMalloc((void **)&d_dataA, sizeof(int64_t) * Row_A * Col_A);
        cudaMalloc((void **)&d_dataB, sizeof(int64_t) * Row_B * Col_B);
        cudaMalloc((void **)&d_dataC, sizeof(int64_t) * Col_B * Row_A);

        receiveMessage(srcX, srcY, 5, 5, d_dataA, Col_A * Row_A * sizeof(int64_t));
        receiveMessage(srcX, srcY, 5, 5, d_dataB, Col_B * Row_B * sizeof(int64_t));

        cudaMemcpy(A, d_dataA, sizeof(int64_t) * Col_A * Row_A, cudaMemcpyDeviceToHost);
        for (int64_t i = 0; i < Row_A * Col_A; i++) {
            std::cout << A[i];
            if (i % Col_A == 0 && i != 0)
                std::cout << std::endl;
            else
                std::cout << " ";
        }
        // calculate
        dim3 threadPerBlock(BLOCK_DIM, BLOCK_DIM);
        // dim3 blockNumber(1);
        dim3 blockNumber((Col_B + threadPerBlock.x - 1) / threadPerBlock.x,
                         (Row_A + threadPerBlock.y - 1) / threadPerBlock.y);
        matrix_mul_gpu<<<blockNumber, threadPerBlock>>>(d_dataA, d_dataB, d_dataC, Col_A, Row_A,
                                                        Col_B);
        cudaMemcpy(C, d_dataC, sizeof(int64_t) * Row_A * Col_B, cudaMemcpyDeviceToHost);
        for (int64_t i = 0; i < Row_A * Col_B; i++) {
            std::cout << C[i];
            if (i % Col_B == 0 && i != 0)
                std::cout << std::endl;
            else
                std::cout << " ";
        }
        sendMessage(5, 5, srcX, srcY, d_dataC, Row_A * Col_B * sizeof(int64_t));
        cudaFree(d_dataA);
        cudaFree(d_dataB);
        cudaFree(d_dataC);
    }
    return 0;
}