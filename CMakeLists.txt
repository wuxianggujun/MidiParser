cmake_minimum_required(VERSION 3.20)
project(MidiParser)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

include_directories(src)

add_executable(MidiParser main.cpp
        src/MidiFile.hpp)
