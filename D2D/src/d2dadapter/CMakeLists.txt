# Create library target
add_library(d2dadapter SHARED)

# Add all source files
file(GLOB_RECURSE SOURCES "./*.cpp")
target_sources(d2dadapter PRIVATE ${SOURCES})

# Add include directory
target_include_directories(d2dadapter PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Link dependencies if needed
target_link_libraries(d2dadapter PUBLIC utils)