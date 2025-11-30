cmake_minimum_required(VERSION 3.0)
project(BEVFusion_Head)

# 设置ONNX Runtime路径
set(ONNXRUNTIME_INCLUDE_DIR "/usr/local/include/onnxruntime")
set(ONNXRUNTIME_LIB_DIR "/usr/local/lib")

# 检查SIMULATOR_ROOT环境变量是否存在
if(NOT DEFINED ENV{SIMULATOR_ROOT})
    message(FATAL_ERROR "SIMULATOR_ROOT 环境变量未设置，请设置后重试")
endif()

# 设置interchiplet库
set(INTERCHIPLET_INCLUDE_DIR "$ENV{SIMULATOR_ROOT}/interchiplet/includes")
set(INTERCHIPLET_C_LIB "$ENV{SIMULATOR_ROOT}/interchiplet/lib/libinterchiplet_c.a")

# 查找ONNX Runtime库
find_library(ONNXRUNTIME_LIBRARY onnxruntime PATHS ${ONNXRUNTIME_LIB_DIR} REQUIRED)

# 将可执行文件改为动态库
# add_library(head SHARED head.cpp)
add_executable(head head.cpp)

# 设置包含目录
target_include_directories(head PRIVATE ${ONNXRUNTIME_INCLUDE_DIR} ${INTERCHIPLET_INCLUDE_DIR})

# 链接ONNX Runtime库
target_link_libraries(head ${ONNXRUNTIME_LIBRARY} ${INTERCHIPLET_C_LIB})

# 设置C++标准
set_property(TARGET head PROPERTY CXX_STANDARD 17)

# 设置动态库的版本信息
set_target_properties(head PROPERTIES
    VERSION 1.0.0
    SOVERSION 1)
