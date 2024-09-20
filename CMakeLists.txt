cmake_minimum_required(VERSION 3.5)
project(GLX_ROUTER)
add_subdirectory(server)
add_subdirectory(client)

install(
        TARGETS client server 
        DESTINATION ${CMAKE_SOURCE_DIR}/install/bin
    )
