cmake_minimum_required(VERSION 3.10)
project(camera_vtransform)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_PREFIX_PATH "/home/ting/SourceCode/libtorch")  # 替换为你的LibTorch路径
find_package(Torch REQUIRED)

set(INTERCHIPLET_INCLUDE_DIR "$ENV{SIMULATOR_ROOT}/interchiplet/includes")
set(INTERCHIPLET_C_LIB "$ENV{SIMULATOR_ROOT}/interchiplet/lib/libinterchiplet_c.a")

# add_library(camera_vtransform SHARED camera_vtransform.cpp) 
add_executable(camera_vtransform camera_vtransform.cpp)
target_include_directories(camera_vtransform PRIVATE ${INTERCHIPLET_INCLUDE_DIR})
target_link_libraries(camera_vtransform ${TORCH_LIBRARIES} ${INTERCHIPLET_C_LIB})
target_compile_options(camera_vtransform PRIVATE -g -O0)
message(STATUS "GDB调试已启用，使用以下命令调试：")