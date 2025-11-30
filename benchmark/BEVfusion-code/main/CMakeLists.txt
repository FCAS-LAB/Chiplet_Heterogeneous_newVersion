cmake_minimum_required(VERSION 3.0)
project(BEVFusion_Main)

set(CMAKE_CXX_STANDARD 17)
add_executable(main main.cpp)

# 检查SIMULATOR_ROOT环境变量是否存在
if(NOT DEFINED ENV{SIMULATOR_ROOT})
    message(FATAL_ERROR "SIMULATOR_ROOT 环境变量未设置，请设置后重试")
endif()

# 设置interchiplet库
set(INTERCHIPLET_INCLUDE_DIR "$ENV{SIMULATOR_ROOT}/interchiplet/includes")
set(INTERCHIPLET_C_LIB "$ENV{SIMULATOR_ROOT}/interchiplet/lib/libinterchiplet_c.a")

# 设置包含目录
target_include_directories(main PRIVATE ${INTERCHIPLET_INCLUDE_DIR})

# 链接interchiplet库
target_link_libraries(main ${INTERCHIPLET_C_LIB})

# 设置C++标准
set_property(TARGET main PROPERTY CXX_STANDARD 17)

