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
#include "apis_cu.h"
#include <thrust/extrema.h>
#include "resnet.cuh"

#define THREADSIZE 10

using namespace std;


void conv_block(float* M1, float* M1_out, float* kernel, int width, int height, int in_channels, int out_width, int out_height, int out_channels = 56, int padding = 3, int stride = 2, int kernel_size = 1) {
	//float* M1, * M1_out;

	// cudaMalloc((void**)&M1, sizeof(float) * width * height * in_channels);
	// cudaMalloc((void**)&M1_out, sizeof(float) * out_width * out_height * out_channels);
	dim3 threadPerblock(THREADSIZE, THREADSIZE, THREADSIZE);
	dim3 blockPerGrid((width + threadPerblock.x - 1) / threadPerblock.x,
		(height + threadPerblock.y - 1) / threadPerblock.y,
		(in_channels + threadPerblock.z - 1) / threadPerblock.z);
	// 使用第一个卷积层进行特征提取
	conv << <threadPerblock, blockPerGrid >> > (M1, kernel, M1_out, width, height, in_channels, out_channels, kernel_size, stride, padding);
	cudaDeviceSynchronize();

	float* mean_value, * stddev;
	cudaMalloc((void**)&mean_value, sizeof(float) * out_channels);
	cudaMalloc((void**)&stddev, sizeof(float) * out_channels);

	dim3 threadPerblock_2(out_channels);
	dim3 blockPerGrid_2(1);
	compute_mean_stddev << <threadPerblock_2, blockPerGrid_2 >> > (M1_out, mean_value, stddev, out_width, out_height, out_channels);

	cudaDeviceSynchronize();
	dim3 blockPerGrid3((out_width + threadPerblock.x - 1) / threadPerblock.x,
		(out_height + threadPerblock.y - 1) / threadPerblock.y,
		(out_channels + threadPerblock.z - 1) / threadPerblock.z);
	// 将结果经过批量归一化和ReLU激活函数
	BatchNormed << <threadPerblock, blockPerGrid3 >> > (out_channels, width, height, M1_out, mean_value, stddev);
	cudaDeviceSynchronize();

	ReLu << <threadPerblock, blockPerGrid3 >> > (out_channels, width, height, M1_out);
	cudaDeviceSynchronize();
	cudaFree(mean_value);
	cudaFree(stddev);
}

//残差块
void bnk1(float* M1, float* M_out, int width, int height, int in_channels, int out_channels,int padding, int stride = 2) {
	float* kernel;
	int kernel_size_first = 1;
	int out_channels_first = out_channels/4;
	cudaMalloc((void**)&kernel, sizeof(float) * in_channels * kernel_size_first * kernel_size_first * out_channels_first);
	int out_width = (width - kernel_size_first + 2 * padding) / stride + 1;
	int out_height = (width - kernel_size_first + 2 * padding) / stride + 1;
	
	float* M1_out;
	cudaMalloc((void**)&M1_out, sizeof(float) * out_width * out_height * out_channels_first);
	dim3 threadPerblock(THREADSIZE, THREADSIZE, THREADSIZE);
	dim3 blockPerGrid((kernel_size_first + threadPerblock.x - 1) / threadPerblock.x,
		(kernel_size_first + threadPerblock.y - 1) / threadPerblock.y,
		(in_channels + threadPerblock.z - 1) / threadPerblock.z);
	Initialize << <threadPerblock, blockPerGrid >> > (kernel, kernel_size_first, kernel_size_first, in_channels,out_channels_first);
	cudaDeviceSynchronize();
	// conv_block(M1, M1_out, kernel, width, height, in_channels, out_width, out_height, out_channels_first, padding, stride, kernel_size_first);
	cudaFree(kernel);
	//
	cout<<"###############################################################################################"<<endl;
	cout<<"##################################### check point first ########################################"<<endl;
	cout<<"###############################################################################################"<<endl;
	int kernel_size_res = 1;
	int out_channels_res = out_channels;
	cudaMalloc((void**)&kernel, sizeof(float) * in_channels * kernel_size_res * kernel_size_res * out_channels_res);
	out_width = (width - kernel_size_res + 2 * padding) / stride + 1;
	out_height = (width - kernel_size_res + 2 * padding) / stride + 1;
	
	float* Mres_out;
	cudaMalloc((void**)&Mres_out, sizeof(float) * out_width * out_height * out_channels_res);
	dim3 blockPerGrid_res((kernel_size_res + threadPerblock.x - 1) / threadPerblock.x,
		(kernel_size_res + threadPerblock.y - 1) / threadPerblock.y,
		(in_channels + threadPerblock.z - 1) / threadPerblock.z);
	
	Initialize << <threadPerblock, blockPerGrid_res >> > (kernel, kernel_size_res, kernel_size_res, in_channels,out_channels_res);
	cudaDeviceSynchronize();
	// conv_block(M1, Mres_out, kernel, width, height, in_channels, out_width, out_height, out_channels_res, padding, stride, kernel_size_res);
	cudaFree(kernel);
	//
	cout<<"###############################################################################################"<<endl;
	cout<<"##################################### check point two ########################################"<<endl;
	cout<<"###############################################################################################"<<endl;
	width = out_width; height = out_height; in_channels = out_channels_first;
	int out_channels_sec = out_channels/4;
	int kernel_size_sec = 3;
	padding = width/2+1;
	cudaMalloc((void**)&kernel, sizeof(float) * in_channels * kernel_size_sec * kernel_size_sec * out_channels_sec);
	out_width = (width - kernel_size_sec + 2 * padding) / stride + 1;
	out_height = (width - kernel_size_sec + 2 * padding) / stride + 1;
	float* M2_out;
	cudaMalloc((void**)&M2_out, sizeof(float) * out_width * out_height * out_channels_sec);
	dim3 blockPerGrid2((kernel_size_sec + threadPerblock.x - 1) / threadPerblock.x,
		(kernel_size_sec + threadPerblock.y - 1) / threadPerblock.y,
		(in_channels + threadPerblock.z - 1) / threadPerblock.z);
	Initialize << <threadPerblock, blockPerGrid2 >> > (kernel, kernel_size_sec, kernel_size_sec, in_channels,out_channels_sec);
	cudaDeviceSynchronize();
	// conv_block(M1_out, M2_out, kernel, width, height, in_channels, out_width, out_height, out_channels_sec, padding, stride, kernel_size_sec);
	cudaFree(kernel);
	//
	cout<<"###############################################################################################"<<endl;
	cout<<"##################################### check point three ########################################"<<endl;
	cout<<"###############################################################################################"<<endl;
	width = out_width; height = out_height; in_channels = out_channels_sec;
	int out_channels_third = out_channels;
	int kernel_size_third = 1;
	padding = width/2;
	cudaMalloc((void**)&kernel, sizeof(float) * in_channels * kernel_size_third * kernel_size_third * out_channels_third);
	out_width = (width - kernel_size_third + 2 * padding) / stride + 1;
	out_height = (width - kernel_size_third + 2 * padding) / stride + 1;
	float* M3_out;
	cudaMalloc((void**)&M3_out, sizeof(float) * out_width * out_height * out_channels_third);
	dim3 blockPerGrid3((kernel_size_third + threadPerblock.x - 1) / threadPerblock.x,
		(kernel_size_third + threadPerblock.y - 1) / threadPerblock.y,
		(in_channels + threadPerblock.z - 1) / threadPerblock.z);
	Initialize << <threadPerblock, blockPerGrid3 >> > (kernel, kernel_size_third, kernel_size_third, in_channels,out_channels_third);
	cudaDeviceSynchronize();
	// conv_block(M2_out, M3_out, kernel, width, height, in_channels, out_width, out_height, out_channels_third, padding, stride, kernel_size_third);
	cudaFree(kernel);
	cout<<"###############################################################################################"<<endl;
	cout<<"##################################### check point four ########################################"<<endl;
	cout<<"###############################################################################################"<<endl;
	width = out_width; height = out_height; in_channels = out_channels_third;
	dim3 blockPerGrid4((width + threadPerblock.x - 1) / threadPerblock.x,
		(height + threadPerblock.y - 1) / threadPerblock.y,
		(out_channels_third + threadPerblock.z - 1) / threadPerblock.z);
	add<<<threadPerblock,blockPerGrid4>>>(M3_out,Mres_out,M_out,width,height,in_channels);
	cudaFree(M1_out);
	cudaFree(Mres_out);
	cudaFree(M2_out);
	cudaFree(M3_out);
}

void bnk2(float* M1, float* M_out, int width, int height, int in_channels, int stride = 2) {
	cout<<"###############################################################################################"<<endl;
	cout<<"##################################### check point five ########################################"<<endl;
	cout<<"###############################################################################################"<<endl;
	int padding=width/2;
	float* kernel;
	int kernel_size_first = 1;
	int out_channels_first = in_channels/4;
	cudaMalloc((void**)&kernel, sizeof(float) * in_channels * kernel_size_first * kernel_size_first * out_channels_first);
	int out_width = (width - kernel_size_first + 2 * padding) / stride + 1;
	int out_height = (width - kernel_size_first + 2 * padding) / stride + 1;
	
	float* M1_out;
	cudaMalloc((void**)&M1_out, sizeof(float) * out_width * out_height * out_channels_first);
	dim3 threadPerblock(THREADSIZE, THREADSIZE, THREADSIZE);
	dim3 blockPerGrid((kernel_size_first + threadPerblock.x - 1) / threadPerblock.x,
		(kernel_size_first + threadPerblock.y - 1) / threadPerblock.y,
		(in_channels + threadPerblock.z - 1) / threadPerblock.z);
	Initialize << <threadPerblock, blockPerGrid >> > (kernel, kernel_size_first, kernel_size_first, in_channels,out_channels_first);
	cudaDeviceSynchronize();
	// conv_block(M1, M1_out, kernel, width, height, in_channels, out_width, out_height, out_channels_first, padding, stride, kernel_size_first);

	//
	cout<<"###############################################################################################"<<endl;
	cout<<"##################################### check point six ########################################"<<endl;
	cout<<"###############################################################################################"<<endl;
	int kernel_size_res = 1;
	int out_channels_res = in_channels;

	out_width = (width - kernel_size_res + 2 * padding) / stride + 1;
	out_height = (width - kernel_size_res + 2 * padding) / stride + 1;
	
	float* Mres_out;
	cudaMalloc((void**)&Mres_out, sizeof(float) * out_width * out_height * out_channels_res);
	dim3 blockPerGrid_res((kernel_size_res + threadPerblock.x - 1) / threadPerblock.x,
		(kernel_size_res + threadPerblock.y - 1) / threadPerblock.y,
		(in_channels + threadPerblock.z - 1) / threadPerblock.z);
	Initialize << <threadPerblock, blockPerGrid_res >> > (kernel, kernel_size_res, kernel_size_res, in_channels,out_channels_res);
	cudaDeviceSynchronize();
	ReLu << <threadPerblock, blockPerGrid_res >> > (in_channels, width, height, M1);
	cudaDeviceSynchronize();
	cudaFree(kernel);
	//
	cout<<"###############################################################################################"<<endl;
	cout<<"##################################### check point seven ########################################"<<endl;
	cout<<"###############################################################################################"<<endl;
	width = out_width; height = out_height; in_channels = out_channels_first;
	int out_channels_sec = in_channels;
	int kernel_size_sec = 3;
	padding = padding+1;
	cudaMalloc((void**)&kernel, sizeof(float) * in_channels * kernel_size_sec * kernel_size_sec * out_channels_sec);
	out_width = (width - kernel_size_sec + 2 * padding) / stride + 1;
	out_height = (width - kernel_size_sec + 2 * padding) / stride + 1;
	float* M2_out;
	cudaMalloc((void**)&M2_out, sizeof(float) * out_width * out_height * out_channels_sec);
	dim3 blockPerGrid2((kernel_size_sec + threadPerblock.x - 1) / threadPerblock.x,
		(kernel_size_sec + threadPerblock.y - 1) / threadPerblock.y,
		(in_channels + threadPerblock.z - 1) / threadPerblock.z);
	Initialize << <threadPerblock, blockPerGrid2 >> > (kernel, kernel_size_sec, kernel_size_sec, in_channels,out_channels_sec);
	cudaDeviceSynchronize();
	// conv_block(M1_out, M2_out, kernel, width, height, in_channels, out_width, out_height, out_channels_sec, padding, stride, kernel_size_sec);
	cudaFree(kernel);
	//
	cout<<"###############################################################################################"<<endl;
	cout<<"##################################### check point eight ########################################"<<endl;
	cout<<"###############################################################################################"<<endl;
	width = out_width; height = out_height; in_channels = out_channels_sec;
	int out_channels_third = in_channels*4;
	int kernel_size_third = 1;
	padding -= 1;
	cudaMalloc((void**)&kernel, sizeof(float) * in_channels * kernel_size_third * kernel_size_third * out_channels_third);
	out_width = (width - kernel_size_third + 2 * padding) / stride + 1;
	out_height = (width - kernel_size_third + 2 * padding) / stride + 1;
	float* M3_out;
	cudaMalloc((void**)&M3_out, sizeof(float) * out_width * out_height * out_channels_third);
	dim3 blockPerGrid3((kernel_size_third + threadPerblock.x - 1) / threadPerblock.x,
		(kernel_size_third + threadPerblock.y - 1) / threadPerblock.y,
		(in_channels + threadPerblock.z - 1) / threadPerblock.z);
	Initialize << <threadPerblock, blockPerGrid3 >> > (kernel, kernel_size_third, kernel_size_third, in_channels,out_channels_third);
	cudaDeviceSynchronize();
	// conv_block(M2_out, M3_out, kernel, width, height, in_channels, out_width, out_height, out_channels_third, padding, stride, kernel_size_third);
	cudaFree(kernel);

	cout<<"###############################################################################################"<<endl;
	cout<<"##################################### check point nine ########################################"<<endl;
	cout<<"###############################################################################################"<<endl;
	width = out_width; height = out_height; in_channels = out_channels_third;
	dim3 blockPerGrid4((width + threadPerblock.x - 1) / threadPerblock.x,
		(height + threadPerblock.y - 1) / threadPerblock.y,
		(out_channels_third + threadPerblock.z - 1) / threadPerblock.z);
	add<<<threadPerblock,blockPerGrid4>>>(M3_out,M1,M_out,width,height,in_channels);
	cout<<"###############################################################################################"<<endl;
	cout<<"##################################### check point ten ########################################"<<endl;
	cout<<"###############################################################################################"<<endl;
	cudaFree(M1_out);
	cudaFree(Mres_out);
	cudaFree(M2_out);
	cudaFree(M3_out);
}

int main(int argc, char** argv) {
	int idX = atoi(argv[1]);
	int idY = atoi(argv[2]);
	int batch = atoi(argv[3]);
	int width = 0, height = 0, channels = 0;
	int out_width = 0, out_height = 0, out_channels = 0;
	if (idY == 1){
		width = 56;
		height = 56;
		channels = 64;
		out_width = 56;
		out_height = 56;
		out_channels = 256;
	}
	else if (idY == 2){
		width = 56;
		height = 56;
		channels = 256;
		out_width = 28;
		out_height = 28;
		out_channels = 512;
	}
	else if (idY == 3){
		width = 28;
		height = 28;
		channels = 512;
		out_width = 14;
		out_height = 14;
		out_channels = 1024;
	}
	else if (idY == 4){
		width = 14;
		height = 14;
		channels = 1024;
		out_width = 7;
		out_height = 7;
		out_channels = 2048;
	}
	else{
		return 1;
	}
	float *M, *M_out;
	int64_t *M_int,*M_out_int;
	cudaMalloc((void **)&M_int, sizeof(int64_t) * width * height * channels);
	cudaMalloc((void **)&M, sizeof(float) * width * height * channels);
	cudaMalloc((void **)&M_out, sizeof(float) * out_width * out_height * out_channels);
	cudaMalloc((void **)&M_out_int, sizeof(int64_t) * out_width * out_height * out_channels);
	for (int i = 0; i < batch; i++)
	{
		receiveMessage(idX, idY, 5, 5, M_int, width * height * channels*sizeof(int64_t));
		dim3 threadPerBlock(THREADSIZE, THREADSIZE, THREADSIZE);
		dim3 blockPerGrid((width + threadPerBlock.x - 1) / threadPerBlock.x,
						  (height + threadPerBlock.y - 1) / threadPerBlock.y,
						  (channels + threadPerBlock.z - 1) / threadPerBlock.z);

		intTofloat<<<threadPerBlock, blockPerGrid>>>(M_int, M, width, height, channels);
		if (idY == 1)
		{
			bnk1(M, M_out, width, height, channels, out_channels, width / 2);
			float *M_out_sec;
			cudaMalloc((void **)&M_out_sec, sizeof(float) * out_width * out_height * out_channels);
			bnk2(M_out, M_out_sec, out_width, out_height, out_channels);
			bnk2(M_out_sec, M_out, out_width, out_height, out_channels);
			cudaFree(M_out_sec);
		}
		else if (idY == 2)
		{
			bnk1(M, M_out, width, height, channels, out_channels, 0);
			float *M_out_sec;
			cudaMalloc((void **)&M_out_sec, sizeof(float) * out_width * out_height * out_channels);
			float *M_out_third;
			cudaMalloc((void **)&M_out_third, sizeof(float) * out_width * out_height * out_channels);
			bnk2(M_out, M_out_sec, out_width, out_height, out_channels);
			bnk2(M_out_sec, M_out_third, out_width, out_height, out_channels);
			bnk2(M_out_third, M_out, out_width, out_height, out_channels);
			cudaFree(M_out_sec);
			cudaFree(M_out_third);
		}
		else if (idY == 3)
		{
			bnk1(M, M_out, width, height, channels, out_channels, 0);
			float *M_out_sec, *M_out_third, *M_out_forth, *M_out_fifth;
			cudaMalloc((void **)&M_out_sec, sizeof(float) * out_width * out_height * out_channels);
			cudaMalloc((void **)&M_out_third, sizeof(float) * out_width * out_height * out_channels);
			cudaMalloc((void **)&M_out_forth, sizeof(float) * out_width * out_height * out_channels);
			cudaMalloc((void **)&M_out_fifth, sizeof(float) * out_width * out_height * out_channels);
			bnk2(M_out, M_out_sec, out_width, out_height, out_channels);
			bnk2(M_out_sec, M_out_third, out_width, out_height, out_channels);
			bnk2(M_out_third, M_out_forth, out_width, out_height, out_channels);
			bnk2(M_out_forth, M_out_fifth, out_width, out_height, out_channels);
			bnk2(M_out_fifth, M_out, out_width, out_height, out_channels);
			cudaFree(M_out_sec);
			cudaFree(M_out_third);
			cudaFree(M_out_forth);
			cudaFree(M_out_fifth);
		}
		else if (idY == 4)
		{
			bnk1(M, M_out, width, height, channels, out_channels, 0);
			float *M_out_sec;
			cudaMalloc((void **)&M_out_sec, sizeof(float) * out_width * out_height * out_channels);
			bnk2(M_out, M_out_sec, out_width, out_height, out_channels);
			bnk2(M_out_sec, M_out, out_width, out_height, out_channels);
			cudaFree(M_out_sec);
		}

		dim3 blockPerGrid2((out_width + threadPerBlock.x - 1) / threadPerBlock.x,
						   (out_height + threadPerBlock.y - 1) / threadPerBlock.y,
						   (out_channels + threadPerBlock.z - 1) / threadPerBlock.z);
		floatToint<<<threadPerBlock, blockPerGrid2>>>(M_out_int, M_out, out_width, out_height, out_channels);
		
		sendMessage(5, 5, idX, idY, M_out_int, out_width * out_height * out_channels*sizeof(int64_t));
		
	}
	cudaFree(M);
	cudaFree(M_out);
}