#include <fstream>
#include <iostream>
#include <stdlib.h>
#include "apis_c.h"
#include "../../interchiplet/includes/pipe_comm.h"

int idX, idY;

InterChiplet::PipeComm global_pipe_comm;

int main(int argc, char** argv)
{
    idX = atoi(argv[1]);
    idY = atoi(argv[2]);
    long long unsigned int timeNow = 1;
    int size_input = 1;
    int size = 12*12*8;
    int64_t* size_A = new int64_t;
    std::string fileName = InterChiplet::receiveSync(5, 5, idX, idY);
    global_pipe_comm.read_data(fileName.c_str(), size_A, size_input * sizeof(int64_t));
    long long int time_end = InterChiplet::readSync(timeNow, 5, 5, idX, idY, size_input * sizeof(int64_t), 0);

    long long unsigned int true_time = 5000;
    timeNow = true_time + time_end; 

    int64_t* ans = new int64_t[size];
    for(int i=0;i<size;i++){
        ans[i] = i;
    }

    fileName = InterChiplet::sendSync(idX, idY, 5, 5);
    global_pipe_comm.write_data(fileName.c_str(), ans, size * sizeof(int64_t));
    time_end = InterChiplet::writeSync(timeNow, idX, idY, 5, 5, size * sizeof(int64_t), 0);

    int size_output = 7*7*2048;
    int64_t* output = new int64_t[size_output];
    fileName = InterChiplet::receiveSync(5, 5, idX, idY);
    global_pipe_comm.read_data(fileName.c_str(), output, size_output * sizeof(int64_t));
    time_end = InterChiplet::readSync(timeNow, 5, 5, idX, idY, size_output * sizeof(int64_t), 0);
    timeNow = time_end + true_time;
    int64_t* ans_back = new int64_t;

    fileName = InterChiplet::sendSync(idX, idY, 5, 5);
    global_pipe_comm.write_data(fileName.c_str(), ans_back, sizeof(int64_t));
    time_end = InterChiplet::writeSync(timeNow, idX, idY, 5, 5, sizeof(int64_t), 0);

    // delete[] size_A;
    delete[] ans;
    
}