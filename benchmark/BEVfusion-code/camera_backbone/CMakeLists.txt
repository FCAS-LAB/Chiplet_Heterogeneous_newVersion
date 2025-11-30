cmake_minimum_required(VERSION 3.0)
project(BEVFusion_Backbone)

set(CMAKE_PREFIX_PATH "/home/ting/SourceCode/libtorch")  # 替换为你的LibTorch路径
find_package(Torch REQUIRED)

set(INTERCHIPLET_INCLUDE_DIR "$ENV{SIMULATOR_ROOT}/interchiplet/includes")
set(INTERCHIPLET_C_LIB "$ENV{SIMULATOR_ROOT}/interchiplet/lib/libinterchiplet_c.a")

# add_library(camera_backbone SHARED camera_backbone.cpp)
add_executable(camera_backbone camera_backbone.cpp)
target_include_directories(camera_backbone PRIVATE ${INTERCHIPLET_INCLUDE_DIR})
target_link_libraries(camera_backbone ${TORCH_LIBRARIES} ${INTERCHIPLET_C_LIB})
set_property(TARGET camera_backbone PROPERTY CXX_STANDARD 17)

set_target_properties(camera_backbone PROPERTIES
    VERSION 1.0.0
    SOVERSION 1)