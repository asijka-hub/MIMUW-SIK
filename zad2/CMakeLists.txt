cmake_minimum_required(VERSION 3.16)
project(sikradio)
set(CMAKE_CXX_STANDARD 20)

set(THREADS_PREFER_PTHREAD_FLAG ON)

add_compile_options(-std=gnu++20 -Wall -Wextra -pedantic -O2)

find_package(Boost 1.71 COMPONENTS program_options system REQUIRED)
find_package(Threads REQUIRED)

add_executable(sikradio-sender src/sender.cpp)
add_executable(sikradio-receiver src/receiver.cpp)

target_link_libraries(sikradio-sender Boost::program_options Boost::system Threads::Threads)
target_link_libraries(sikradio-receiver Boost::program_options Boost::system Threads::Threads)