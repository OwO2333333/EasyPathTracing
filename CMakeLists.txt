cmake_minimum_required(VERSION 4.3)
project(PathTracing)

set(CMAKE_CXX_STANDARD 20)

if(MSVC)
    add_compile_options(/utf-8)
endif()

add_executable(PathTracing src/main.cpp
        include/stb_image.h
        include/stb_image_write.h
        include/path_tracing.h
        src/path_tracing.cpp)

target_include_directories(
        PathTracing PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/src
)

