
option(DEBUG_LEVEL_1 "Enable print debug messages in level 1." OFF)
if(DEBUG_LEVEL_1)
    add_definitions(-DDEBUG_LEVEL_1)
endif()

add_subdirectory(utils)
add_subdirectory(sideband)
add_subdirectory(d2dadapter)
add_subdirectory(logphy)
add_subdirectory(protocol)
add_subdirectory(e2e)

# Create library target
add_library(ccps INTERFACE)

# Link dependencies if needed
target_link_libraries(ccps INTERFACE
    utils
    sideband
    d2dadapter
    logphy
    protocol
    e2e
)