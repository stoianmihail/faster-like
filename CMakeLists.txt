cmake_minimum_required(VERSION 3.22)
project(faster-like)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -O3")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -Wall -Wextra")

find_package(Threads REQUIRED)
set(THREADS_PREFER_PTHREAD_FLAG ON)

include_directories(${CMAKE_SOURCE_DIR})

file(GLOB INCLUDE_H "include/*.h" "src/*.cpp")
set(MAIN_FILES main.cc)

set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 
find_package(Boost 1.71.0) 

if (Boost_Found)
  add_executable(main ${INCLUDE_H} ${MAIN_FILES})
  target_link_libraries(main pthread Boost::boost)
else()
  add_executable(main ${INCLUDE_H} ${MAIN_FILES})
  target_link_libraries(main pthread)
endif()