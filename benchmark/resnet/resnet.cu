#include<stdio.h>
#include<iostream>
#include<cuda_runtime.h>
#include<stdlib.h>
#include<device_launch_parameters.h>
#include<random>
#include <curand.h>
#include <curand_kernel.h>
#include <ctime>
#include <sys/time.h>
#include <thrust/extrema.h>
using namespace std;

__global__ void conv(float* M1, float* kernel, float* M_out, int width, int height, int in_channels, int out_channels, int kernel_size, int stride, int padding = 3) {
	int x = threadIdx.x + blockDim.x * blockIdx.x;
	int y = threadIdx.y + blockDim.y * blockIdx.y;
	int z = threadIdx.z + blockDim.z * blockIdx.z;

	int out_width = (width - kernel_size + 2 * padding) / stride + 1;
	int out_height = (height - kernel_size + 2 * padding) / stride + 1;
	if (x < out_width && y < out_height && z < out_channels) {
		// 计算输入特征图中卷积核覆盖的区域的起始坐标
		int x_start = x * stride - padding;
		int y_start = y * stride - padding;

		float sum = 0;
		for (int c = 0; c < in_channels; c++) {
			for (int i = 0; i < kernel_size; i++) {
				for (int j = 0; j < kernel_size; j++) {
					int input_x = x_start + j;
					int input_y = y_start + i;

					// 边界处理：如果输入坐标在边界外，则使用边界填充的值
					if (input_x >= 0 && input_x < width && input_y >= 0 && input_y < height) {
						float val = M1[input_x + input_y * width + c * width * height];
						float k_val = kernel[j + i * kernel_size + c * kernel_size * kernel_size + z * kernel_size * kernel_size * in_channels];
						sum += val * k_val;
					}
				}
			}
		}
		M_out[x + y * out_width + z * out_height * out_width] = sum;
	}
}


__global__ void BatchNormed(int in_channels, int width, int height, float* M1, float* mean_value, float* stddev) {
	int x = threadIdx.x + blockDim.x * blockIdx.x;
	int y = threadIdx.y + blockDim.y * blockIdx.y;
	int z = threadIdx.z + blockDim.z * blockIdx.z;

	if(stddev[z]==0){
		stddev[z]=1;
	}
	if (x < width && y < height && z < in_channels)
		M1[x + y * width + z * width * height] = (M1[x + y * width + z * width * height] - mean_value[z]) / stddev[z];
}

__global__ void ReLu(int in_channels, int width, int height, float* M1) {
	int x = threadIdx.x + blockDim.x * blockIdx.x;
	int y = threadIdx.y + blockDim.y * blockIdx.y;
	int z = threadIdx.z + blockDim.z * blockIdx.z;

	if (x < width && y < height && z < in_channels && M1[x + y * width + z * width * height] < 0) {
		M1[x + y * width + z * width * height] = 0;
	}
}


__global__ void Maxpool(int in_channels, int width, int height, int out_width, int out_height, float* M1, float* M1_out, int kernel_size, int stride) {
	int x = threadIdx.x + blockDim.x * blockIdx.x;
	int y = threadIdx.y + blockDim.y * blockIdx.y;
	int z = threadIdx.z + blockDim.z * blockIdx.z;

	if (x < out_width && y < out_height && z < in_channels) {
		int x_start = x * stride;
		int y_start = y * stride;
		float Max = M1[x_start + y_start * width + z * width * height];
		for (int i = 0; i < kernel_size; i++) {
			for (int j = 0; j < kernel_size; j++) {
				int x_input = x_start + i;
				int y_input = y_start + j;

				if (x_input >= 0 && x_input < width && y_input >= 0 && y_input < height) {
					if (Max < M1[x_input + y_input * width + z * width * height]) {
						Max = M1[x_input + y_input * width + z * width * height];
					}
				}
			}
		}
		M1_out[x + y * out_width + z * out_width * out_height] = Max;
	}
}

__global__ void compute_mean_stddev(float* M1_out, float* mean_value, float* stddev, int out_width, int out_height, int out_channels) {
	// int stride = blockDim.x * gridDim.x;
	int x = threadIdx.x + blockDim.x * blockIdx.x;
	// 计算均值
	if(x<out_channels){
		float sum = 0;
		for (int j = 0; j < out_height; j++) {
			for (int z = 0; z < out_width; z++) {
				sum += M1_out[z + j * out_width + x * out_width * out_height];
			}
		}
		mean_value[x] = sum / (out_height * out_width);
		

		// 同时计算标准差
		
		float dev = 0;
		for (int j = 0; j < out_height; j++) {
			for (int z = 0; z < out_width; z++) {
				float diff = M1_out[z + j * out_width + x * out_width * out_height] - mean_value[x];
				dev += diff * diff;
			}
		}
		stddev[x] = sqrtf(dev / (out_height * out_width));
	}
	
}

__global__ void Copy(float* shortcut, float* M_out, int width, int height, int channels) {
	int x = threadIdx.x + blockDim.x * blockIdx.x;
	int y = threadIdx.y + blockDim.y * blockIdx.y;
	int z = threadIdx.z + blockDim.z * blockIdx.z;

	if (x < width && y < height && z < channels) {
		shortcut[x + y * width + z * width * height] = M_out[x + y * width + z * width * height];
	}
}

//数据随机初始化
__global__ void Initialize(float *M, int width, int height, int channel, int dim){
	int x = threadIdx.x + blockDim.x * blockIdx.x;
	int y = threadIdx.y + blockDim.y * blockIdx.y;
	int z = threadIdx.z + blockDim.z * blockIdx.z;
	if (x < width && y < height && z < channel) {
        int seed = x + y * width + z * height * width;
        curandState_t state;
        curand_init(seed, x, 0, &state);
        for (int i = 0; i < dim; i++) {
            M[x + y * width + z * height * width + i * width * height * channel] = curand_normal(&state);
        }
    }
}

__global__ void add(float *M,float *res,float *M_out,int width,int height,int channel){
	int x = threadIdx.x + blockDim.x * blockIdx.x;
	int y = threadIdx.y + blockDim.y * blockIdx.y;
	int z = threadIdx.z + blockDim.z * blockIdx.z;

	if (x < width && y < height && z < channel){
		M_out[x + y * width + z * height * width] = M[x + y * width + z * height * width] + res[x + y * width + z * height * width];
	}
}

__global__ void intTofloat(int64_t *M_int,float *M,int width,int height,int channel){
	int x = threadIdx.x + blockDim.x * blockIdx.x;
	int y = threadIdx.y + blockDim.y * blockIdx.y;
	int z = threadIdx.z + blockDim.z * blockIdx.z;
	float time=1e4;
	if (x < width && y < height && z < channel){
		M[x + y * width + z * height * width] = M_int[x + y * width + z * height * width] / time;
	}
}

__global__ void floatToint(int64_t *M_int,float *M,int width,int height,int channel){
	int x = threadIdx.x + blockDim.x * blockIdx.x;
	int y = threadIdx.y + blockDim.y * blockIdx.y;
	int z = threadIdx.z + blockDim.z * blockIdx.z;
	float time=1e4;
	if (x < width && y < height && z < channel){
		M_int[x + y * width + z * height * width] = M[x + y * width + z * height * width] * time;
	}
}