## 常用网址导航：  
怎样构建一个使用LLVM库的项目：https://llvm.org/docs/Projects.html  
要使用LLVM头文件、库、工具等，需要进行一些设置，包括makefile变量和目录布局等  

llvm命令行用法：https://llvm.org/docs/CommandGuide/index.html  
该页面列举了所有llvm工具链的用法，包括核心工具、GNU binutils替代品、调试工具、开发者工具和备注工具  

使用源码编译安装llvm、clang、lld：https://llvm.org/docs/GettingStarted.html  

clang用户文档和使用选项:  https://clang.llvm.org/docs/UsersManual.html  

llvm IR 参考文档：http://llvm.org/docs/LangRef.html   

## llvm常用工具  
`bugpoint`

bugpoint用于通过将给定测试用例缩小到仍然导致问题（无论是崩溃还是错误编译）的最小传递和/或指令数量来调试优化传递或代码生成后端。

`llvm-ar`

归档程序生成一个包含给定 LLVM 位码文件的归档文件，还可以选择带有索引以加快查找速度。

`llvm-as`

汇编器将人类可读的 LLVM 程序集转换为 LLVM 位码。

`llvm-dis`

反汇编器将 LLVM 位码转换为人类可读的 LLVM 程序集。

`llvm-link`

llvm-link毫不奇怪，它将多个 LLVM 模块链接到一个程序中。

`lli`

lli是LLVM解释器，它可以直接执行LLVM位码（虽然很慢……）。对于支持它的体系结构（当前为 x86、Sparc 和 PowerPC），默认情况下，lli它将充当即时编译器（如果该功能已编译），并且执行代码的速度 比解释器快得多。

`llc`

llc是 LLVM 后端编译器，它将 LLVM 位代码转换为本机代码汇编文件。

`opt`

opt读取 LLVM 位码，应用一系列 LLVM 到 LLVM 转换（在命令行上指定），并输出结果位码。' ' 是获取 LLVM 中可用程序转换列表的好方法。opt -help

opt还可以对输入 LLVM 位码文件运行特定分析并打印结果。主要用于调试分析或熟悉分析的作用。

## llvm工具链和clang命令行使用示例  
将c文件编译为可执行文件  
```bash
clang hello.c -o hello
```

将 C 文件编译为 LLVM bitcode文件：  
```bash
clang -O3 -emit-llvm hello.c -c -o hello.bc
```

lli是解释器，可以直接运行llvm bitcode：  
```bash
lli hello.bc
```

使用该llvm-dis实用程序查看 LLVM 汇编代码：  
```bash
llvm-dis < hello.bc | less
```



把C语言源码转换为LLVM IR：  
首先在multiply.c文件中编写一段C语言代码，如下：  
```cpp
$ cat multiply.c
int mult() {
int a =5;
int b = 3;
int c = a * b;
return c;
}
```

使用以下命令来将C语言代码转换成LLVM IR：
```bash
$ clang -emit-llvm -S multiply.c -o multiply.ll
```


将LLVM IR转换为bitcode：  
bitcode是什么？
> 是LLVM IR的二进制表示  

```bash
llvm-as test.ll –o test.bc
```
test.bc文件是二进制的，如果想查看：  
```bash
hexdump -C test.bc
```

将LLVM bitcode转换为目标平台汇编码:  
```bash
$ llc test.bc –o test.s
```
运行结果：  
```cpp
        .text
        .file   "testfile.ll"
        .globl  test1                           # -- Begin function test1
        .p2align        4, 0x90
        .type   test1,@function
test1:                                  # @test1
        .cfi_startproc
# %bb.0:
        movl    %edi, %eax
        retq
.Lfunc_end0:
        .size   test1, .Lfunc_end0-test1
        .cfi_endproc
                                        # -- End function
        .p2align        4, 0x90                         # -- Begin function test
        .type   test,@function
test:                                   # @test
        .cfi_startproc
# %bb.0:
        movl    %edi, %eax
        retq
.Lfunc_end1:
        .size   test, .Lfunc_end1-test
        .cfi_endproc
                                        # -- End function
        .globl  caller                          # -- Begin function caller
        .p2align        4, 0x90
        .type   caller,@function
caller:                                 # @caller
        .cfi_startproc
# %bb.0:
        pushq   %rax
        .cfi_def_cfa_offset 16
        movl    $123, %edi
        movl    $456, %esi                      # imm = 0x1C8
        callq   test
        popq    %rcx
        .cfi_def_cfa_offset 8
        retq
.Lfunc_end2:
        .size   caller, .Lfunc_end2-caller
        .cfi_endproc
                                        # -- End function
        .section        ".note.GNU-stack","",@progbits
```

使用clang从bitcode文件格式生成汇编码:  
```bash
$ clang -S test.bc -o test.s –fomit-frame-pointer
```

通过反汇编工具llvm-dis把LLVM bitcode转回为LLVM IR：  
```bash
$ llvm-dis test.bc –o test.ll
```

opt工具对IR代码实施优化：  

首先创建一个llvm IR文件:`testfile.ll`  
```cpp
$ cat testfile.ll
define i32 @test1(i32 %A) {
%B = add i32 %A, 0
ret i32 %B
}
define internal i32 @test(i32 %X, i32 %dead) {
ret i32 %X
}
define i32 @caller() {
%A
= call i32 @test(i32 123, i32 456)
ret i32 %A
}
```
指令合并优化:  
```bash
$ opt –S –instcombine testfile.ll –o output1.ll
```
运行结果:  
```cpp
$ cat output1.ll
; ModuleID = 'testfile.ll'
define i32 @test1(i32 %A) {
ret i32 %A
}
define internal i32 @test(i32 %X, i32 %dead) {
ret i32 %X
}
define i32 @caller() {
%A = call i32 @test(i32 123, i32 456)
ret i32 %A
}
```

无用参数消除优化(aka 死代码消除)：  
```bash
$ opt –S –deadargelim testfile.ll –o output2.ll
```

运行结果:  
```cpp
; ModuleID = 'testfile.ll'
source_filename = "testfile.ll"

define i32 @test1(i32 %A) {
  %B = add i32 %A, 0
  ret i32 %B
}

define internal i32 @test(i32 %X) {
  ret i32 %X
}

define i32 @caller() {
  %A = call i32 @test(i32 123)
  ret i32 %A
}

```

优化内存访问（将局部变量从内存提升到寄存器）：  
```bash
$ opt -mem2reg -S multiply.ll -o multiply1.ll
```

opt工具常用的pass:  
- adce：入侵式无用代码消除。
- bb-vectorize：基本块向量化。
- constprop：简单常量传播。
- dce：无用代码消除。
- deadargelim：无用参数消除。
- globaldce：无用全局变量消除。
- globalopt：全局变量优化。
- gvn：全局变量编号。
- inline：函数内联。
- instcombine：冗余指令合并。
- licm：循环常量代码外提。
- loop-unswitch：循环外提。
- loweratomic：原子内建函数lowering。
- lowerinvoke：invode指令lowering，以支持不稳定的代码生成器。
- lowerswitch：switch指令lowering。
- mem2reg：内存访问优化。
- memcpyopt：MemCpy优化。
- simplifycfg：简化CFG。
- sink：代码提升。
- tailcallelim：尾调用消除。

使用llvm-link工具链接LLVM bitcode:  
```bash
$ clang -emit-llvm -S test1.c -o test1.ll
$ clang -emit-llvm -S test2.c -o test2.ll
$ llvm-as test1.ll -o test1.bc
$ llvm-as test2.ll -o test2.bc
```

```bash
$ llvm-link test1.bc test2.bc –o output.bc
```

使用lli工具执行LLVM bitcode:  
```bash
$ lli output.bc
number is 10
```
