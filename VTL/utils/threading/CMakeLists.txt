add_library(VTL_threading INTERFACE)

target_include_directories(VTL_threading INTERFACE ${CMAKE_SOURCE_DIR})

if(UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(VTL_threading INTERFACE Threads::Threads)
endif()
