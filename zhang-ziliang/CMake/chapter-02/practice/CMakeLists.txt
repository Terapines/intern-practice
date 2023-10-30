# chapter2内容汇总

# set minimum cmake version
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

# project name and language
project(recipe-02 LANGUAGES CXX)

# define executable and its source file
add_executable(hello-world hello-world.cpp)

# 查看系统名称
message(STATUS "CCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}!!!")
# 添加宏定义
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")  
  target_compile_definitions(hello-world PUBLIC   "IS_LINUX")  
endif()  

# 查看编译器
message(STATUS "CMAKE_CXX_COMPILER_ID=${CMAKE_CXX_COMPILER_ID}!!!!!")
# 添加带值的宏定义
target_compile_definitions(hello-world PUBLIC "COMPILER_NAME=\"${CMAKE_CXX_COMPILER_ID}\"")

# 查看空指针大小 CMAKE_SIZEOF_VOID_P，单位字节，如果是8字节即为64位机器
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  target_compile_definitions(hello-world PUBLIC "IS_64_BIT_ARCH")
  message(STATUS "Target is 64 bits")
else()
  target_compile_definitions(hello-world PUBLIC "IS_32_BIT_ARCH")
  message(STATUS "Target is 32 bits")
endif()

# 查看处理器架构 CMAKE_HOST_SYSTEM_PROCESSOR
if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "i386")
  message(STATUS "i386 architecture detected")
elseif(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "i686")
  message(STATUS "i686 architecture detected")
elseif(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "x86_64")
  message(STATUS "x86_64 architecture detected")
else()
  message(STATUS "host processor architecture is unknown")
endif()

# 使用cmake_host_system_information模块查询机器信息
foreach(key
  IN ITEMS
    NUMBER_OF_LOGICAL_CORES
    NUMBER_OF_PHYSICAL_CORES
    TOTAL_VIRTUAL_MEMORY
    AVAILABLE_VIRTUAL_MEMORY
    TOTAL_PHYSICAL_MEMORY
    AVAILABLE_PHYSICAL_MEMORY
    IS_64BIT
    HAS_FPU
    HAS_MMX
    HAS_MMX_PLUS
    HAS_SSE
    HAS_SSE2
    HAS_SSE_FP
    HAS_SSE_MMX
    HAS_AMD_3DNOW
    HAS_AMD_3DNOW_PLUS
    HAS_IA64
    OS_NAME
    OS_RELEASE
    OS_VERSION
    OS_PLATFORM
  )
  cmake_host_system_information(RESULT _${key} QUERY ${key})
  message(STATUS "${key}=${_${key}}!!!")
endforeach()

# 生成config.h
configure_file(config.h.in config.h @ONLY)

# 检测编译器是否支持以下选项
# 需要手动添加该模块
include(CheckCXXCompilerFlag)

check_cxx_compiler_flag("-march=native" _march_native_works)
check_cxx_compiler_flag("-xHost" _xhost_works)

set(_CXX_FLAGS)
if(_march_native_works)
  message(STATUS "Using processor's vector instructions (-march=native compiler flag set)")
  set(_CXX_FLAGS "-march=native")
elseif(_xhost_works)
  message(STATUS "Using processor's vector instructions (-xHost compiler flag set)")
  set(_CXX_FLAGS "-xHost")
else()
  message(STATUS "No suitable compiler flag found for vectorization")
endif()

# 添加编译器选项
target_compile_options(hello-world
  PRIVATE
    ${_CXX_FLAGS}
  )