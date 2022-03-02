cmake_minimum_required(VERSION 3.21)
project(iteration2)

set(CMAKE_CXX_STANDARD 17)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(iteration2 main.cpp io_context.hpp)
target_link_libraries(iteration2 PRIVATE Threads::Threads)