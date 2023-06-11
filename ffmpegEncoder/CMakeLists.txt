cmake_minimum_required(VERSION 3.25)
project(ffmpegEncoder)

include_directories(/opt/homebrew/include)
link_directories(/opt/homebrew/lib)

set(CMAKE_CXX_STANDARD 17)
set(FFMPEG_LIBS
        avdevice
        avfilter
        avformat
        avcodec
        swresample
        swscale
        avutil)

add_executable(ffmpegEncoder main.cpp h265_cup_encoder.h h265_cup_encoder.cpp)
target_link_libraries(ffmpegEncoder ${FFMPEG_LIBS})