cmake_minimum_required(VERSION 3.31.5)

project(encrypter)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")


#WIndows API inclusion
add_compile_options("$ENV{CFLAGS}" "$ENV{CXXFLAGS}")
link_directories("$ENV{LDFLAGS}")

add_executable(${PROJECT_NAME} src/main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE kernel32)
