# Create library target
add_library(e2e SHARED)

# Add all source files
file(GLOB_RECURSE SOURCES "./*.cpp")
target_sources(e2e PRIVATE ${SOURCES})

# Add include directory
target_include_directories(e2e PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Link dependencies if needed
target_link_libraries(e2e PUBLIC utils sideband d2dadapter logphy protocol)