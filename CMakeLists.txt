cmake_minimum_required(VERSION 3.19)
cmake_policy(VERSION 3.19...3.22)

set(MBED_APP_JSON_PATH mbed_app.json5)

include(mbed-os/tools/cmake/mbed_toolchain_setup.cmake)
project(mbed-http-server-example # here you can change your project name
    LANGUAGES C CXX ASM) 
include(mbed_project_setup)

add_subdirectory(mbed-os)

add_subdirectory(mbed-http)

add_executable(${PROJECT_NAME} src/main.cpp)

target_link_libraries(${PROJECT_NAME} mbed-os mbed-netsocket mbed-http)
mbed_set_post_build(${PROJECT_NAME})