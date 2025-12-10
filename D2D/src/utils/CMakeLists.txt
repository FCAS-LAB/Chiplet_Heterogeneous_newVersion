# Create library target
add_library(utils SHARED)

# Add all source files
file(GLOB_RECURSE SOURCES "./*.cpp")
target_sources(utils PRIVATE ${SOURCES})

# Add include directory
target_include_directories(utils PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    ${Boost_INCLUDE_DIRS}
)

target_link_libraries(utils
    PRIVATE
        Boost::boost
)

# Link dependencies if needed
# target_link_libraries(core PUBLIC some_dependency)