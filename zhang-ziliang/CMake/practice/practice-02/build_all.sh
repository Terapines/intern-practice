#!/bin/bash

# 清除旧构建结果
rm -rf build
mkdir build
cd build

# 使用CMake生成构建系统
cmake ..

# 编译项目
make

# cmake --build .