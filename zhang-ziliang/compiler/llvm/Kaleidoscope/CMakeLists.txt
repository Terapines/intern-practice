cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(Kaleidscope LANGUAGES CXX)

# # 查找并导入LLVM配置
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")


file(GLOB_RECURSE RESOURCE src/*.cpp src/*/*.cpp !src/JIT/*.cpp)
file(REMOVE_RECURSE RESOURCE src/JIT/*.cpp)
# 创建可执行文件并链接所有的模块
# add_executable(test src/main.cpp)
add_executable(test ${RESOURCE})


# 设置头文件搜索路径
target_include_directories(test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(test PUBLIC ${LLVM_INCLUDE_DIRS})
target_include_directories(test PUBLIC /usr/include/c++)

message("LLVM_INCLUDE_DIRS=${LLVM_INCLUDE_DIRS}")
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS})
target_link_directories(test PUBLIC ${LLVM_LIBRARY_DIRS})

# Find the libraries that correspond to the LLVM components
# that we wish to use

llvm_map_components_to_libnames(llvm_libs support core orcjit native)
message("llvm_libs=${llvm_libs}")

# 链接模块的静态库
# link_libraries(LLVMSupport LLVMCodeGen LLVMIRReader LLVMCore LLVMBinaryFormat)
target_link_libraries(test LLVMSupport LLVMCodeGen LLVMIRReader LLVMCore LLVMBinaryFormat 
LLVMOrcJIT LLVMX86CodeGen LLVMX86AsmParser LLVMX86Desc LLVMX86Disassembler LLVMX86Info)