# 每小节概要
## 1.1 编译单个文件   
## 1.2 切换生成器   
CMake默认会生成makefile文件，但不仅仅支持生成makefile  
使用cmake --help 可以在generators这一段查看cmake支持的所有生成器  
常见的生成器有make和ninja  
make和ninja的区别？  
ninja相对于make的优势在于Ninja 舍弃了各种高级功能，语法和用法非常简单，所以启动编译的速度非常快。当然，缺点就是相对于make功能不够强大  

如何切换生成器？  
cmake -G Ninja .. 
## 1.3 构建和链接静态库 动态库  
## 1.4 条件语句  
## 1.5 向用户暴露选项  
## 1.6 指定编译器  
## 1.7 切换构建类型  
## 1.8 设置编译器选项  
## 1.9 设置C++版本  
## 1.10 foreach  