# CPP - OrderBook Implementation

## Requirements
1. vcpkg installed
2. GoogleTest installed via vcpkg:
```bash
./vcpkg install gtest
```

## Steps to run
1) Go to the cpp-orderbook folder:
```bash
cd ~/Documents/cpp-orderbook
```
2) Create the build folder and configure with CMake:
```bash
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=~/Documents/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build .
```
3) Run tests:
```bash
cd build
./tests/all_tests   
```
4) For performance
```base
sudo sysctl -w kernel.perf_event_paranoid=-1
```




