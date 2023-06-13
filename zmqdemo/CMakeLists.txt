cmake_minimum_required(VERSION 3.25)
project(zmqtest)

# depend head file and lib
include_directories(/opt/homebrew/include)
link_directories(/opt/homebrew/lib)

set(CMAKE_CXX_STANDARD 17)
set(ZMQ_TEST_INCLUDE
        zmq_receiver.h
        zmq_sender.h)
set(ZMQ_TEST_SRC
        zmqtest.cpp
        zmq_receiver.cpp
        zmq_sender.cpp)

add_executable(zmqtest ${ZMQ_TEST_INCLUDE} ${ZMQ_TEST_SRC})
target_link_libraries(zmqtest zmq)