cmake_minimum_required(VERSION 3.3)

project(ALSA_WAV_RECORDING)

set ( CMAKE_CXX_FLAGS "-O3")

find_package(ALSA REQUIRED)                                    
if (ALSA_FOUND)                                                
    include_directories(${ALSA_INCLUDE_DIRS})
    link_libraries (${ALSA_LIBRARIES}) 

    add_library(WaveHeaderLib STATIC WaveHeader.cpp)

    add_executable(test test.cpp)
    target_link_libraries(test PRIVATE WaveHeaderLib)                      
endif(ALSA_FOUND) 