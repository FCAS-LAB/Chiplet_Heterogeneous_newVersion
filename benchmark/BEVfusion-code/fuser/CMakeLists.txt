cmake_minimum_required(VERSION 3.0)
project(BEVFusion_Fuser)

set(CMAKE_PREFIX_PATH "/home/ting/SourceCode/libtorch")  # 替换为你的LibTorch路径
find_package(Torch REQUIRED)

set(INTERCHIPLET_INCLUDE_DIR "$ENV{SIMULATOR_ROOT}/interchiplet/includes")
set(INTERCHIPLET_C_LIB "$ENV{SIMULATOR_ROOT}/interchiplet/lib/libinterchiplet_c.a")

# 将可执行文件改为动态库
# add_library(fuser SHARED fuser.cpp)
add_executable(fuser fuser.cpp)
target_include_directories(fuser PRIVATE ${INTERCHIPLET_INCLUDE_DIR})
target_link_libraries(fuser "${TORCH_LIBRARIES}" ${INTERCHIPLET_C_LIB})
set_property(TARGET fuser PROPERTY CXX_STANDARD 17)

# 设置动态库的版本信息（可选）
set_target_properties(fuser PROPERTIES
    VERSION 1.0.0
    SOVERSION 1)