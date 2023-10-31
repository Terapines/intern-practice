## 3.1检测python解释器  
**find_package(PythonInterp REQUIRED)**  
find_package模块会调用Find<name>.cmake，即FindPythonInterp.cmake  
该模块包含以下附带的CMake变量：  
- PYTHONINTERP_FOUND：是否找到解释器
- PYTHON_EXECUTABLE：Python解释器到可执行文件的路径
- PYTHON_VERSION_STRING：Python解释器的完整版本信息
- PYTHON_VERSION_MAJOR：Python解释器的主要版本号
- PYTHON_VERSION_MINOR ：Python解释器的次要版本号
- PYTHON_VERSION_PATCH：Python解释器的补丁版本号  
其他Find<name>.cmake同理。  

可以要求特定版本：  
- find_package(PythonInterp 2.7)  
可以强制满足依赖关系：  
- find_package(PythonInterp REQUIRED)
如果在查找位置中没有找到适合Python解释器的可执行文件，CMake将中止配置。  
注意，find_package只会从标准位置去找，可以通过参数传递指定位置：  
- cmake -D PYTHON_EXECUTABLE=/custom/location/python ..
## 3.2 检测python库  
- find_package(PythonInterp REQUIRED)  
用法同上，对应的cmake文件为FindPythonLibs.cmake  
## 3.3 检测python模块和包  
核心代码：  
execute_process(  
  COMMAND  
    \${PYTHON_EXECUTABLE} "-c" "import re, numpy; print(re.compile('/__init__.py.*').sub('',numpy.__file__))"  
  RESULT_VARIABLE _numpy_status  
  OUTPUT_VARIABLE _numpy_location  
  ERROR_QUIET  
  OUTPUT_STRIP_TRAILING_WHITESPACE  
  )  
if(NOT _numpy_status)  
  set(NumPy ${_numpy_location} CACHE STRING "Location of NumPy")  
endif()  

使用executable_process，用python代码实现检测并返回包的位置  

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(NumPy
  FOUND_VAR NumPy_FOUND
  REQUIRED_VARS NumPy
  VERSION_VAR _numpy_version
  )  

使用FindPackageHandleStandardArgs模块查找包的位置  

3.使用add_custom_command命令可以将use_numpy.py复制到build文件夹中  

## 3.4检测BLAS和LAPACK数学库
核心代码：  
- find_package(BLAS REQUIRED)
- find_package(LAPACK REQUIRED)  
查询的结果会保存在BLAS_LIBRARIES变量中  
该变量会在连接库时使用到：  
target_link_libraries(math
  PUBLIC
    ${LAPACK_LIBRARIES}
  )  

## 3.5 检测OpenMP的并行环境  
核心代码：  
- find_package(OpenMP REQUIRED)  
在3.9以上的cmake环境中的用法：  
- target_link_libraries(example
  PUBLIC
    OpenMP::OpenMP_CXX
  )  
在3.5的cmake环境中用法：  
- target_compile_options(example
  PUBLIC
    \${OpenMP_CXX_FLAGS}
  )  

- set_target_properties(example
  PROPERTIES
    LINK_FLAGS ${OpenMP_CXX_FLAGS}
  )

实际的输出：  
- /usr/bin/c++ &emsp; -fopenmp  

## 3.6 检测MPI的并行环境  
MPI是个库：  
- 消息传递接口(Message Passing Interface, MPI)，可以作为OpenMP(共享内存并行方式)的补
充，它也是分布式系统上并行程序的实际标准。尽管，最新的MPI实现也允许共享内存并行，但高性能
计算中的一种典型方法就是，在计算节点上OpenMP与MPI结合使用。  
用法同上：  
- find_package(MPI REQUIRED)  
- target_link_libraries(hello-mpi
  PUBLIC
    MPI::MPI_C
  )  
执行结果：  
- /usr/bin/cc  -isystem /path_to_openmpi  
补充编译器的常用选项：  
- -I（大写i）  
添加头文件路径  
- -L（大写L）  
添加库的路径  
- l（小写L）  
添加库，后面跟库名称，例如pthread库  
## 3.7 检测Eigen库  
同上：  
- find_package(Eigen3 3.3 REQUIRED CONFIG)  
## 3.8 检测Boost库
同上：  
- find_package(Boost 1.54 REQUIRED COMPONENTS filesystem)  
- target_link_libraries(path-info
  PUBLIC
    Boost::filesystem
  )  
## 3.9 检测外部库:Ⅰ. 使用pkg-config
先找到pkg-config工具：  
- find_package(PkgConfig REQUIRED QUIET)  
使用pkg-config工具查找ZeroMQ库：  
- pkg_search_module(
  ZeroMQ
  REQUIRED
    libzeromq libzmq lib0mq
  IMPORTED_TARGET
  )  

CMake提供两个函数来使用pkg-config这个工具：  
- pkg_check_modules
- pkg_search_module

## 3.10 检测外部库:Ⅱ. 自定义find模块
有四种方式可用于找到依赖包:  
1. 使用由包供应商提供CMake文件 <package>Config.cmake
， <package>ConfigVersion.cmake 和 <package>Targets.cmake ，通常会在包的标准安装位
置查找。  
2. 无论是由CMake还是第三方提供的模块，为所需包使用 find-module 。  
3. 使用 pkg-config ，如本节的示例所示。  
4. 如果这些都不可行，那么编写自己的 find 模块。   