# CMake chapter1功能汇总

# 指定cmake最低版本
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

# 项目名和语言
project(chapter01 LANGUAGES CXX)

#用option代替set创建变量，可以使其暴露给用户，从而可以使用-D选项来设置
# 使用方法： cmake .. -D USE_LIBRARY=ON
option(USE_LIBRARY "Compile sources into a library" OFF)


# 切换构建类型Release或Debug等，默认为空
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# 添加静态库
#library可以指定的类型有STATIC SHARED等
add_library(geometry
  STATIC
    geometry_circle.cpp
    geometry_circle.hpp
    geometry_polygon.cpp
    geometry_polygon.hpp
    geometry_rhombus.cpp
    geometry_rhombus.hpp
    geometry_square.cpp
    geometry_square.hpp
  )

# 添加编译器选项
message("C++ compiler flags: ${CMAKE_CXX_FLAGS}")

list(APPEND flags "-fPIC" "-Wall")
if(NOT WIN32)
  list(APPEND flags "-Wextra" "-Wpedantic")
endif()

target_compile_options(geometry
  PRIVATE
    ${flags}
  )

# 添加编译器选项
target_compile_options(geometry
PRIVATE
  -O3
)

# 设置C++版本
set_target_properties(geometry
  PROPERTIES
    CXX_STANDARD 14 # -std=14
    CXX_EXTENSIONS OFF # 不使用拓展
    CXX_STANDARD_REQUIRED ON # 指定所选标准的版本
    POSITION_INDEPENDENT_CODE 0 # 告诉编译器生成位置无关的代码，用于构建共享库
  )
 
# 定义list变量
list(
  APPEND sources_with_lower_optimization
    geometry_circle.cpp
    geometry_rhombus.cpp
  )

# 遍历每个需要修改的文件 使用set_source_files_properties（）来修改参数
# 将COMPILE_FLAGS修改为 -O2
message(STATUS "Setting source properties using IN LISTS syntax:")
foreach(_source IN LISTS sources_with_lower_optimization)
  set_source_files_properties(${_source} PROPERTIES COMPILE_FLAGS -O2)
  message(STATUS "Appending -O2 flag for ${_source}")
endforeach()


# 查询一下修改后的COMPILE_FLAGS
message(STATUS "Querying sources properties using plain syntax:")
foreach(_source ${sources_with_lower_optimization})
  get_source_file_property(_flags ${_source} COMPILE_FLAGS)
  message(STATUS "Source ${_source} has the following extra COMPILE_FLAGS: ${_flags}")
endforeach()

# 添加可执行文件
add_executable(compute-areas compute-areas.cpp)

# 链接静态库
target_link_libraries(compute-areas geometry)