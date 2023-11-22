# LLVM IR reference manual learning
https://releases.llvm.org/11.0.0/docs/LangRef.html#instruction-reference  
## table of contents
- High level structure
including:module structure
function
...
- Type system
- constants
- Metadata
- Instruction reference
- Intrinsic Functions
## we start with instruction first
- Terminator instructions 终止符指令
- Unary operations
- Binary operations
- Bitwise Binary operations 按位二元运算
- Vector operations 向量运算
- Aggregate operations 聚合运算
-Memory access and addressing operations 访存和取址运算  
- Conversion operations (类型强转)
- other operations
### Terminator instructions
`ret`
```cpp
// Syntax:
ret <type> <value>       ; Return a value from a non-void function
ret void                 ; Return from void function
// Example:
ret i32 5                       ; Return an integer value of 5
ret void                        ; Return from a void function
ret { i32, i8 } { i32 4, i8 2 } ; Return a struct of values 
```

`br`  
```cpp
// Example:¶
Test:
  %cond = icmp eq i32 %a, %b
  br i1 %cond, label %IfEqual, label %IfUnequal
IfEqual:
  ret i32 1
IfUnequal:
  ret i32 0
```

`switch`
```cpp
// Example:
; Emulate a conditional br instruction
%Val = zext i1 %value to i32
switch i32 %Val, label %truedest [ i32 0, label %falsedest ]

; Emulate an unconditional br instruction
switch i32 0, label %dest [ ]

; Implement a jump table:
switch i32 %val, label %otherwise [ i32 0, label %onzero
                                    i32 1, label %onone
                                    i32 2, label %ontwo ]
```

`indirectbr`  
看不懂，可能是优化指令  
```cpp
//example
indirectbr i8* %Addr, [ label %bb1, label %bb2, label %bb3 ]
```
`invoke`  
相当于对`call`的拓展,主要是添加了对异常处理的支持  
```cpp
define i32 @example(i32 %a, i32 %b) personality i32 (...)* @personality_func {
entry:
  %result = invoke i32 @someFunction(i32 %a, i32 %b) to label %normal returns label %unwind

normal:
  ; 正常返回时的处理
  ret i32 %result

unwind:
  ; 异常处理时的处理
  unreachable
}
```
`callbr`:该指令只能用于实现 gcc 风格内联汇编的“goto”功能。任何其他用途都会导致 IR 验证器出错。  
`resume`:指令是没有后继指令的终止符指令。' resume' 指令恢复现有（运行中）异常的传播，该异常的展开被 登陆板指令中断。  
`catchswitch`:  
`catchret`  
`cleanupret`  
`unreachable`  
### unary operations
`fneg` : floating point negate  
```cpp
<result> = fneg float %val          ; yields float:result = -%var
```
### Binary operations
`add`  
`fadd`  
`sub`  
`fsub`  
`mul`  
`fmul`  
`udiv`:unsigned  
`sdiv`:signed  
`fdiv`:floating  
`urem`:reminder  
`srem`  
`frem`  
### Bitwise operations  
`shl`:shift to left  
`lshr`:logical shift right  
`ashr`:arithmetic shift right  
`and`  
`or`  
`xor`：exclusive or  
### vector operations
`extractelement`:extracts a single scalar element from a vector at a specified index  
`insertelement` inserts a scalar element into a vector at a specified index  
`shufflevector`: The ‘shufflevector’ instruction constructs a permutation of elements from two input vectors, returning a vector with the same element type as the input and length that is the same as the shuffle mask.  
### Aggregate operations 
`extractvalue`,`insertvalue`:操作结构体或数组  
### memory access and addressing operations
`alloca`:The ‘alloca’ instruction allocates memory on the stack frame of the currently executing function, to be automatically released when this function returns to its caller. The object is always allocated in the address space for allocas indicated in the datalayout.
```cpp
//example
%ptr = alloca i32                             ; yields i32*:ptr
%ptr = alloca i32, i32 4                      ; yields i32*:ptr
%ptr = alloca i32, i32 4, align 1024          ; yields i32*:ptr
%ptr = alloca i32, align 1024                 ; yields i32*:ptr
```
`load`:The ‘load’ instruction is used to read from memory.  
```cpp
// Examples:
%ptr = alloca i32                               ; yields i32*:ptr
store i32 3, i32* %ptr                          ; yields void
%val = load i32, i32* %ptr                      ; yields i32:val = i32 3
```
`store`:The ‘store’ instruction is used to write to memory.  
`fence`  
`cmpxchg`:原子指令  
`atomicrmw`:The ‘atomicrmw’ instruction is used to atomically modify memory  
`getelementptr`:The ‘getelementptr’ instruction is used to get the address of a subelement of an aggregate data structure. It performs address calculation only and does not access memory. The instruction can also be used to calculate a vector of such addresses.  
### conversion operations
`trunc`:截断，The ‘trunc’ instruction takes a value to trunc, and a type to trunc it to. Both types must be of integer types, or vectors of the same number of integers. The bit size of the value must be larger than the bit size of the destination type, ty2. Equal sized types are not allowed.  
`zext`:The ‘zext’ instruction zero extends its operand to type ty2.  
`sext`:The ‘sext’ sign extends value to the type ty2.  
`fptrunc`:floating-point trunc  
`fpext`  
`fptoui`:The ‘fptoui’ converts a floating-point value to its unsigned integer equivalent of type ty2.  
`fptosi`  
`uitofp`  
`sitofp`  
`ptrtoint`  
`inttoptr`  
`bitcast`:The ‘bitcast’ instruction converts value to type ty2 without changing any bits.(我的理解：能用于各种基本类型强转)  
`addrspacecast`:The ‘addrspacecast’ instruction converts ptrval from pty in address space n to type pty2 in address space m.  
`icmp`:The ‘icmp’ instruction returns a boolean value or a vector of boolean values based on comparison of its two integer, integer vector, pointer, or pointer vector operands.  
```cpp
<result> = icmp eq i32 4, 5          ; yields: result=false
<result> = icmp ne float* %X, %X     ; yields: result=false
<result> = icmp ult i16  4, 5        ; yields: result=true
<result> = icmp sgt i16  4, 5        ; yields: result=false
<result> = icmp ule i16 -4, 5        ; yields: result=false
<result> = icmp sge i16  4, 5        ; yields: result=false
```
`fcmp`:The ‘fcmp’ instruction returns a boolean value or vector of boolean values based on comparison of its operands.

If the operands are floating-point scalars, then the result type is a boolean (i1).

If the operands are floating-point vectors, then the result type is a vector of boolean with the same number of elements as the operands being compared.  
`phi`:The ‘phi’ instruction is used to implement the φ node in the SSA graph representing the function.  
`select`：实现三元运算符  
`freeze`:把一个数冻结，方便编译器后续优化  
`call`  
`va_arg`:The ‘va_arg’ instruction is used to access arguments passed through the “variable argument” area of a function call. It is used to implement the va_arg macro in C.  
`landingpad`：用于异常处理  
`catchpad`  
`cleanuppad`  

## High level structure
![IR layout](https://img2023.cnblogs.com/blog/2334219/202306/2334219-20230617201736543-1183820387.png)
简而言之：module对应一个.ll文件，包含函数、全局变量、符号表等  
function对应一个函数，包含参数，entry基本块，其他基本块等  
basic block代表一个基本块，包含label phi等  