cmake_minimum_required(VERSION 3.26)
project(HH_QuadraticEle)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -funroll-loops -ffast-math -DNDEBUG")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE}")

add_executable(HH_QuadraticEle main.cpp
        MurmurHash3.cpp
        CountMin.cpp
        header/DUET.h
        DUET.cpp
        header/DualSketch.h
        DualSketch.cpp
        header/utils.h
        Utils.cpp
        header/GlobalHH.h
        GlobalHH.cpp
        header/TwoDMisraGries.h
        TwoDMisraGries.cpp
        header/CSSCHH.h
        CSSCHH.cpp
)


