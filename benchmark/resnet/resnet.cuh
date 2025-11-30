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
__global__ void conv(float* M1, float* kernel, float* M_out, int width, int height, int in_channels, int out_channels, int kernel_size, int stride, int padding = 3);
__global__ void BatchNormed(int in_channels, int width, int height, float* M1, float* mean_value, float* stddev);
__global__ void ReLu(int in_channels, int width, int height, float* M1);
__global__ void Maxpool(int in_channels, int width, int height, int out_width, int out_height, float* M1, float* M1_out, int kernel_size, int stride);
__global__ void compute_mean_stddev(float* M1_out, float* mean_value, float* stddev, int out_width, int out_height, int out_channels);
__global__ void Copy(float* shortcut, float* M_out, int width, int height, int channels);
__global__ void Initialize(float *M, int width, int height, int channel, int dim);
__global__ void add(float *M,float *res,float *M_out,int width,int height,int channel);
__global__ void intTofloat(int64_t *M_int,float *M,int width,int height,int channel);
__global__ void floatToint(int64_t *M_int,float *M,int width,int height,int channel);