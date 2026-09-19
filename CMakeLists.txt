cmake_minimum_required(VERSION 3.20)
project(NCore)

set(CMAKE_CXX_STANDARD 20)

add_library(NCore STATIC src/StreamedNet.cpp)

target_include_directories(NCore PUBLIC 
   "${CMAKE_CURRENT_SOURCE_DIR}/include"
   "${CMAKE_CURRENT_SOURCE_DIR}/external/asio/include"
)

target_compile_definitions(NCore PRIVATE ASIO_STANDALONE)