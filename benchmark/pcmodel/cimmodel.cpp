#include <cstdio>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <fstream>
#include <iostream>

#include "apis_c.h"

#define CIM_ROW  16
#define CIM_COL  4
#define MAX_NUM  64

int idX, idY;

void random_init(int8_t * data, int size){
    int i;
    for (i = 0; i < size; i++){
        data[i] = rand();
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

    uint8_t test_num = 4;

    int8_t *input  = (int8_t*)malloc(512*test_num);
    int8_t *weight = (int8_t*)malloc(256*512);
    int8_t *output1 = (int8_t*)malloc(256*test_num);
    int8_t *output2 = (int8_t*)malloc(256*test_num);
    int8_t *output3 = (int8_t*)malloc(256*test_num);
    int8_t *output4 = (int8_t*)malloc(256*test_num);

    random_init(input, 512*test_num);     
    random_init(weight, 256*512);

    InterChiplet::sendMessage(0, 0, idX, idY, input, 512*test_num * sizeof(int8_t));
    InterChiplet::sendMessage(0, 1, idX, idY, input, 512*test_num * sizeof(int8_t));
    InterChiplet::sendMessage(1, 0, idX, idY, input, 512*test_num * sizeof(int8_t));
    InterChiplet::sendMessage(1, 1, idX, idY, input, 512*test_num * sizeof(int8_t));

    InterChiplet::sendMessage(0, 0, idX, idY, weight, 256*512 * sizeof(int8_t));
    InterChiplet::sendMessage(0, 1, idX, idY, weight, 256*512 * sizeof(int8_t));
    InterChiplet::sendMessage(1, 0, idX, idY, weight, 256*512 * sizeof(int8_t));
    InterChiplet::sendMessage(1, 1, idX, idY, weight, 256*512 * sizeof(int8_t));

    InterChiplet::receiveMessage(idX, idY, 0, 0, output1, 256*test_num * sizeof(int8_t));
    InterChiplet::receiveMessage(idX, idY, 0, 1, output2, 256*test_num * sizeof(int8_t));
    InterChiplet::receiveMessage(idX, idY, 1, 0, output3, 256*test_num * sizeof(int8_t));
    InterChiplet::receiveMessage(idX, idY, 1, 1, output4, 256*test_num * sizeof(int8_t));

    for (int i = 0; i < 256*test_num; i++) {
        output1[i] += output2[i];
        output1[i] += output3[i];
        output1[i] += output4[i];
    }

    write_int8_txt("./output_int8.txt", output1, 256*test_num);

    free(input);
    free(weight);
    free(output1);
    free(output2);
    free(output3);
    free(output4);
    return 0;

}
