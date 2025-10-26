# Code Reproduction

## Project Structure

This project implements several algorithms for joint heavy hitter and heavy quadratic element identification, including DualSketch, DUET[1], CSSCHH[2], GlobalHH[3], and 2D-MG[4].  

The directory structure is as follows:

```
Cpp/
├── CMakeLists.txt
├── main.cpp
├── MurmurHash3.cpp
├── CountMin.cpp
├── DUET.cpp
├── DualSketch.cpp
├── CSSCHH.cpp
├── GlobalHH.cpp
├── TwoDMisraGries.cpp
├── Utils.cpp
└── header/
    ├── DUET.h
    ├── DualSketch.h
    ├── CSSCHH.h
    ├── GlobalHH.h
    ├── TwoDMisraGries.h
    ├── utils.h
    ├── MurmurHash3.h
    └── CountMin.h
```

## Implementation and Environment

The project is written in **C++17** and built using **CMake**.  We perform our experiments on a machine equipped with a 13th Gen Intel(R) Core(TM) i7-13700 2.10 GHz processor. You can follow the steps below to build and run the project, and test with your desired parameters.

## Build Instructions

```bash
cd Cpp
mkdir build && cd build
cmake ..
make
```

After compilation, an executable file named `HH_QuadraticEle` will be generated in the `build` directory.  You can run the program with the following command:

```bash
./HH_QuadraticEle
```

## References

> [1] Jiaqian Liu, Haipeng Dai, Rui Xia, Meng Li, Ran Ben Basat, Rui Li, and Guihai Chen. Duet: A generic framework for finding special quadratic elements in data streams. In Proceedings of the ACM Web Conference 2022, pages 2989–2997, 2022.
> 
> [2] Italo Epicoco, Massimo Cafaro, and Marco Pulimeno. Fast and accurate mining of correlated heavy hitters. Data Mining and Knowledge Discovery, 32(1):162–186, 2018.
> 
> [3] Katsiaryna Mirylenka, Themis Palpanas, Graham Cormode, and Divesh Srivastava. Finding interesting correlations with conditional heavy hitters. In 2013 IEEE 29th International Conference on Data Engineering (ICDE), pages 1069–1080. IEEE, 2013.
> 
> [4] Bibudh Lahiri, Arko Provo Mukherjee, and Srikanta Tirthapura. Identifying correlated heavy-hitters in a two-dimensional data stream. Data mining and knowledge discovery, 30(4):797–818, 2016.
