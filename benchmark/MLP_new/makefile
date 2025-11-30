BENCHMARK_ROOT=$(SIMULATOR_ROOT)/artifact/MLP
INTERCHIPLET=../../interchiplet/bin/interchiplet
GIT_REPO=https://github.com/nlohmann/json.git
REPO_DIR=$(BENCHMARK_ROOT)/json

GPGPUSIM_CONFIG_CUDAART=/gcc-7.5.0/cuda-11080/release

# Compiler environment of C/C++
CC=g++
CFLAGS=-Wall -Werror -g -I$(SIMULATOR_ROOT)/interchiplet/includes
INTERCHIPLET_C_LIB=$(SIMULATOR_ROOT)/interchiplet/lib/libinterchiplet_c.a

# C/C++ Source file
C_SRCS=mlp.cpp
C_OBJS=obj/mlp.o
C_TARGET=bin/mlp_c

# Compiler environment of CUDA
NVCC=nvcc
CUFLAGS=--compiler-options -Wall -I$(SIMULATOR_ROOT)/interchiplet/includes
INTERCHIPLET_CU_LIB=$(SIMULATOR_ROOT)/interchiplet/lib/libinterchiplet_cu.a

# CUDA Source file
CUDA_SRCS = mlp.cu
CUDA_OBJS = $(patsubst %.cu, cuobj/%.o, $(CUDA_SRCS))
CUDA_TARGET = bin/mlp_cu

NPU_SRCS=dsa.cpp
NPU_OBJS=obj/dsa.o
NPU_TARGET=bin/dsa

MNSIM_SRCS=mnsim.cpp
MNSIM_OBJS=obj/mnsim.o
MNSIM_TARGET=bin/mnsim

all:bin_dir obj_dir cuobj_dir sniper_target gpgpusim_target  NPU_target MNSIM_target

sniper_target: $(C_OBJS)
	$(CC) -g $(C_OBJS) $(INTERCHIPLET_C_LIB) -o $(C_TARGET) -lpthread

obj/%.o: %.cpp
	if [ ! -d $(REPO_DIR) ]; then \
        git clone $(GIT_REPO) $(REPO_DIR); \
    fi
	
	$(CC) $(CFLAGS) -c $< -o $@

cuobj/%.o: %.cu 
	$(NVCC) $(CUFLAGS) -c $< -o $@

debug: CFLAGS += -DDEBUG -g
debug: all

gpgpusim_target: $(CUDA_OBJS)
	$(NVCC) -L$(SIMULATOR_ROOT)/gpgpu-sim/lib/$(GPGPUSIM_CONFIG_CUDAART) -g --cudart shared $(CUDA_OBJS) -o $(CUDA_TARGET)

# NPU language target
NPU_target: $(NPU_OBJS)
	$(CC) $(NPU_OBJS) $(INTERCHIPLET_C_LIB) -o $(NPU_TARGET)

# MNSIM language target
MNSIM_target: $(MNSIM_OBJS)
	$(CC) $(MNSIM_OBJS) $(INTERCHIPLET_C_LIB) -o $(MNSIM_TARGET)

# Directory for binary files.
bin_dir:
	mkdir -p bin

# Directory for object files for C.
obj_dir:
	mkdir -p obj

cuobj_dir:
	mkdir -p cuobj

run:
	$(INTERCHIPLET) mlp.yml -w 6 -f 2

run_cpu:
	$(SNIPER_EXEC) --curdir $(BENCHMARK_ROOT) -- $(BENCHMARK_ROOT)/$(SNIPER_TARGET) 0 0

run_gpu:
	./$(GPGPUSIM_TARGET) 0 1 > gpgpusim.0.1.log 2>&1 


gdb: $(SNIPER_TARGET)
	cd $(BENCHMARK_ROOT) && gdb ./$(SNIPER_TARGET) -- $(BENCHMARK_ROOT)/$(SNIPER_TARGET) 0 0

valgrind: $(SNIPER_TARGET)
	cd $(BENCHMARK_ROOT) && valgrind --leak-check=full ./$(SNIPER_TARGET) -- $(BENCHMARK_ROOT)/$(SNIPER_TARGET) 0 0

clean:
	rm -rf bench.txt delayInfo.txt buffer* message_record.txt
	rm -rf proc_r*_t* 

clean_all:
	make clean 
	rm -rf obj cuobj bin

kill:
	pkill -f mlp_cu
	pkill -f mlp_cpu
