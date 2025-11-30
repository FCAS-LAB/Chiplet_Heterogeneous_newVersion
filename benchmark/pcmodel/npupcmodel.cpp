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

#define MAX_POINT_NUM  1024
#define OUTCH_NUM 1
#define INCH_NUM 3
#define RANGE 1
#define CFG_CNN_SIZE_X 3
#define CFG_CNN_SIZE_Y 3
#define CFG_CNN_SIZE_Z 3
#define WT_DATA_SIZE 32*4

InterChiplet::PipeComm global_pipe_comm;
/**
 * 核心函数，由每个线程都会运行一次本函数，
 * 根据线程编号不同计算出位于结果矩阵不同位置的数据。
 */
void sconv_model(
    int idX,
    int idY,
    const uint16_t *point_x,  
    const uint16_t *point_y, 
    const uint16_t *point_z,
    const int8_t  *in_data,
    const int8_t  *wet_data,
    int8_t *out_data,
    int point_num,
    int inch_loop_num,
    int shift_num 
){
    int i = idX * 3 + idY;
    assert(i<MAX_POINT_NUM);
    int32_t *out_data_golden_32  = (int32_t*)malloc(point_num * OUTCH_NUM * sizeof(int32_t));
    for(int och = 0; och < OUTCH_NUM; och++){
        out_data_golden_32[i*OUTCH_NUM + och] = 0;
    }
    for (int j = 0; j < point_num; j++){
        uint8_t kx = point_x[j] - point_x[i] + RANGE ;
        uint8_t ky = point_y[j] - point_y[i] + RANGE ;
        uint8_t kz = point_z[j] - point_z[i] + RANGE ;
        if( kx >= 0 && kx <= 2*RANGE && 
            ky >= 0 && ky <= 2*RANGE && 
            kz >= 0 && kz <= 2*RANGE ) {
            // printf("INTO Compute, i = %d, j  = %d, kx=%d, ky=%d, kz=%d\n", i ,j, kx,ky,kz);
            for (int och = 0; och < OUTCH_NUM; och++){
                for(int ich_loop = 0; ich_loop < inch_loop_num + 1; ich_loop++){
                    // printf("ich_loop = %d \n", ich_loop);
                    int wt_index =  (kx + ky * CFG_CNN_SIZE_X + kz * CFG_CNN_SIZE_Y * CFG_CNN_SIZE_Z) + ich_loop * CFG_CNN_SIZE_Z * CFG_CNN_SIZE_Y * CFG_CNN_SIZE_X;
                    for (int ich = 0; ich < INCH_NUM; ich++){
                        out_data_golden_32[i*OUTCH_NUM + och] +=  wet_data[(och*WT_DATA_SIZE+wt_index)*INCH_NUM + ich]* in_data[(j + ich_loop * point_num)*INCH_NUM + ich];
                    }
                }
            }
        }
    }
    for(int och = 0; och < OUTCH_NUM; och++){
        int32_t temp = out_data_golden_32[i*OUTCH_NUM + och] >> shift_num;
        if(temp > 127)
            out_data[i*OUTCH_NUM + och] = 127;
        else if(temp < -128) 
            out_data[i*OUTCH_NUM + och] = -128;
        else 
            out_data[i*OUTCH_NUM + och] = temp;
    }   

}

int main(int argc, char** argv) {
    // 读取本进程所代表的chiplet编号

    int idX = atoi(argv[1]);
    int idY = atoi(argv[2]);
    int  point_num = 3;
    int  shift_num = 10; 
    int inch_loop_num = 3;
    uint16_t *point_x  = (uint16_t*)malloc(point_num * sizeof(uint16_t));
    uint16_t *point_y  = (uint16_t*)malloc(point_num * sizeof(uint16_t));
    uint16_t *point_z  = (uint16_t*)malloc(point_num * sizeof(uint16_t));
    int8_t *in_data = (int8_t*)malloc(point_num*(inch_loop_num+1)*INCH_NUM * sizeof(int8_t));
    int8_t *wet_data = (int8_t*)malloc(WT_DATA_SIZE*OUTCH_NUM*INCH_NUM*CFG_CNN_SIZE_Z*CFG_CNN_SIZE_Y*CFG_CNN_SIZE_X * sizeof(int8_t));
    int8_t *out_data = (int8_t*)malloc(point_num*OUTCH_NUM * sizeof(int8_t));

    memset(out_data, 0, sizeof(int8_t) * point_num*OUTCH_NUM);

    long long unsigned int time_end = 1;
    std::string fileName = InterChiplet::receiveSync(2, 2, idX, idY);
    global_pipe_comm.read_data(fileName.c_str(), point_x, point_num * sizeof(uint16_t));
    time_end = InterChiplet::readSync(time_end, 2, 2, idX, idY, point_num * sizeof(uint16_t), 0);

    fileName = InterChiplet::receiveSync(2, 2, idX, idY);
    global_pipe_comm.read_data(fileName.c_str(), point_y, point_num * sizeof(uint16_t));
    time_end = InterChiplet::readSync(time_end, 2, 2, idX, idY, point_num * sizeof(uint16_t), 0);

    fileName = InterChiplet::receiveSync(2, 2, idX, idY);
    global_pipe_comm.read_data(fileName.c_str(), point_z, point_num * sizeof(uint16_t));
    time_end = InterChiplet::readSync(time_end, 2, 2, idX, idY, point_num * sizeof(uint16_t), 0);

    fileName = InterChiplet::receiveSync(2, 2, idX, idY);
    global_pipe_comm.read_data(fileName.c_str(), in_data, sizeof(int8_t) * point_num*(inch_loop_num+1)*INCH_NUM);
    time_end = InterChiplet::readSync(time_end, 2, 2, idX, idY, sizeof(int8_t) * point_num*(inch_loop_num+1)*INCH_NUM, 0);

    fileName = InterChiplet::receiveSync(2, 2, idX, idY);
    global_pipe_comm.read_data(fileName.c_str(), wet_data, sizeof(int8_t) * WT_DATA_SIZE*OUTCH_NUM*INCH_NUM*CFG_CNN_SIZE_Z*CFG_CNN_SIZE_Y*CFG_CNN_SIZE_X);
    time_end = InterChiplet::readSync(time_end, 2, 2, idX, idY, sizeof(int8_t) * WT_DATA_SIZE*OUTCH_NUM*INCH_NUM*CFG_CNN_SIZE_Z*CFG_CNN_SIZE_Y*CFG_CNN_SIZE_X, 0);

    uint32_t frequency = 10;
    uint8_t ddr_row = 69;
    uint8_t ddr_column = 100;
    int random = rand();
    int data_size = (random % ddr_row*ddr_column) / (8 * frequency * sizeof(int8_t));
    int8_t *ddr_data = (int8_t*)malloc((data_size+1) * sizeof(int8_t));
    InterChiplet::launchSync(idX, idY, 2, 1);
    time_end = InterChiplet::writeSync(time_end, idX, idY, 2, 1, 1, InterChiplet::SPD_LAUNCH);

    fileName = InterChiplet::sendSync(idX, idY, 2, 1);
    global_pipe_comm.write_data(fileName.c_str(), &random, sizeof(int));
    time_end = InterChiplet::writeSync(time_end, idX, idY, 2, 1, sizeof(int), 0);

    fileName = InterChiplet::receiveSync(2, 1, idX, idY);
    global_pipe_comm.read_data(fileName.c_str(), ddr_data, (data_size+1) * sizeof(int8_t));
    time_end = InterChiplet::readSync(time_end, 2, 1, idX, idY, (data_size+1) * sizeof(int8_t), 0);

    // calculate
    sconv_model(
        idX,
        idY,
        point_x,  
        point_y, 
        point_z,
        in_data,
        wet_data,
        out_data,
        point_num,
        inch_loop_num,
        shift_num 
    );

    fileName = InterChiplet::sendSync(idX, idY, 2, 2);
    global_pipe_comm.write_data(fileName.c_str(), out_data, sizeof(int8_t) * point_num*OUTCH_NUM);
    std::cout << "simulator power of cost " << ":" << sizeof(int8_t) * point_num*OUTCH_NUM*8*0.024 << "pJ" << std::endl;
    InterChiplet::writeSync(time_end + point_num*(inch_loop_num+1)*INCH_NUM/512, idX, idY, 2, 2, sizeof(int8_t) * point_num*OUTCH_NUM, 0);
    
    free(point_x);
    free(point_y);
    free(point_z);
    free(in_data);
    free(wet_data);
    free(out_data);
    return 0;
}
