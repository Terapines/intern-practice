## 2.1 检测操作系统  
使用CMAKE_SYSTEM_NAME变量  
## 2.2 根据系统添加宏定义  
核心代码如下：  
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")  
  target_compile_definitions(hello-world PUBLIC   "IS_LINUX")  
endif()  

## 2.3 处理与编译器相关的源代码  
target_compile_definitions(hello-world PUBLIC "COMPILER_NAME=\"\${CMAKE_CXX_COMPILER_ID}\"")  
target_compile_definitions(hello-world
  PUBLIC "IS_${CMAKE_Fortran_COMPILER_ID}_FORTRAN_COMPILER"
)  
还是使用宏定义的方式告诉代码编译器类型  
## 2.4 检测处理器体系结构  
CMAKE_SIZEOF_VOID_P 空指针大小，单位字节，8字节代表64位cpu  
CMAKE_HOST_SYSTEM_PROCESSOR 处理器架构  
## 2.5 检测处理器指令集
这里使用config.h的方法，让cmake填充头文件的宏定义来告诉代码指令集类型  
