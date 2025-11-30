cmake_minimum_required(VERSION 3.0)
project(BEVfusion)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置LibTorch路径
set(CMAKE_PREFIX_PATH "/home/ting/SourceCode/libtorch")
find_package(Torch REQUIRED)

# 设置ONNX Runtime路径
set(ONNXRUNTIME_INCLUDE_DIR "/usr/local/include/onnxruntime")
set(ONNXRUNTIME_LIB_DIR "/usr/local/lib")
find_library(ONNXRUNTIME_LIBRARY onnxruntime PATHS ${ONNXRUNTIME_LIB_DIR} REQUIRED)

# 设置interchiplet库
set(INTERCHIPLET_INCLUDE_DIR "$ENV{SIMULATOR_ROOT}/interchiplet/includes")
set(INTERCHIPLET_C_LIB "$ENV{SIMULATOR_ROOT}/interchiplet/lib/libinterchiplet_c.a")

# 添加子目录
add_subdirectory(camera_backbone)
add_subdirectory(camera_vtransform)
add_subdirectory(lidar_backbone)
add_subdirectory(fuser)
add_subdirectory(head)

# 添加可执行文件
add_executable(bevfusion BEVfusion_main.cpp)

# 包含头文件目录
target_include_directories(bevfusion PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/camera_backbone
    ${CMAKE_CURRENT_SOURCE_DIR}/camera_vtransform
    ${CMAKE_CURRENT_SOURCE_DIR}/lidar_backbone
    ${CMAKE_CURRENT_SOURCE_DIR}/fuser
    ${CMAKE_CURRENT_SOURCE_DIR}/head
    ${ONNXRUNTIME_INCLUDE_DIR}
    ${INTERCHIPLET_INCLUDE_DIR}
)

# 链接库
target_link_libraries(bevfusion
    camera_backbone
    camera_vtransform
    lidar_backbone
    fuser
    head
    ${TORCH_LIBRARIES}
    ${ONNXRUNTIME_LIBRARY}
    ${INTERCHIPLET_C_LIB}
)

# # 允许使用相对路径的 RPATH
# set(CMAKE_SKIP_BUILD_RPATH FALSE)
# set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
# set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
# set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# # 设置可执行文件的 RPATH 包含构建目录中的库路径
# set_target_properties(bevfusion PROPERTIES
#     INSTALL_RPATH_USE_LINK_PATH TRUE
#     BUILD_RPATH "${CMAKE_BINARY_DIR}/camera_backbone;${CMAKE_BINARY_DIR}/camera.vtransform;${CMAKE_BINARY_DIR}/lidar_backbone;${CMAKE_BINARY_DIR}/fuser;${CMAKE_BINARY_DIR}/head"
# )

# 添加调试符号
set(CMAKE_BUILD_TYPE Debug)

# 扩大栈空间
# if(UNIX)
#     # Linux系统下扩大栈空间
#     set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-z,stack-size=1073741824")
# elseif(WIN32)
#     # Windows系统下扩大栈空间
#     set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:1073741824")
# endif()