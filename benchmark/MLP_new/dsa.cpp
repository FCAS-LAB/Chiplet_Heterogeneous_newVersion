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

void T(double* ans, double* A, int Row, int Col) {
    for (int j = 0; j < Col; j++) {
        for (int i = 0; i < Row; i++) {
            ans[i + j * Row] = A[i * Col + j];
        }
    }
}

int main(int argc, char** argv){
    int idX = atoi(argv[1]);
    int idY = atoi(argv[2]);
    while(1){
        int64_t *size_A = new int64_t[2];
        long long unsigned int timeNow = 1;
        std::string fileName = InterChiplet::receiveSync(5, 5, idX, idY);
        global_pipe_comm.read_data(fileName.c_str(), size_A, 2 * sizeof(int64_t));
        long long int time_end = InterChiplet::readSync(timeNow, 5, 5, idX, idY, 2 * sizeof(int64_t), 0);
        std::cout<<"size_A[0]: "<<size_A[0]<<" size_A[1]: "<<size_A[1]<<std::endl;

        int64_t Row_A = size_A[0];
        int64_t Col_A = size_A[1];

        if(Row_A == -1 && Col_A == -1){
            delete[] size_A;
            break;
        }
        double *A = new double[Row_A * Col_A];
        fileName = InterChiplet::receiveSync(5, 5, idX, idY);
        global_pipe_comm.read_data(fileName.c_str(), A, Row_A * Col_A * sizeof(double));
        time_end = InterChiplet::readSync(time_end, 5, 5, idX, idY, Row_A * Col_A * sizeof(double), 0);

        double *ans = new double[Row_A * Col_A];

        T(ans, A, Row_A, Col_A);

        fileName = InterChiplet::sendSync(idX, idY, 5, 5);
        global_pipe_comm.write_data(fileName.c_str(), ans, Row_A * Col_A * sizeof(double));
        time_end = InterChiplet::writeSync(time_end, idX, idY, 5, 5, Row_A * Col_A * sizeof(double), 0);

        delete[] size_A;
        delete[] A;
        delete[] ans;
    }
    
    return 0;
}
