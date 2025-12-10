cmake_minimum_required(VERSION 3.14)
project(CCPS VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(DEBUG_LEVEL_1 "Whether print level 1 module ios." OFF)
if (DEBUG_LEVEL_1)
    add_definitions(-DDEBUG_LEVEL_1)
endif()

# Debug flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
# Release flags
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

set(BOOST_ROOT "${CMAKE_SOURCE_DIR}/3rd_party/boost")
set(Boost_NO_SYSTEM_PATHS ON)

find_package(Boost REQUIRED)
if(Boost_FOUND)
    message(STATUS "Found Boost version: ${Boost_VERSION}")
    message(STATUS "Boost includes: ${Boost_INCLUDE_DIRS}")
endif()

#find_package(GTest REQUIRED)

# Include directories for the whole project
include_directories(include)
include_directories(test/include)

# Add subdirectories
add_subdirectory(src)       # Builds the library
add_subdirectory(test)      # Builds the tests