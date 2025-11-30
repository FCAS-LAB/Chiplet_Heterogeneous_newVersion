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

#define CIM_ROW  16
#define CIM_COL  4
#define MAX_NUM  64

InterChiplet::PipeComm global_pipe_comm;
/**
 * 核心函数，由每个线程都会运行一次本函数，
 * 根据线程编号不同计算出位于结果矩阵不同位置的数据。
 */

void cim_model(
    int idX,
    int idY,
    const int8_t *input,  //input: 512*TEST_NUM
    const int8_t *weight, //weight: 256*512
    int8_t  *output, //output: 256*TEST_NUM
    uint8_t shift_num 
){
    int num = idX * 2 + idY;
    assert(num<MAX_NUM);
    int32_t golden_out_data_full[256*MAX_NUM];
    for (int  row = 0; row < 16*CIM_ROW; row++) {
        uint32_t index = num * 16 * CIM_ROW + row;
        golden_out_data_full[index] = 0;  
        for ( int i = 0; i < CIM_COL; i++) {
            for ( int j = 0; j < 128; j ++) {
                golden_out_data_full[index] += weight[row * 128 * CIM_COL + 128*i + j] * input[num*CIM_COL*128 + i * 128 + j]; 
            }
        }
    }

    int32_t golden_out_data_shift[256*MAX_NUM];
    for (int row = 0; row < 16*CIM_ROW; row++) {
        uint32_t index = num * 16 * CIM_ROW + row;
        golden_out_data_shift[index] = golden_out_data_full[index] >> shift_num;// 
        if(golden_out_data_shift[index] > 127) {
            output[index]  = 127;
        }else if(golden_out_data_shift[index] < -128){
            output[index]  = -128;
        }else {
            output[index]  = golden_out_data_shift[index] ;
        }
    }
}

int main(int argc, char** argv) {
    // 读取本进程所代表的chiplet编号

    int idX = atoi(argv[1]);
    int idY = atoi(argv[2]);
    uint8_t test_num = 3;
    uint8_t shift_num = 10; 

    int8_t *input  = (int8_t*)malloc(512*test_num);
    int8_t *weight = (int8_t*)malloc(256*512);
    int8_t *output = (int8_t*)malloc(256*test_num);

    memset(output, 0, sizeof(int8_t) * 256*test_num);

    long long unsigned int timeNow = 1;
    std::string fileName = InterChiplet::receiveSync(2, 2, idX, idY);
    global_pipe_comm.read_data(fileName.c_str(), input, sizeof(int8_t) * 512*test_num);
    long long int time_end = InterChiplet::readSync(timeNow, 2, 2, idX, idY, sizeof(int8_t) * 512*test_num, 0);

    fileName = InterChiplet::receiveSync(2, 2, idX, idY);
    global_pipe_comm.read_data(fileName.c_str(), weight, sizeof(int8_t) * 256*512);
    time_end = InterChiplet::readSync(time_end, 2, 2, idX, idY, sizeof(int8_t) * 256*512, 0);

    uint32_t frequency = 10;
    uint8_t ddr_row = 3;
    uint8_t ddr_column = 3;
    int random = rand();
    int data_size = (random % ddr_row*ddr_column) / (8 * frequency * sizeof(int8_t));
    int8_t *ddr_data = (int8_t*)malloc((data_size+1) * sizeof(int8_t));
    InterChiplet::launchSync(idX, idY, 2, 0);
    time_end = InterChiplet::writeSync(time_end, idX, idY, 2, 0, 1, InterChiplet::SPD_LAUNCH);

    fileName = InterChiplet::sendSync(idX, idY, 2, 0);
    global_pipe_comm.write_data(fileName.c_str(), &random, sizeof(int));
    time_end = InterChiplet::writeSync(time_end, idX, idY, 2, 0, sizeof(int), 0);

    fileName = InterChiplet::receiveSync(2, 0, idX, idY);
    global_pipe_comm.read_data(fileName.c_str(), ddr_data, (data_size+1) * sizeof(int8_t));
    time_end = InterChiplet::readSync(time_end, 2, 0, idX, idY, (data_size+1) * sizeof(int8_t), 0);

    // calculate
    cim_model(
        idX,
        idY,
        input,  
        weight, 
        output, 
        shift_num         
    );

    fileName = InterChiplet::sendSync(idX, idY, 2, 2);
    global_pipe_comm.write_data(fileName.c_str(), output, sizeof(int8_t) * 256*test_num);
    std::cout << "simulator power of cost " << ":" << sizeof(int8_t) * 256*test_num*8*0.024 << "pJ" << std::endl;

    InterChiplet::writeSync(time_end + 512*test_num/512, idX, idY, 2, 2, sizeof(int8_t) * 256*test_num, 0);
    
    free(input);
    free(weight);
    free(output);
    return 0;
}
