# Project environment
# SIMULATOR_ROOT, defined by setup_env.sh
BENCHMARK_ROOT=$(SIMULATOR_ROOT)/benchmark/pcmodel

# Compiler environment of C/C++
CC=g++
CFLAGS=-Wall -Werror -g -I$(SIMULATOR_ROOT)/interchiplet/includes
INTERCHIPLET_C_LIB=$(SIMULATOR_ROOT)/interchiplet/lib/libinterchiplet_c.a

# C/C++ Source file
C_SRCS=pcmodel.cpp
C_OBJS=obj/pcmodel.o
C_TARGET=bin/pcmodel_c

# NPU Source file
CUDA_SRCS=npupcmodel.cpp
CUDA_OBJS=cuobj/npupcmodel.o
CUDA_TARGET=bin/npupcmodel

NPU_SRCS=cim.cpp
NPU_OBJS=cuobj/cim.o
NPU_TARGET=bin/cim

DDR_SRCS=ddr.cpp
DDR_OBJS=cuobj/ddr.o
DDR_TARGET=bin/ddr

3DDRAM_SRCS=3DDRAM.cpp
3DDRAM_OBJS=cuobj/3DDRAM.o
3DDRAM_TARGET=bin/3DDRAM

all: bin_dir obj_dir cuobj_dir C_target CUDA_target NPU_target DDR_target 3DDRAM_target

# C language target
C_target: $(C_OBJS)
	$(CC) $(C_OBJS) $(INTERCHIPLET_C_LIB) -o $(C_TARGET)

# CUDA language target
CUDA_target: $(CUDA_OBJS)
	$(CC) $(CUDA_OBJS) $(INTERCHIPLET_C_LIB) -o $(CUDA_TARGET)

# NPU language target
NPU_target: $(NPU_OBJS)
	$(CC) $(NPU_OBJS) $(INTERCHIPLET_C_LIB) -o $(NPU_TARGET)

# DDR language target
DDR_target: $(DDR_OBJS)
	$(CC) $(DDR_OBJS) $(INTERCHIPLET_C_LIB) -o $(DDR_TARGET)

# 3DDRAM language target
3DDRAM_target: $(3DDRAM_OBJS)
	$(CC) $(3DDRAM_OBJS) $(INTERCHIPLET_C_LIB) -o $(3DDRAM_TARGET)

# Rule for C object
obj/%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

# Rule for Cuda object
cuobj/%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

# Directory for binary files.
bin_dir:
	mkdir -p bin

# Directory for object files for C.
obj_dir:
	mkdir -p obj

# Directory for object files for CUDA.
cuobj_dir:
	mkdir -p cuobj

# Clean generated files.
clean:
	rm -rf bench.txt delayInfo.txt buffer* message_record.txt
	rm -rf proc_r*_t* *.log
	rm -rf obj cuobj bin
