#include <fstream>
#include <iostream>
#include "apis_c.h"
#include<random>
#include <ctime>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>


using namespace std;
int idX,idY;
std::mutex mtx,mtxM1,mtxM2,mtxM3,mtx_send,mtx_read;
queue<int64_t*> first_second,second_third,third_forth,answer;
std::condition_variable cv;
bool start_first=true;

queue<bool> read_first_done,read_sec_done,read_third_done,read_forth_done;
bool send_first_done = false;
bool send_sec_done = false;
bool send_third_done = false;
bool send_forth_done = false;


int batch=1;
void send_first(int idX, int idY, int64_t* M, size_t size) {
	for (int i = 0; i < batch; i++) {
		{
			std::unique_lock<std::mutex> lock(mtx);
			cv.wait(lock, []
				{ return !send_first_done && (send_sec_done || start_first); });
		}
        
		{
            std::unique_lock<std::mutex> lock(mtx_send);
            cout<<"向(0,1)发送数据"<<endl;
			InterChiplet::sendMessage(0, 1, idX, idY, M, size);
		}
		{
			std::unique_lock<std::mutex> lock(mtx);
			send_first_done = true;
		}
		cv.notify_all();
	}
}

void read_first(int idX, int idY, int64_t* M_out_first, size_t size_out_first) {
    for (int i = 0; i < batch; i++) {
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, []
                { return send_first_done; });
        }
        
        
        {
            unique_lock<mutex> lock(mtx_read);
            cout<<"从(0,1)读取数据"<<endl;
            InterChiplet::receiveMessage(idX, idY, 0, 1, M_out_first, size_out_first);
        }
        
        {
            std::unique_lock<std::mutex> lock(mtxM1);
            first_second.push(M_out_first);
        }
        {
            std::lock_guard<std::mutex> lock(mtx);
            read_first_done.push(true);
            send_first_done = false;
            start_first = false;
        }
        cv.notify_all();
    }
}

void send_sec(int idX, int idY, int64_t* M_out_first, size_t size_out_first) {
    for (int i = 0; i < batch; i++) {
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, []
                { return read_first_done.size() && !send_sec_done; });
            read_first_done.pop();
        }
        {
            M_out_first=first_second.front();
            std::unique_lock<std::mutex> lock(mtxM1);
            first_second.pop();
        }
        {
            cout<<"向(0,2)发送数据"<<endl;
            std::unique_lock<std::mutex> lock(mtx_send);
            InterChiplet::sendMessage(0, 2, idX, idY, M_out_first, size_out_first);
        }
        {
            std::unique_lock<std::mutex> lock(mtx);
            send_sec_done = true;
        }
        cv.notify_all();
    }
    
}

void read_sec(int idX, int idY, int64_t* M_out_second, size_t size_out_second) {
    int64_t *allow_receive=new int64_t;
    for (int i = 0; i < batch; i++) {
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, []
                { return send_sec_done; });
        }
        {
            std::unique_lock<std::mutex> lock(mtx_read);
            cout<<"从(0,2)读取数据"<<endl;
            InterChiplet::receiveMessage(idX, idY, 0, 2, M_out_second, size_out_second);
        }
        
        {
            std::unique_lock<std::mutex> lock(mtxM2);
            second_third.push(M_out_second);
        }
        {
            std::lock_guard<std::mutex> lock(mtx);
            read_sec_done.push(true);
            send_sec_done = false;
        }
        cv.notify_all();
    }
    delete allow_receive;
    
}

void send_third(int idX, int idY, int64_t* M_out_second, size_t size_out_second) {
    for (int i = 0; i < batch; i++) {
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, []
                { return read_sec_done.size() && !send_third_done; });
            read_sec_done.pop();
        }
        {
            std::unique_lock<std::mutex> lock(mtxM2);
            M_out_second=second_third.front();
            second_third.pop();
        }
        
        {
            std::unique_lock<std::mutex> lock(mtx_send);
            cout<<"向(0,3)发送数据"<<endl;
            InterChiplet::sendMessage(0, 3, idX, idY, M_out_second, size_out_second);
        }
        
        {
            std::unique_lock<std::mutex> lock(mtx);
            send_third_done = true;
            
        }
        cv.notify_all();
    }
}

void read_third(int idX, int idY, int64_t* M_out_third, size_t size_out_third) {
    for (int i = 0; i < batch; i++) {
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, []
                { return send_third_done; });
        }
        
        {
            std::unique_lock<std::mutex> lock(mtx_read);
            cout<<"从(0,3)读取数据"<<endl;
            InterChiplet::receiveMessage(idX, idY, 0, 3, M_out_third, size_out_third);
        }
        
        {
            std::unique_lock<std::mutex> lock(mtxM3);
            third_forth.push(M_out_third);
        }
        
        {
            std::unique_lock<std::mutex> lock(mtx);
            send_third_done = false;
            read_third_done.push(true);
        }
        cv.notify_all();
    } 
}

void send_forth(int idX, int idY, int64_t* M_out_third, size_t size_out_third) {
    for (int i = 0; i < batch; i++) {
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, []
                { return read_third_done.size() && !send_forth_done; });
            read_third_done.pop();
        }
        {
            std::unique_lock<std::mutex> lock(mtxM3);
            M_out_third=third_forth.front();
            third_forth.pop();
            cout<<"###############"<<endl;
        }
        
        {
            std::unique_lock<std::mutex> lock(mtx_send);
            cout<<"向(0,4)发送数据"<<endl;
            InterChiplet::sendMessage(0, 4, idX, idY, M_out_third, size_out_third);
        }
        {
            std::unique_lock<std::mutex> lock(mtx);
            send_forth_done = true;
        }
        cv.notify_all();
    }
}

void read_forth(int idX, int idY, int64_t* M_out_forth, size_t size_out_forth) {
    for (int i = 0; i < batch; i++) {
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, []
                { return send_forth_done; });
        }
        {
            unique_lock<mutex> lock(mtx_read);
            cout << "从(0,4)读取数据" << endl;
            InterChiplet::receiveMessage(idX, idY, 0, 4, M_out_forth, size_out_forth);
            // answer.push(M_out_forth);
        }
        {
            std::unique_lock<std::mutex> lock(mtx);
            send_forth_done = false;
            // read_forth_done.push(true);
        }
        cv.notify_all();
    } 
}

void random_init(int64_t * data, int size){
    int i;
    for (i = 0; i < size; i++){
        //data[i] = rand();
        data[i] = 128;
    }
}

int main(int argc, char** argv)
{
    idX = atoi(argv[1]);
    idY = atoi(argv[2]);
    batch = atoi(argv[3]);

    int size = 12*12*8;
    int size_input = 1;
    int64_t *test = new int64_t[size_input];
    int64_t *test_ans = new int64_t[size];
    
    InterChiplet::sendMessage(0, 5, idX, idY, test, size_input*sizeof(int64_t));
    random_init(test, size_input);
    InterChiplet::receiveMessage(idX, idY, 0, 5, test_ans, size*sizeof(int64_t));
    delete[] test;
    delete[] test_ans;
    std::cout<<"-------------------------------------ddr over-------------------------------------"<<std::endl;

    int originalWidth = 112, originalHeight = 112, channels = 64;
    int width=56,height=56;
    int out_width_first = 56, out_height_first = 56, out_channels_first = 256;
    int out_width_sec = 28, out_height_sec = 28, out_channels_sec = 512;
    int out_width_third = 14, out_height_third = 14, out_channels_third = 1024;
    int out_width_forth = 7, out_height_forth = 7, out_channels_forth = 2048;

    int original_size = originalWidth * originalHeight * channels;
    size=width*height*channels;
    int size_out_first=out_width_first*out_height_first*out_channels_first;
    int size_out_second=out_width_sec*out_height_sec*out_channels_sec;
    int size_out_third=out_width_third*out_height_third*out_channels_third;
    int size_out_forth=out_width_forth*out_height_forth*out_channels_forth;

    int64_t* original_M=new int64_t[original_size];
    int64_t* M=new int64_t[size];
    int64_t* M_out_first=new int64_t[size_out_first];
    int64_t* M_send_second=new int64_t[size_out_first];
    int64_t* M_out_second=new int64_t[size_out_second];
    int64_t* M_send_third=new int64_t[size_out_second];
    int64_t* M_out_third=new int64_t[size_out_third];
    int64_t* M_send_forth=new int64_t[size_out_third];
    int64_t* M_out_forth=new int64_t[size_out_forth];
    for (int i = 0; i < original_size; i++) {
        original_M[i] = (rand() % 255)*1e4;
    }
    
    // 发送原始图像数据
    InterChiplet::sendMessage(1, 0, idX, idY, original_M, original_size * sizeof(int64_t));
    std::cout<<"---------------------------------------发送原始图像数据完成---------------------------------------"<<std::endl;
    // 读取原始图像数据
    InterChiplet::receiveMessage(idX, idY, 1, 0, M, size* sizeof(int64_t));
    std::cout<<"---------------------------------------读取修改图像数据完成---------------------------------------"<<std::endl;
    // 创建并启动线程
    std::thread send_first_thread(send_first, idX, idY, M, size* sizeof(int64_t));
    std::thread read_first_thread(read_first, idX, idY, M_out_first, size_out_first* sizeof(int64_t));
    std::thread send_sec_thread(send_sec, idX, idY, M_send_second, size_out_first* sizeof(int64_t));
    std::thread read_sec_thread(read_sec, idX, idY, M_out_second, size_out_second* sizeof(int64_t));
    std::thread send_third_thread(send_third, idX, idY, M_send_third, size_out_second* sizeof(int64_t));
    std::thread read_third_thread(read_third, idX, idY, M_out_third, size_out_third* sizeof(int64_t));
    std::thread send_forth_thread(send_forth, idX, idY, M_send_forth, size_out_third* sizeof(int64_t));
    std::thread read_forth_thread(read_forth, idX, idY, M_out_forth, size_out_forth* sizeof(int64_t));

    // 等待所有线程执行完毕
    send_first_thread.join();
    read_first_thread.join();
    send_sec_thread.join();
    read_sec_thread.join();
    send_third_thread.join();
    read_third_thread.join();
    send_forth_thread.join();
    read_forth_thread.join();
    std::cout<<"write back"<<endl;
    int64_t* ans_back = new int64_t;
    InterChiplet::sendMessage(0, 5, idX, idY, M_out_forth, size_out_forth* sizeof(int64_t));
    InterChiplet::receiveMessage(idX, idY, 0, 5, ans_back, sizeof(int64_t));
}