cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(Kaleidscope LANGUAGES CXX)

# # 查找并导入LLVM配置
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# 设置头文件搜索路径
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS})

add_subdirectory(src/JIT)
add_subdirectory(src/AST)
add_subdirectory(src/codegen)
add_subdirectory(src/lexer)
add_subdirectory(src/parser)

# 创建可执行文件并链接所有的模块
add_executable(main src/main.cpp)

# Find the libraries that correspond to the LLVM components
# that we wish to use
llvm_map_components_to_libnames(llvm_libs support core orcjit native)
message("llvm_libs=${llvm_libs}")

# 链接模块的静态库
target_link_libraries(
  main 
PRIVATE 
JIT 
lexer 
AST 
parser 
codegen 
${llvm_libs} 
)