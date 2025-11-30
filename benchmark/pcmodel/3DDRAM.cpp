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

InterChiplet::PipeComm global_pipe_comm;

int main(int argc, char** argv) {
    // 读取本进程所代表的chiplet编号

    int idX = atoi(argv[1]);
    int idY = atoi(argv[2]);
    uint8_t ddr_row = 69;
    uint8_t ddr_column = 100;
    int input = -1;
    uint32_t frequency = 10;

    int8_t *output = (int8_t*)malloc(ddr_row*ddr_column);

    memset(output, 0, sizeof(int8_t) * ddr_row*ddr_column);

    long long unsigned int time_end = 1;
    long long unsigned int time_end1 = 1;
    for (int i = 0; i < 3; i++)
    {
        int srcX = -1;
        int srcY = -1;
        InterChiplet::waitlaunchSync(&srcX, &srcY, idX, idY);
        time_end = InterChiplet::readSync(time_end, srcX, srcY, idX, idY, 1, InterChiplet::SPD_LAUNCH);

        std::string fileName = InterChiplet::receiveSync(srcX, srcY, idX, idY);
        global_pipe_comm.read_data(fileName.c_str(), &input, sizeof(int));
        time_end1 = InterChiplet::readSync(time_end, srcX, srcY, idX, idY, sizeof(int), 0);
        time_end += (time_end1 - time_end)/frequency;

        // calculate
        int index = input % ddr_row*ddr_column;
        output[index] = input;

        int data_size = index / (8 * frequency * sizeof(int8_t));
        fileName = InterChiplet::sendSync(idX, idY, srcX, srcY);
        global_pipe_comm.write_data(fileName.c_str(), output, sizeof(int8_t) * (data_size+1));
        std::cout << "simulator power of cost " << ":" << 10 << "pJ" << std::endl;

        InterChiplet::writeSync(time_end + frequency*sizeof(int8_t) * (data_size+1)/3000000000000, idX, idY, srcX, srcY, sizeof(int8_t) * (data_size+1), 0);
    }
    
    free(output);
    return 0;
}
