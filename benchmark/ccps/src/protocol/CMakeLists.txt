# Create library target
add_library(protocol SHARED)

# Add all source files
file(GLOB_RECURSE SOURCES "./*.cpp")
target_sources(protocol PRIVATE ${SOURCES})

# Add include directory
target_include_directories(protocol PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Link dependencies if needed
target_link_libraries(protocol PUBLIC utils)