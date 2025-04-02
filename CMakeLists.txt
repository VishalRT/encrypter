cmake_minimum_required(VERSION 3.31.5)

project(encrypter)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")

# TODO: Remove this once c++ 23 feature are fully available in clang
# https://www.reddit.com/r/cpp/comments/1b6f3s4/comment/ktcxhha/?utm_source=share&utm_medium=web3x&utm_name=web3xcss&utm_term=1
# if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
#   message("Using gcc libc++ temporarily")
#   add_compile_options(
#     -stdlib=libstdc++ -D__cpp_concepts=202002 -Wno-builtin-macro-redefined
#   )
# endif()
#



#WIndows API inclusion
add_compile_options("$ENV{CFLAGS}" "$ENV{CXXFLAGS}")
link_directories("$ENV{LDFLAGS}")

add_executable(${PROJECT_NAME} src/main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE kernel32)
