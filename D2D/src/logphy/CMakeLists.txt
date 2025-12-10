# Create library target
add_library(logphy SHARED)

# Add all source files
file(GLOB_RECURSE SOURCES "./*.cpp")
target_sources(logphy PRIVATE ${SOURCES})

# Add include directory
target_include_directories(logphy PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Link dependencies if needed
target_link_libraries(logphy PUBLIC utils)