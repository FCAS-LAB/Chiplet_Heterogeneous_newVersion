#include <cstdio>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <fstream>
#include <iostream>

#include "apis_c.h"

#define MAX_POINT_NUM  1024
#define OUTCH_NUM 1
#define INCH_NUM 3
#define RANGE 1
#define CFG_CNN_SIZE_X 3
#define CFG_CNN_SIZE_Y 3
#define CFG_CNN_SIZE_Z 3
#define WT_DATA_SIZE 32*4

int idX, idY;

void random_init(int8_t * data, int size){
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

int main(int argc, char **argv) {
    idX = atoi(argv[1]);
    idY = atoi(argv[2]);

    uint8_t test_num = 3;

    int8_t *input  = (int8_t*)malloc(512*test_num);
    int8_t *weight = (int8_t*)malloc(256*512);
    int8_t *output0 = (int8_t*)malloc(256*test_num);
    int8_t *output1 = (int8_t*)malloc(256*test_num);
    int8_t *output2 = (int8_t*)malloc(256*test_num);

    random_init(input, 512*test_num);
    random_init(weight, 256*512);

    int  point_num = 3;
    int inch_loop_num = 3;
    int max_value = 50;
    uint16_t *point_x  = (uint16_t*)malloc(point_num * sizeof(uint16_t));
    uint16_t *point_y  = (uint16_t*)malloc(point_num * sizeof(uint16_t));
    uint16_t *point_z  = (uint16_t*)malloc(point_num * sizeof(uint16_t));
    int8_t *in_data = (int8_t*)malloc(point_num*(inch_loop_num+1)*INCH_NUM * sizeof(int8_t));
    int8_t *wet_data = (int8_t*)malloc(WT_DATA_SIZE*OUTCH_NUM*INCH_NUM*CFG_CNN_SIZE_Z*CFG_CNN_SIZE_Y*CFG_CNN_SIZE_X * sizeof(int8_t));

    random_init_uint16(point_x, point_num, max_value);
    random_init_uint16(point_y, point_num, max_value);
    random_init_uint16(point_z, point_num, max_value);

    random_init(in_data, point_num*(inch_loop_num+1)*INCH_NUM);
    random_init(wet_data, WT_DATA_SIZE*OUTCH_NUM*INCH_NUM*CFG_CNN_SIZE_Z*CFG_CNN_SIZE_Y*CFG_CNN_SIZE_X);

    int8_t *output3 = (int8_t*)malloc(point_num*OUTCH_NUM * sizeof(int8_t));
    int8_t *output4 = (int8_t*)malloc(point_num*OUTCH_NUM * sizeof(int8_t));
    int8_t *output5 = (int8_t*)malloc(point_num*OUTCH_NUM * sizeof(int8_t));

    InterChiplet::sendMessage(1, 1, idX, idY, point_x, point_num * sizeof(uint16_t));
    InterChiplet::sendMessage(0, 2, idX, idY, point_x, point_num * sizeof(uint16_t));
    InterChiplet::sendMessage(1, 2, idX, idY, point_x, point_num * sizeof(uint16_t));

    InterChiplet::sendMessage(1, 1, idX, idY, point_y, point_num * sizeof(uint16_t));
    InterChiplet::sendMessage(0, 2, idX, idY, point_y, point_num * sizeof(uint16_t));
    InterChiplet::sendMessage(1, 2, idX, idY, point_y, point_num * sizeof(uint16_t));

    InterChiplet::sendMessage(1, 1, idX, idY, point_z, point_num * sizeof(uint16_t));
    InterChiplet::sendMessage(0, 2, idX, idY, point_z, point_num * sizeof(uint16_t));
    InterChiplet::sendMessage(1, 2, idX, idY, point_z, point_num * sizeof(uint16_t));

    InterChiplet::sendMessage(0, 0, idX, idY, input, 512*test_num * sizeof(int8_t));
    InterChiplet::sendMessage(0, 1, idX, idY, input, 512*test_num * sizeof(int8_t));
    InterChiplet::sendMessage(1, 0, idX, idY, input, 512*test_num * sizeof(int8_t));
    InterChiplet::sendMessage(1, 1, idX, idY, in_data, point_num*(inch_loop_num+1)*INCH_NUM * sizeof(int8_t));
    InterChiplet::sendMessage(0, 2, idX, idY, in_data, point_num*(inch_loop_num+1)*INCH_NUM * sizeof(int8_t));
    InterChiplet::sendMessage(1, 2, idX, idY, in_data, point_num*(inch_loop_num+1)*INCH_NUM * sizeof(int8_t));

    InterChiplet::sendMessage(0, 0, idX, idY, weight, 256*512 * sizeof(int8_t));
    InterChiplet::sendMessage(0, 1, idX, idY, weight, 256*512 * sizeof(int8_t));
    InterChiplet::sendMessage(1, 0, idX, idY, weight, 256*512 * sizeof(int8_t));
    InterChiplet::sendMessage(1, 1, idX, idY, wet_data, WT_DATA_SIZE*OUTCH_NUM*INCH_NUM*CFG_CNN_SIZE_Z*CFG_CNN_SIZE_Y*CFG_CNN_SIZE_X);
    InterChiplet::sendMessage(0, 2, idX, idY, wet_data, WT_DATA_SIZE*OUTCH_NUM*INCH_NUM*CFG_CNN_SIZE_Z*CFG_CNN_SIZE_Y*CFG_CNN_SIZE_X);
    InterChiplet::sendMessage(1, 2, idX, idY, wet_data, WT_DATA_SIZE*OUTCH_NUM*INCH_NUM*CFG_CNN_SIZE_Z*CFG_CNN_SIZE_Y*CFG_CNN_SIZE_X);

    InterChiplet::receiveMessage(idX, idY, 0, 0, output0, 256*test_num * sizeof(int8_t));
    InterChiplet::receiveMessage(idX, idY, 0, 1, output1, 256*test_num * sizeof(int8_t));
    InterChiplet::receiveMessage(idX, idY, 1, 0, output2, 256*test_num * sizeof(int8_t));
    InterChiplet::receiveMessage(idX, idY, 1, 1, output3, point_num*OUTCH_NUM * sizeof(int8_t));
    InterChiplet::receiveMessage(idX, idY, 0, 2, output4, point_num*OUTCH_NUM * sizeof(int8_t));
    InterChiplet::receiveMessage(idX, idY, 1, 2, output5, point_num*OUTCH_NUM * sizeof(int8_t));

    for (int i = 0; i < 256*test_num; i++) {
        output0[i] += output1[i];
        output0[i] += output2[i];
    }

    write_int8_txt("./output_int8.txt", output0, 256*test_num);

    for (int i = 0; i < point_num*OUTCH_NUM; i++) {
        output3[i] += output4[i];
        output3[i] += output5[i];
    }

    write_int8_txt("./pointcloud_out.txt", output3, point_num*OUTCH_NUM);

    free(input);
    free(weight);
    free(in_data);
    free(wet_data);
    free(output1);
    free(output2);
    free(output3);
    free(output4);
    free(output5);
    free(output0);
    free(point_x);
    free(point_y);
    free(point_z);
    return 0;

}
