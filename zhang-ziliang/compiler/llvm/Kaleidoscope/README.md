# llvm前端入门

## chapter3

词法分析：  
```cpp
enum Token {
  tok_eof = -1,

  // commands
  tok_def = -2,
  tok_extern = -3,

  // primary
  tok_identifier = -4,
  tok_number = -5
};

static std::string IdentifierStr; // Filled in if tok_identifier
static double NumVal;             // Filled in if tok_number

static int gettok()
```

gettok代码思路：  
如果是字母：读出整个单词，记为identifier，存在全局变量**IdentifierStr**中  
如果是数字：读出整个数，记为number，存在全局变量**NumVal**中  
如果是'#'，跳过这一行  
如果是其他字符，例如运算符和括号，直接return该字符，而不是return枚举值  

语法分析：AST类存储AST节点，parser函数构建AST  

AST节点类型：  
基类：ExprAST  
子类：  
NumberExprAST  
VariableExprAST
BinaryExprAST

函数AST节点类型不继承ExprAST：
FunctionAST ：  
  std::unique_ptr<PrototypeAST> Proto;  
  std::unique_ptr<ExprAST> Body;  


生成IR code：  
```cpp
Value *NumberExprAST::codegen() {
  return ConstantFP::get(*TheContext, APFloat(Val));
}
Value *VariableExprAST::codegen() {
  // Look this variable up in the function.
  Value *V = NamedValues[Name];
  if (!V)
    return LogErrorV("Unknown variable name");
  return V;
}
```

```cpp
Value *BinaryExprAST::codegen() {
  Value *L = LHS->codegen();
  Value *R = RHS->codegen();
  if (!L || !R)
    return nullptr;

  switch (Op) {
  case '+':
    return Builder->CreateFAdd(L, R, "addtmp");
  case '-':
    return Builder->CreateFSub(L, R, "subtmp");
  case '*':
    return Builder->CreateFMul(L, R, "multmp");
  case '<':
    L = Builder->CreateFCmpULT(L, R, "cmptmp");
    // Convert bool 0/1 to double 0.0 or 1.0
    return Builder->CreateUIToFP(L, Type::getDoubleTy(*TheContext), "booltmp");
  default:
    return LogErrorV("invalid binary operator");
  }
}
```
使用IRBuilder类的接口生成IR code  
运行效果：  
```cpp
%multmp = fmul double %a, %b
```

函数调用：  
```cpp
Value *CallExprAST::codegen() {
  // Look up the name in the global module table.
  Function *CalleeF = TheModule->getFunction(Callee);
  if (!CalleeF)
    return LogErrorV("Unknown function referenced");

  // If argument mismatch error.
  if (CalleeF->arg_size() != Args.size())
    return LogErrorV("Incorrect # arguments passed");

  std::vector<Value *> ArgsV;
  for (unsigned i = 0, e = Args.size(); i != e; ++i) {
    ArgsV.push_back(Args[i]->codegen());
    if (!ArgsV.back())
      return nullptr;
  }

  return Builder->CreateCall(CalleeF, ArgsV, "calltmp");
}
```

一旦我们有了要调用的函数，我们就会递归地编码生成要传入的每个参数，并创建一个 LLVM调用指令  
运行效果：  
```CPP
%calltmp = call double @foo(double %a, double 4.000000e+00)
```


### 函数IR代码生成  

```cpp
Function *PrototypeAST::codegen() {
  // Make the function type:  double(double,double) etc.
  std::vector<Type*> Doubles(Args.size(),
                             Type::getDoubleTy(*TheContext));
  FunctionType *FT =
    FunctionType::get(Type::getDoubleTy(*TheContext), Doubles, false);

  Function *F =
    Function::Create(FT, Function::ExternalLinkage, Name, TheModule.get());
  // Set names for all arguments.
  unsigned Idx = 0;
  for (auto &Arg : F->args())
  Arg.setName(Args[Idx++]);

  return F;
```

设置functionType  
将函数名注册在“ TheModule”的符号表中  
将参数名注册在Function中  

```cpp
Function *FunctionAST::codegen() {
    // First, check for an existing function from a previous 'extern' declaration.
  Function *TheFunction = TheModule->getFunction(Proto->getName());

  if (!TheFunction)
    TheFunction = Proto->codegen();

  if (!TheFunction)
    return nullptr;

  if (!TheFunction->empty())
    return (Function*)LogErrorV("Function cannot be redefined.");
```

对于函数定义，我们首先在 TheModule 的符号表中搜索该函数的现有版本，以防已经使用“extern”语句创建了该函数。如果 Module::getFunction 返回 null，则先前版本不存在，因此我们将从 Prototype 中生成一个版本。  

```cpp
// Create a new basic block to start insertion into.
BasicBlock *BB = BasicBlock::Create(*TheContext, "entry", TheFunction);
Builder->SetInsertPoint(BB);

// Record the function arguments in the NamedValues map.
NamedValues.clear();
for (auto &Arg : TheFunction->args())
  NamedValues[std::string(Arg.getName())] = &Arg;
```
Builder现在我们到了设置的地方。第一行创建一个新的基本块 （名为“entry”），将其插入到TheFunction. 然后，第二行告诉构建器应将新指令插入到新基本块的末尾。  

接下来，我们将函数参数添加到 NamedValues 映射（首先将其清除之后），以便VariableExprAST节点可以访问它们。  
我的理解：NamedValues存的是函数中的局部变量，函数调用结束后会被销毁  

```cpp
if (Value *RetVal = Body->codegen()) {
  // Finish off the function.
  Builder->CreateRet(RetVal);

  // Validate the generated code, checking for consistency.
  verifyFunction(*TheFunction);

  return TheFunction;
}
```
创建ret指令，使用`ret`指令将返回地址从堆栈中弹出，并跳转到返回地址所指向的位置，从而实现子程序的返回。  
运行效果：  
```cpp
ready> def foo(a b) a*a + 2*a*b + b*b;
Read function definition:
define double @foo(double %a, double %b) {
entry:
  %multmp = fmul double %a, %a
  %multmp1 = fmul double 2.000000e+00, %a
  %multmp2 = fmul double %multmp1, %b
  %addtmp = fadd double %multmp, %multmp2
  %multmp3 = fmul double %b, %b
  %addtmp4 = fadd double %addtmp, %multmp3
  ret double %addtmp4
}
```

## chapter4
一个代码优化的小例子，常量折叠：  
```cpp
ready> def test(x) 1+2+x;
Read function definition:
define double @test(double %x) {
entry:
        %addtmp = fadd double 2.000000e+00, 1.000000e+00
        %addtmp1 = fadd double %addtmp, %x
        ret double %addtmp1
}
```

```cpp
ready> def test(x) 1+2+x;
Read function definition:
define double @test(double %x) {
entry:
        %addtmp = fadd double 3.000000e+00, %x
        ret double %addtmp
}
```

然而一般的代码优化只能处理简单的场景：  
```cpp
ready> def test(x) (1+2+x)*(x+(1+2));
ready> Read function definition:
define double @test(double %x) {
entry:
        %addtmp = fadd double 3.000000e+00, %x
        %addtmp1 = fadd double %x, 3.000000e+00
        %multmp = fmul double %addtmp, %addtmp1
        ret double %multmp
}
```

在这种情况下，乘法的左侧和右侧是相同的值。我们真的很希望看到它生成“ tmp = x+3;result = tmp*tmp; ”而不是计算“ x+3 ”两次。 

不幸的是，无论进行多少本地分析都无法检测并纠正这一问题。这需要两个转换：表达式的重新关联（以使加法的词法相同）和公共子表达式消除（CSE）以删除冗余的加法指令。幸运的是，LLVM 以“pass”的形式提供了广泛的优化供您使用。

```cpp
void InitializeModuleAndManagers(void) {
  // Open a new context and module.
  TheContext = std::make_unique<LLVMContext>();
  TheModule = std::make_unique<Module>("KaleidoscopeJIT", *TheContext);
  TheModule->setDataLayout(TheJIT->getDataLayout());

  // Create a new builder for the module.
  Builder = std::make_unique<IRBuilder<>>(*TheContext);

  // Create new pass and analysis managers.
  TheFPM = std::make_unique<FunctionPassManager>();
  TheFAM = std::make_unique<FunctionAnalysisManager>();
  TheMAM = std::make_unique<ModuleAnalysisManager>();
  ThePIC = std::make_unique<PassInstrumentationCallbacks>();
  TheSI = std::make_unique<StandardInstrumentations>(*TheContext,
                                                    /*DebugLogging*/ true);
  TheSI->registerCallbacks(*ThePIC, TheMAM.get());
  ...
  ```
初始化全局模块TheModule和FunctionPassManager之后，我们需要初始化框架的其他部分。  
FunctionAnalysisManager 和 ModuleAnalysisManager 允许我们添加分别在函数和整个模块上运行的分析过程。  
PassInstrumentationCallbacks 和 StandardInstrumentations 是传递检测框架所必需的，它允许开发人员自定义传递之间发生的情况。  
一旦设置了这些管理器，我们就使用一系列“addPass”调用来添加一堆 LLVM 转换过程：  
```cpp
// Add transform passes.
// Do simple "peephole" optimizations and bit-twiddling optzns.
TheFPM->addPass(InstCombinePass());
// Reassociate expressions.
TheFPM->addPass(ReassociatePass());
// Eliminate Common SubExpressions.
TheFPM->addPass(GVNPass());
// Simplify the control flow graph (deleting unreachable blocks, etc).
TheFPM->addPass(SimplifyCFGPass());

```
接下来注册对应的analysis pass  
```cpp
// Register analysis passes used in these transform passes.
  TheFAM->registerPass([&] { return AAManager(); });
  TheFAM->registerPass([&] { return AssumptionAnalysis(); });
  TheFAM->registerPass([&] { return DominatorTreeAnalysis(); });
  TheFAM->registerPass([&] { return LoopAnalysis(); });
  TheFAM->registerPass([&] { return MemoryDependenceAnalysis(); });
  TheFAM->registerPass([&] { return MemorySSAAnalysis(); });
  TheFAM->registerPass([&] { return OptimizationRemarkEmitterAnalysis(); });
  TheFAM->registerPass([&] {
    return OuterAnalysisManagerProxy<ModuleAnalysisManager, Function>(*TheMAM);
  });
  TheFAM->registerPass(
      [&] { return PassInstrumentationAnalysis(ThePIC.get()); });
  TheFAM->registerPass([&] { return TargetIRAnalysis(); });
  TheFAM->registerPass([&] { return TargetLibraryAnalysis(); });

  TheMAM->registerPass([&] { return ProfileSummaryAnalysis(); });
}
```

一旦 PassManager 设置完毕，我们就需要使用它。在 FunctionAST::codegen 中()：  
```cpp
if (Value *RetVal = Body->codegen()) {
  // Finish off the function.
  Builder.CreateRet(RetVal);

  // Validate the generated code, checking for consistency.
  verifyFunction(*TheFunction);

  // Optimize the function.
  TheFPM->run(*TheFunction, *TheFAM);

  return TheFunction;
}
```

运行效果：  
```cpp
ready> def test(x) (1+2+x)*(x+(1+2));
ready> Read function definition:
define double @test(double %x) {
entry:
        %addtmp = fadd double %x, 3.000000e+00
        %multmp = fmul double %addtmp, %addtmp
        ret double %multmp
}
```

### 添加 JIT 编译器  
KaleidscopeJIT 类是专门为教程构建的简单 JIT，现在我们将其视为给定的，先学习怎么用，后面再学原理。  
在main中初始化：  
```cpp
static std::unique_ptr<KaleidoscopeJIT> TheJIT;
...
int main() {
  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();
  InitializeNativeTargetAsmParser();

  // Install standard binary operators.
  // 1 is lowest precedence.
  BinopPrecedence['<'] = 10;
  BinopPrecedence['+'] = 20;
  BinopPrecedence['-'] = 20;
  BinopPrecedence['*'] = 40; // highest.

  // Prime the first token.
  fprintf(stderr, "ready> ");
  getNextToken();

  TheJIT = std::make_unique<KaleidoscopeJIT>();

  // Run the main "interpreter loop" now.
  MainLoop();

  return 0;
}
```
设置DataLayout：  
```cpp
void InitializeModuleAndPassManager(void) {
  // Open a new context and module.
  TheContext = std::make_unique<LLVMContext>();
  TheModule = std::make_unique<Module>("my cool jit", TheContext);
  TheModule->setDataLayout(TheJIT->getDataLayout());

  // Create a new builder for the module.
  Builder = std::make_unique<IRBuilder<>>(*TheContext);

  // Create a new pass manager attached to it.
  TheFPM = std::make_unique<legacy::FunctionPassManager>(TheModule.get());
  ...
```
```cpp
static ExitOnError ExitOnErr;
...
static void HandleTopLevelExpression() {
  // Evaluate a top-level expression into an anonymous function.
  if (auto FnAST = ParseTopLevelExpr()) {
    if (FnAST->codegen()) {
      // Create a ResourceTracker to track JIT'd memory allocated to our
      // anonymous expression -- that way we can free it after executing.
      auto RT = TheJIT->getMainJITDylib().createResourceTracker();

      auto TSM = ThreadSafeModule(std::move(TheModule), std::move(TheContext));
      ExitOnErr(TheJIT->addModule(std::move(TSM), RT));
      InitializeModuleAndPassManager();

      // Search the JIT for the __anon_expr symbol.
      auto ExprSymbol = ExitOnErr(TheJIT->lookup("__anon_expr"));
      assert(ExprSymbol && "Function not found");

      // Get the symbol's address and cast it to the right type (takes no
      // arguments, returns a double) so we can call it as a native function.
      double (*FP)() = ExprSymbol.getAddress().toPtr<double (*)()>();
      fprintf(stderr, "Evaluated to %f\n", FP());

      // Delete the anonymous expression module from the JIT.
      ExitOnErr(RT->remove());
    }
```
如果解析和代码生成成功，下一步是将包含顶级表达式的模块添加到 JIT。我们通过调用 addModule 来完成此操作，这会触发模块中所有函数的代码生成，并接受 ResourceTracker可用于稍后从 JIT 中删除模块的 a 。一旦模块被添加到 JIT 中，就无法再修改，因此我们还通过调用 来打开一个新模块来保存后续代码InitializeModuleAndPassManager()。

将模块添加到 JIT 后，我们需要获取指向最终生成代码的指针。我们通过调用 JIT 的lookup方法并传递顶级表达式函数的名称来实现此目的：__anon_expr。由于我们刚刚添加了这个函数，我们断言lookup返回了一个结果。
__anon_expr接下来，我们通过调用 getAddress()符号来获取函数的内存地址。回想一下，我们将顶级表达式编译成一个独立的 LLVM 函数，该函数不带参数并返回计算双精度值。由于 LLVM JIT 编译器与本机平台 ABI 相匹配，这意味着您可以将结果指针强制转换为该类型的函数指针并直接调用它。这意味着，JIT 编译的代码和静态链接到应用程序的本机机器代码之间没有区别。

运行效果：  
```cpp
ready> 4+5;
Read top-level expression:
define double @0() {
entry:
  ret double 9.000000e+00
}

Evaluated to 9.000000
```

我们甚至还可以通过调用动态库来使用c函数  
```cpp
#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

/// putchard - putchar that takes a double and returns 0.
extern "C" DLLEXPORT double putchard(double X) {
  fputc((char)X, stderr);
  return 0;
}
```

## chapter5
### 实现if else 控制流

添加枚举值:  
```cpp
// control
tok_if = -6,
tok_then = -7,
tok_else = -8,
```

解析关键字  
```cpp
...
if (IdentifierStr == "def")
  return tok_def;
if (IdentifierStr == "extern")
  return tok_extern;
if (IdentifierStr == "if")
  return tok_if;
if (IdentifierStr == "then")
  return tok_then;
if (IdentifierStr == "else")
  return tok_else;
return tok_identifier;
```

添加AST
```cpp
/// IfExprAST - Expression class for if/then/else.
class IfExprAST : public ExprAST {
  std::unique_ptr<ExprAST> Cond, Then, Else;

public:
  IfExprAST(std::unique_ptr<ExprAST> Cond, std::unique_ptr<ExprAST> Then,
            std::unique_ptr<ExprAST> Else)
    : Cond(std::move(Cond)), Then(std::move(Then)), Else(std::move(Else)) {}

  Value *codegen() override;
};
```

添加parser
```cpp
/// ifexpr ::= 'if' expression 'then' expression 'else' expression
static std::unique_ptr<ExprAST> ParseIfExpr() {
  getNextToken();  // eat the if.

  // condition.
  auto Cond = ParseExpression();
  if (!Cond)
    return nullptr;

  if (CurTok != tok_then)
    return LogError("expected then");
  getNextToken();  // eat the then

  auto Then = ParseExpression();
  if (!Then)
    return nullptr;

  if (CurTok != tok_else)
    return LogError("expected else");

  getNextToken();

  auto Else = ParseExpression();
  if (!Else)
    return nullptr;

  return std::make_unique<IfExprAST>(std::move(Cond), std::move(Then),
                                      std::move(Else));
}
```

```cpp
static std::unique_ptr<ExprAST> ParsePrimary() {
  switch (CurTok) {
  default:
    return LogError("unknown token when expecting an expression");
  case tok_identifier:
    return ParseIdentifierExpr();
  case tok_number:
    return ParseNumberExpr();
  case '(':
    return ParseParenExpr();
  case tok_if:
    return ParseIfExpr();
  }
}
```

先看一个简单示例：
```cpp
extern foo();
extern bar();
def baz(x) if x then foo() else bar();
```

```cpp
declare double @foo()

declare double @bar()

define double @baz(double %x) {
entry:
  %ifcond = fcmp one double %x, 0.000000e+00
  br i1 %ifcond, label %then, label %else

then:       ; preds = %entry
  %calltmp = call double @foo()
  br label %ifcont

else:       ; preds = %entry
  %calltmp1 = call double @bar()
  br label %ifcont

ifcont:     ; preds = %else, %then
  %iftmp = phi double [ %calltmp, %then ], [ %calltmp1, %else ]
  ret double %iftmp
}
```

phi函数是什么？

>phi函数是用于在SSA（Static Single Assignment）形式中进行变量值合并的特殊函数。它是SSA形式中的一种指令或操作，用于解决控制流图中不同基本块之间变量赋值的问题。在SSA形式中，每个变量都只能被赋值一次。但是，在存在条件分支（如if-else语句）的情况下，变量的赋值可能存在不同的路径。为了处理这种情况，就需要使用phi函数。

>phi函数接受一个变量在不同控制流路径上的可能的赋值，并根据当前控制流的来源来选择正确的值。phi函数通常用于基本块的开头，并在控制流图中的分支节点处使用，以合并不同路径上的变量值。

例如，考虑以下示例代码片段：

```
if condition:
    x = 1
else:
    x = 2
```

在SSA形式下，可以使用phi函数来表示变量"x"的赋值操作：

```
x1 = phi(1, 2)
```

根据当前控制流到达分支节点的路径，phi函数会选择1或2作为变量"x"的值。

通过使用phi函数，编译器可以在变量的不同路径上选择正确的值，以保持SSA形式的一致性，并为后续的优化和分析提供准确的信息。

添加codegen函数

```cpp
Value *IfExprAST::codegen() {
  Value *CondV = Cond->codegen();
  if (!CondV)
    return nullptr;

  // Convert condition to a bool by comparing non-equal to 0.0.
  CondV = Builder->CreateFCmpONE(
      CondV, ConstantFP::get(*TheContext, APFloat(0.0)), "ifcond");
```

```cpp
Function *TheFunction = Builder->GetInsertBlock()->getParent();

// Create blocks for the then and else cases.  Insert the 'then' block at the
// end of the function.
BasicBlock *ThenBB =
    BasicBlock::Create(*TheContext, "then", TheFunction);
BasicBlock *ElseBB = BasicBlock::Create(*TheContext, "else");
BasicBlock *MergeBB = BasicBlock::Create(*TheContext, "ifcont");

Builder->CreateCondBr(CondV, ThenBB, ElseBB);
```
创建三个基本块，创建条件跳转指令

```cpp
// Emit then value.
Builder->SetInsertPoint(ThenBB);

Value *ThenV = Then->codegen();
if (!ThenV)
  return nullptr;

Builder->CreateBr(MergeBB);
// Codegen of 'Then' can change the current block, update ThenBB for the PHI.
ThenBB = Builder->GetInsertBlock();
```

`SetInsertPoint`设置插入点，插入点是指定在控制流图中的哪个位置插入新的指令，严格来说，此调用将插入点移动到指定块的末尾  
`CreateBr`无条件跳转到指定基本块，这里就是在then基本块的末尾加入无条件跳转到merge基本块的代码  

>Codegen of 'Then' can change the current block, update ThenBB for the PHI.  
插入代码后更新一下基本块


```CPP
// Emit else block.
TheFunction->insert(TheFunction->end(), ElseBB);
Builder->SetInsertPoint(ElseBB);

Value *ElseV = Else->codegen();
if (!ElseV)
  return nullptr;

Builder->CreateBr(MergeBB);
// codegen of 'Else' can change the current block, update ElseBB for the PHI.
ElseBB = Builder->GetInsertBlock();
```

“else”块的代码生成基本上与“then”块的代码生成相同  

>codegen of 'Else' can change the current block, update ElseBB for the PHI.  
插入代码后更新一下基本块

```CPP
// Emit merge block.
  TheFunction->insert(TheFunction->end(), MergeBB);
  Builder->SetInsertPoint(MergeBB);
  PHINode *PN =
    Builder->CreatePHI(Type::getDoubleTy(*TheContext), 2, "iftmp");

  PN->addIncoming(ThenV, ThenBB);
  PN->addIncoming(ElseV, ElseBB);
  return PN;
}
```

### for循环表达式
添加枚举值，gettok中添加关键字解析，略  

AST节点
```CPP
/// ForExprAST - Expression class for for/in.
class ForExprAST : public ExprAST {
  std::string VarName;
  std::unique_ptr<ExprAST> Start, End, Step, Body;

public:
  ForExprAST(const std::string &VarName, std::unique_ptr<ExprAST> Start,
             std::unique_ptr<ExprAST> End, std::unique_ptr<ExprAST> Step,
             std::unique_ptr<ExprAST> Body)
    : VarName(VarName), Start(std::move(Start)), End(std::move(End)),
      Step(std::move(Step)), Body(std::move(Body)) {}

  Value *codegen() override;
};
```
AST简单明了，所以parser函数的解析流程也简单明了，分别解析`Start, End, Step, Body`四个部分，省略  

添加到顶层解析`ParsePrimary`中，略  
#### 重点部分 LLVM IR
预期效果：  
```CPP
declare double @putchard(double)

define double @printstar(double %n) {
entry:
  ; initial value = 1.0 (inlined into phi)
  br label %loop

loop:       ; preds = %loop, %entry
  %i = phi double [ 1.000000e+00, %entry ], [ %nextvar, %loop ]
  ; body
  %calltmp = call double @putchard(double 4.200000e+01)
  ; increment
  %nextvar = fadd double %i, 1.000000e+00

  ; termination test
  %cmptmp = fcmp ult double %i, %n
  %booltmp = uitofp i1 %cmptmp to double
  %loopcond = fcmp one double %booltmp, 0.000000e+00
  br i1 %loopcond, label %loop, label %afterloop

afterloop:      ; preds = %loop
  ; loop always returns 0.0
  ret double 0.000000e+00
}
```
开始codegen部分

```CPP
Value *ForExprAST::codegen() {
  // Emit the start code first, without 'variable' in scope.
  Value *StartVal = Start->codegen();
  if (!StartVal)
    return nullptr;
```
解析start部分表达式  

```CPP
// Make the new basic block for the loop header, inserting after current
// block.
Function *TheFunction = Builder->GetInsertBlock()->getParent();
BasicBlock *PreheaderBB = Builder->GetInsertBlock();
BasicBlock *LoopBB =
    BasicBlock::Create(*TheContext, "loop", TheFunction);

// Insert an explicit fall through from the current block to the LoopBB.
Builder->CreateBr(LoopBB);
```

`TheFunction`获取`parent block`，后面创建`LoopBB`时需要用到  
然后无条件跳转到`LoopBB`  

```CPP
// Start insertion in LoopBB.
Builder->SetInsertPoint(LoopBB);

// Start the PHI node with an entry for Start.
PHINode *Variable = Builder->CreatePHI(Type::getDoubleTy(*TheContext),
                                       2, VarName);
Variable->addIncoming(StartVal, PreheaderBB);
```
对应的IR CODE
```CPP
%i = phi double [ 1.000000e+00, %entry ], [ %nextvar, %loop ]
```
翻译过来的意思就是，如果是从entry块进来的，就默认为1，如果是loop块跳过来的，就nextvar  

```CPP
// Within the loop, the variable is defined equal to the PHI node.  If it
// shadows an existing variable, we have to restore it, so save it now.
Value *OldVal = NamedValues[VarName];
NamedValues[VarName] = Variable;

// Emit the body of the loop.  This, like any other expr, can change the
// current BB.  Note that we ignore the value computed by the body, but don't
// allow an error.
if (!Body->codegen())
  return nullptr;
```

如果for里面的局部变量与for外面的变量重复了，记得用完了要恢复  

```CPP
// Emit the step value.
Value *StepVal = nullptr;
if (Step) {
  StepVal = Step->codegen();
  if (!StepVal)
    return nullptr;
} else {
  // If not specified, use 1.0.
  StepVal = ConstantFP::get(*TheContext, APFloat(1.0));
}

Value *NextVar = Builder->CreateFAdd(Variable, StepVal, "nextvar");

```

计算step表达式，如果没有指定，默认为1，stepval等于立即数1.0，nextval加上stepval的值  

```CPP
// Compute the end condition.
Value *EndCond = End->codegen();
if (!EndCond)
  return nullptr;

// Convert condition to a bool by comparing non-equal to 0.0.
EndCond = Builder->CreateFCmpONE(
    EndCond, ConstantFP::get(*TheContext, APFloat(0.0)), "loopcond");
```

结束条件就是cmp一下  

```CPP
// Create the "after loop" block and insert it.
BasicBlock *LoopEndBB = Builder->GetInsertBlock();
BasicBlock *AfterBB =
    BasicBlock::Create(*TheContext, "afterloop", TheFunction);

// Insert the conditional branch into the end of LoopEndBB.
Builder->CreateCondBr(EndCond, LoopBB, AfterBB);

// Any new code will be inserted in AfterBB.
Builder->SetInsertPoint(AfterBB);
```
创建条件分支，判断是否结束循环  
更新插入点位置  

```CPP
  // Add a new entry to the PHI node for the backedge.
  Variable->addIncoming(NextVar, LoopEndBB);

  // Restore the unshadowed variable.
  if (OldVal)
    NamedValues[VarName] = OldVal;
  else
    NamedValues.erase(VarName);

  // for expr always returns 0.0.
  return Constant::getNullValue(Type::getDoubleTy(*TheContext));
}
```

如果跳出循环了，variable更新到最后一次nextval  
如果存在varname同名的全局变量，就恢复  
没有就删掉这个局部变量  

## chapter6 用户定义的运算符
什么意思？  
添加`binary`，`unary`关键字，可以使用该关键字定义运算符函数，并且指定优先级  
### 二元运算符
添加枚举类型，gettok解析关键字，略  
AST节点：  
```cpp
/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its argument names as well as if it is an operator.
class PrototypeAST {
  std::string Name;
  std::vector<std::string> Args;
  bool IsOperator;
  unsigned Precedence;  // Precedence if a binary op.

public:
  PrototypeAST(const std::string &Name, std::vector<std::string> Args,
               bool IsOperator = false, unsigned Prec = 0)
  : Name(Name), Args(std::move(Args)), IsOperator(IsOperator),
    Precedence(Prec) {}

  Function *codegen();
  const std::string &getName() const { return Name; }

  bool isUnaryOp() const { return IsOperator && Args.size() == 1; }
  bool isBinaryOp() const { return IsOperator && Args.size() == 2; }

  char getOperatorName() const {
    assert(isUnaryOp() || isBinaryOp());
    return Name[Name.size() - 1];
  }

  unsigned getBinaryPrecedence() const { return Precedence; }
};
```
我们需要拓展PrototypeAST,增加一个变量记录二元运算符的优先级,
以及判断运算符类型,函数名即为运算符的符号  

修改`ParsePrototype`,处理binary关键字,略  

修改`BinaryExprAST::codegen`  

```cpp
Value *BinaryExprAST::codegen() {
  Value *L = LHS->codegen();
  Value *R = RHS->codegen();
  if (!L || !R)
    return nullptr;

  switch (Op) {
  case '+':
    return Builder->CreateFAdd(L, R, "addtmp");
  case '-':
    return Builder->CreateFSub(L, R, "subtmp");
  case '*':
    return Builder->CreateFMul(L, R, "multmp");
  case '<':
    L = Builder->CreateFCmpULT(L, R, "cmptmp");
    // Convert bool 0/1 to double 0.0 or 1.0
    return Builder->CreateUIToFP(L, Type::getDoubleTy(*TheContext),
                                "booltmp");
  default:
    break;
  }

  // If it wasn't a builtin binary operator, it must be a user defined one. Emit
  // a call to it.
  Function *F = getFunction(std::string("binary") + Op);
  assert(F && "binary operator not found!");

  Value *Ops[2] = { L, R };
  return Builder->CreateCall(F, Ops, "binop");
}
```
使用`getFunction`获取用户自定义运算符,然后使用`Builder->CreateCall`解析为Function  call  

最后,需要修改`FunctionAST::codegen()`  

```cpp
Function *FunctionAST::codegen() {
  // Transfer ownership of the prototype to the FunctionProtos map, but keep a
  // reference to it for use below.
  auto &P = *Proto;
  FunctionProtos[Proto->getName()] = std::move(Proto);
  Function *TheFunction = getFunction(P.getName());
  if (!TheFunction)
    return nullptr;

  // If this is an operator, install it.
  if (P.isBinaryOp())
    BinopPrecedence[P.getOperatorName()] = P.getBinaryPrecedence();

  // Create a new basic block to start insertion into.
  BasicBlock *BB = BasicBlock::Create(*TheContext, "entry", TheFunction);
  ...
```
判断如果是二元运算符,将运算符的优先级注册到优先级表中  

### 一元运算符  

添加`UnaryExprAST`  

```cpp
/// UnaryExprAST - Expression class for a unary operator.
class UnaryExprAST : public ExprAST {
  char Opcode;
  std::unique_ptr<ExprAST> Operand;

public:
  UnaryExprAST(char Opcode, std::unique_ptr<ExprAST> Operand)
    : Opcode(Opcode), Operand(std::move(Operand)) {}

  Value *codegen() override;
};
```

添加`ParseUnary()`函数  

```cpp
/// unary
///   ::= primary
///   ::= '!' unary
static std::unique_ptr<ExprAST> ParseUnary() {
  // If the current token is not an operator, it must be a primary expr.
  if (!isascii(CurTok) || CurTok == '(' || CurTok == ',')
    return ParsePrimary();

  // If this is a unary operator, read it.
  int Opc = CurTok;
  getNextToken();
  if (auto Operand = ParseUnary())
    return std::make_unique<UnaryExprAST>(Opc, std::move(Operand));
  return nullptr;
}
```

修改`ParseBinOpRHS`和`ParseExpression`,在解析lhs和rhs之前,先执行一遍`ParseUnary()`  

```cpp
/// binoprhs
///   ::= ('+' unary)*
static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,
                                              std::unique_ptr<ExprAST> LHS) {
  ...
    // Parse the unary expression after the binary operator.
    auto RHS = ParseUnary();
    if (!RHS)
      return nullptr;
  ...
}
/// expression
///   ::= unary binoprhs
///
static std::unique_ptr<ExprAST> ParseExpression() {
  auto LHS = ParseUnary();
  if (!LHS)
    return nullptr;

  return ParseBinOpRHS(0, std::move(LHS));
}
```
修改`ParsePrototype()`,解析一元运算符  

```cpp
/// prototype
///   ::= id '(' id* ')'
///   ::= binary LETTER number? (id, id)
///   ::= unary LETTER (id)
static std::unique_ptr<PrototypeAST> ParsePrototype() {
  std::string FnName;

  unsigned Kind = 0;  // 0 = identifier, 1 = unary, 2 = binary.
  unsigned BinaryPrecedence = 30;

  switch (CurTok) {
  default:
    return LogErrorP("Expected function name in prototype");
  case tok_identifier:
    FnName = IdentifierStr;
    Kind = 0;
    getNextToken();
    break;
  case tok_unary:
    getNextToken();
    if (!isascii(CurTok))
      return LogErrorP("Expected unary operator");
    FnName = "unary";
    FnName += (char)CurTok;
    Kind = 1;
    getNextToken();
    break;
  case tok_binary:
    ...
```

添加codegen,流程类似,找到对应函数,解析为call指令  

```cpp
Value *UnaryExprAST::codegen() {
  Value *OperandV = Operand->codegen();
  if (!OperandV)
    return nullptr;

  Function *F = getFunction(std::string("unary") + Opcode);
  if (!F)
    return LogErrorV("Unknown unary operator");

  return Builder->CreateCall(F, OperandV, "unop");
}
```

运行效果:  

```cpp
ready> extern printd(x);
Read extern:
declare double @printd(double)

ready> def binary : 1 (x y) 0;  # Low-precedence operator that ignores operands.
...
ready> printd(123) : printd(456) : printd(789);
123.000000
456.000000
789.000000
Evaluated to 0.000000
```

## chapter7 添加可变变量
example  
```cpp
define i32 @example() {
entry:
  %X = alloca i32           ; type of %X is i32*.
  ...
  %tmp = load i32, i32* %X  ; load the stack value %X from the stack.
  %tmp2 = add i32 %tmp, 1   ; increment it
  store i32 %tmp2, i32* %X  ; store it back
  ...
```
`alloca`指令在当前执行函数的堆栈帧上分配内存，当该函数返回到其调用者时自动释放内存。  
其中可以看出,`%X`存储的是i32的地址,而非值,想要使用变量,需要从栈上取数和回写,aka `load`,`store`  

使用可变变量会导致SSA构造复杂？
```
使用可变变量（Mutable Variables）可能会导致在构建SSA（Static Single Assignment）形式时更复杂。SSA形式要求每个变量只能被赋值一次，并且在程序中具有唯一的定义。然而，如果存在可变变量，也就是允许对变量进行多次赋值的情况，那么在构建SSA形式时就需要进行特殊处理。以下是使用可变变量时可能导致SSA构造复杂的原因：
1. 重新赋值：可变变量允许对同一个变量进行多次赋值。这意味着在构建SSA形式时，变量的定义将具有多个版本，每个版本对应于不同的赋值点。这会增加phi函数的数量，并增加了变量定义和使用关系的复杂性和不确定性。
2. 别名问题：可变变量可能导致别名（Alias）问题。如果多个变量引用了同一个可变变量，编译器在构建SSA形式时可能会面临处理别名的困难。这可能需要进行别名分析和控制流分析来确保变量的定义和使用关系正确并准确。

针对可变变量的复杂性，编译器可能需要进行特殊的处理和分析策略，以确保SSA形式的正确性和有效性。这可能包括引入临时变量、插入额外的phi函数、进行别名分析或通过其他手段来处理可变变量的定义和使用关系。

需要注意的是，构建SSA形式时如何处理可变变量的方法可以因编译器的实现和优化策略而有所不同。编译器设计师和开发者需要根据具体情况选择适当的方法，以在处理可变变量时维护SSA形式的优势和正确性。
```

理想状态下使用SSA的效果:  

```cpp
int G, H;
int test(_Bool Condition) {
  int X;
  if (Condition)
    X = G;
  else
    X = H;
  return X;
}
```

```cpp
@G = weak global i32 0   ; type of @G is i32*
@H = weak global i32 0   ; type of @H is i32*

define i32 @test(i1 %Condition) {
entry:
  br i1 %Condition, label %cond_true, label %cond_false

cond_true:
  %X.0 = load i32, i32* @G
  br label %cond_next

cond_false:
  %X.1 = load i32, i32* @H
  br label %cond_next

cond_next:
  %X.2 = phi i32 [ %X.1, %cond_false ], [ %X.0, %cond_true ]
  ret i32 %X.2
}
```

我们可以重写示例以使用 alloca 技术来避免使用 PHI 节点：  

```cpp
@G = weak global i32 0   ; type of @G is i32*
@H = weak global i32 0   ; type of @H is i32*

define i32 @test(i1 %Condition) {
entry:
  %X = alloca i32           ; type of %X is i32*.
  br i1 %Condition, label %cond_true, label %cond_false

cond_true:
  %X.0 = load i32, i32* @G
  store i32 %X.0, i32* %X   ; Update X
  br label %cond_next

cond_false:
  %X.1 = load i32, i32* @H
  store i32 %X.1, i32* %X   ; Update X
  br label %cond_next

cond_next:
  %X.2 = load i32, i32* %X  ; Read X
  ret i32 %X.2
}
```

也就是说不用SSA的效果就是每次都要笨拙的取数写回,而SSA相当于是暂存到寄存器里,效率高  
那为什么可变变量会导致SSA难以实现?  
原因之一可能就是情况太多太复杂,寄存器不够用  

不使用SSA的缺点也很明显,性能. 还好LLVM优化器有一个叫做`mem2reg`的pass,优化效果如下:  
```cpp
$ llvm-as < example.ll | opt -mem2reg | llvm-dis
@G = weak global i32 0
@H = weak global i32 0

define i32 @test(i1 %Condition) {
entry:
  br i1 %Condition, label %cond_true, label %cond_false

cond_true:
  %X.0 = load i32, i32* @G
  br label %cond_next

cond_false:
  %X.1 = load i32, i32* @H
  br label %cond_next

cond_next:
  %X.01 = phi i32 [ %X.1, %cond_false ], [ %X.0, %cond_true ]
  ret i32 %X.01
}
```

### 开始实现可变变量  
第一步,重构符号表`NamedValues`以支持mutation  
```cpp
static std::map<std::string, AllocaInst*> NamedValues;
```

创建一个辅助函数来在函数入口处创建分配器  
```cpp
/// CreateEntryBlockAlloca - Create an alloca instruction in the entry block of
/// the function.  This is used for mutable variables etc.
static AllocaInst *CreateEntryBlockAlloca(Function *TheFunction,
                                          const std::string &VarName) {
  IRBuilder<> TmpB(&TheFunction->getEntryBlock(),
                 TheFunction->getEntryBlock().begin());
  return TmpB.CreateAlloca(Type::getDoubleTy(*TheContext), nullptr,
                           VarName);
}
```
修改`VariableExprAST::codegen()`  

```cpp
Value *VariableExprAST::codegen() {
  // Look this variable up in the function.
  AllocaInst *A = NamedValues[Name];
  if (!A)
    return LogErrorV("Unknown variable name");

  // Load the value.
  return Builder->CreateLoad(A->getAllocatedType(), A, Name.c_str());
}
```

从符号表取出`AllocaInst`,然后创建load指令  

同理,修改`ForExprAST::codegen()`  

```cpp
Function *TheFunction = Builder->GetInsertBlock()->getParent();

// Create an alloca for the variable in the entry block.
AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, VarName);

// Emit the start code first, without 'variable' in scope.
Value *StartVal = Start->codegen();
if (!StartVal)
  return nullptr;

// Store the value into the alloca.
Builder->CreateStore(StartVal, Alloca);
...

// Compute the end condition.
Value *EndCond = End->codegen();
if (!EndCond)
  return nullptr;

// Reload, increment, and restore the alloca.  This handles the case where
// the body of the loop mutates the variable.
Value *CurVar = Builder->CreateLoad(Alloca->getAllocatedType(), Alloca,
                                    VarName.c_str());
Value *NextVar = Builder->CreateFAdd(CurVar, StepVal, "nextvar");
Builder->CreateStore(NextVar, Alloca);
...
```

添加`mem2reg`pass  

```cpp
// Promote allocas to registers.
TheFPM->add(createPromoteMemoryToRegisterPass());
// Do simple "peephole" optimizations and bit-twiddling optzns.
TheFPM->add(createInstructionCombiningPass());
// Reassociate expressions.
TheFPM->add(createReassociatePass());
...
```

以fib函数为例,查看优化前后对比.优化前:  
```cpp
define double @fib(double %x) {
entry:
  %x1 = alloca double
  store double %x, double* %x1
  %x2 = load double, double* %x1
  %cmptmp = fcmp ult double %x2, 3.000000e+00
  %booltmp = uitofp i1 %cmptmp to double
  %ifcond = fcmp one double %booltmp, 0.000000e+00
  br i1 %ifcond, label %then, label %else

then:       ; preds = %entry
  br label %ifcont

else:       ; preds = %entry
  %x3 = load double, double* %x1
  %subtmp = fsub double %x3, 1.000000e+00
  %calltmp = call double @fib(double %subtmp)
  %x4 = load double, double* %x1
  %subtmp5 = fsub double %x4, 2.000000e+00
  %calltmp6 = call double @fib(double %subtmp5)
  %addtmp = fadd double %calltmp, %calltmp6
  br label %ifcont

ifcont:     ; preds = %else, %then
  %iftmp = phi double [ 1.000000e+00, %then ], [ %addtmp, %else ]
  ret double %iftmp
}
```

优化后:  

```cpp
define double @fib(double %x) {
entry:
  %cmptmp = fcmp ult double %x, 3.000000e+00
  %booltmp = uitofp i1 %cmptmp to double
  %ifcond = fcmp one double %booltmp, 0.000000e+00
  br i1 %ifcond, label %then, label %else

then:
  br label %ifcont

else:
  %subtmp = fsub double %x, 1.000000e+00
  %calltmp = call double @fib(double %subtmp)
  %subtmp5 = fsub double %x, 2.000000e+00
  %calltmp6 = call double @fib(double %subtmp5)
  %addtmp = fadd double %calltmp, %calltmp6
  br label %ifcont

ifcont:     ; preds = %else, %then
  %iftmp = phi double [ 1.000000e+00, %then ], [ %addtmp, %else ]
  ret double %iftmp
}
```

### 添加赋值运算符  

设置优先级  
```cpp
int main() {
  // Install standard binary operators.
  // 1 is lowest precedence.
  BinopPrecedence['='] = 2;
  BinopPrecedence['<'] = 10;
  BinopPrecedence['+'] = 20;
  BinopPrecedence['-'] = 20;
```

codegen解析  
```cpp
Value *BinaryExprAST::codegen() {
  // Special case '=' because we don't want to emit the LHS as an expression.
  if (Op == '=') {
    // This assume we're building without RTTI because LLVM builds that way by
    // default. If you build LLVM with RTTI this can be changed to a
    // dynamic_cast for automatic error checking.
    VariableExprAST *LHSE = static_cast<VariableExprAST*>(LHS.get());
    if (!LHSE)
      return LogErrorV("destination of '=' must be a variable");
```

在处理其他二元运算符之前，会将其作为特殊情况进行处理。另一个奇怪的事情是它要求 LHS 是一个变量。“(x+1) = expr”是无效的 - 只允许“x = expr”之类的内容  

```cpp
  // Codegen the RHS.
  Value *Val = RHS->codegen();
  if (!Val)
    return nullptr;

  // Look up the name.
  Value *Variable = NamedValues[LHSE->getName()];
  if (!Variable)
    return LogErrorV("Unknown variable name");

  Builder->CreateStore(Val, Variable);
  return Val;
}
...
```
`CreateStore`实现赋值操作  


现在我们可以在函数内执行赋值操作了,接下来我们实现局部变量的定义  

添加枚举值,gettok解析关键字,略  

添加AST节点`VarExprAST`  
```cpp
/// VarExprAST - Expression class for var/in
class VarExprAST : public ExprAST {
  std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames;
  std::unique_ptr<ExprAST> Body;

public:
  VarExprAST(std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames,
             std::unique_ptr<ExprAST> Body)
    : VarNames(std::move(VarNames)), Body(std::move(Body)) {}

  Value *codegen() override;
};
```

修改顶层解析`ParsePrimary`,略  

添加`ParseVarExpr`,注意,我们支持一行里定义多个变量(好像是连等的情况)  
```cpp
/// varexpr ::= 'var' identifier ('=' expression)?
//                    (',' identifier ('=' expression)?)* 'in' expression
static std::unique_ptr<ExprAST> ParseVarExpr() {
  getNextToken();  // eat the var.

  std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames;

  // At least one variable name is required.
  if (CurTok != tok_identifier)
    return LogError("expected identifier after var");
  while (true) {
    std::string Name = IdentifierStr;
    getNextToken();  // eat identifier.

    // Read the optional initializer.
    std::unique_ptr<ExprAST> Init;
    if (CurTok == '=') {
      getNextToken(); // eat the '='.

      Init = ParseExpression();
      if (!Init) return nullptr;
    }

    VarNames.push_back(std::make_pair(Name, std::move(Init)));

    // End of var list, exit loop.
    if (CurTok != ',') break;
    getNextToken(); // eat the ','.

    if (CurTok != tok_identifier)
      return LogError("expected identifier list after var");
  }
```

然后解析等号右边的表达式  
```cpp
  // At this point, we have to have 'in'.
  if (CurTok != tok_in)
    return LogError("expected 'in' keyword after 'var'");
  getNextToken();  // eat 'in'.

  auto Body = ParseExpression();
  if (!Body)
    return nullptr;

  return std::make_unique<VarExprAST>(std::move(VarNames),
                                       std::move(Body));
}
```

实现codegen  
```cpp
Value *VarExprAST::codegen() {
  std::vector<AllocaInst *> OldBindings;

  Function *TheFunction = Builder->GetInsertBlock()->getParent();

  // Register all variables and emit their initializer.
  for (unsigned i = 0, e = VarNames.size(); i != e; ++i) {
    const std::string &VarName = VarNames[i].first;
    ExprAST *Init = VarNames[i].second.get();
    // Emit the initializer before adding the variable to scope, this prevents
    // the initializer from referencing the variable itself, and permits stuff
    // like this:
    //  var a = 1 in
    //    var a = a in ...   # refers to outer 'a'.
    Value *InitVal;
    if (Init) {
      InitVal = Init->codegen();
      if (!InitVal)
        return nullptr;
    } else { // If not specified, use 0.0.
      InitVal = ConstantFP::get(*TheContext, APFloat(0.0));
    }

    AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, VarName);
    Builder->CreateStore(InitVal, Alloca);

    // Remember the old variable binding so that we can restore the binding when
    // we unrecurse.
    OldBindings.push_back(NamedValues[VarName]);

    // Remember this binding.
    NamedValues[VarName] = Alloca;
  }
```

如果赋初值了，就解析赋值的表达式，否则就将值设为立即数0，然后创建stroe指令  
记得保存符号表中被替换的变量名和值  

```cpp
// Codegen the body, now that all vars are in scope.
Value *BodyVal = Body->codegen();
if (!BodyVal)
  return nullptr;
  // Pop all our variables from scope.
  for (unsigned i = 0, e = VarNames.size(); i != e; ++i)
    NamedValues[VarNames[i].first] = OldBindings[i];

  // Return the body computation.
  return BodyVal;
}
```

执行body,最后return之前恢复被覆盖的变量  

## chapter8 编译为目标代码

我们需要指定`architectures`-架构,和`features`-功能  
架构要使用一个三元组,格式如下: 
- <arch><sub>-<vendor>-<sys>-<abi>  
使用clang可以查看自己当前的三元组  
```bash
$ clang --version | grep Target
Target: x86_64-unknown-linux-gnu
```
接下来我们看看在代码中怎么使用  
首先我们不考虑交叉编译的情况,直接本机编译,也就是使用默认的目标三元组  
```cpp
auto TargetTriple = sys::getDefaultTargetTriple();
```
接下来,进行一系列初始化  
```cpp
InitializeAllTargetInfos();
InitializeAllTargets();
InitializeAllTargetMCs();
InitializeAllAsmParsers();
InitializeAllAsmPrinters();
```
实例化target类  
```cpp
std::string Error;
auto Target = TargetRegistry::lookupTarget(TargetTriple, Error);

// Print an error and exit if we couldn't find the requested target.
// This generally occurs if we've forgotten to initialise the
// TargetRegistry or we have a bogus target triple.
if (!Target) {
  errs() << Error;
  return 1;
}
```

接下来,我们还需要实例化`TargetMachine`类,这需要我们指定`CPU`和`feature`,我们可以使用llc来查看  

```bash
$ llvm-as < /dev/null | llc -march=x86 -mattr=help
Available CPUs for this target:

  amdfam10      - Select the amdfam10 processor.
  athlon        - Select the athlon processor.
  athlon-4      - Select the athlon-4 processor.
  ...

Available features for this target:

  16bit-mode            - 16-bit mode (i8086).
  32bit-mode            - 32-bit mode (80386).
  3dnow                 - Enable 3DNow! instructions.
  3dnowa                - Enable 3DNow! Athlon instructions.
  ...
```

然后实例化`TargetMachine`类  
```cpp
auto CPU = "generic";
auto Features = "";

TargetOptions opt;
auto RM = std::optional<Reloc::Model>();
auto TargetMachine = Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);

```

接下来,可以配置module,指定Target和DataLayout,注意:这并不是绝对必要的，但前端性能指南建议这样做。了解目标和数据布局有助于优化。  
```cpp
TheModule->setDataLayout(TargetMachine->createDataLayout());
TheModule->setTargetTriple(TargetTriple);
```

现在就可以生成目标代码了  
```cpp
auto Filename = "output.o";
std::error_code EC;
raw_fd_ostream dest(Filename, EC, sys::fs::OF_None);

if (EC) {
  errs() << "Could not open file: " << EC.message();
  return 1;
}
```

最后,定义一个pass,然后运行该pass  
```cpp
legacy::PassManager pass;
auto FileType = CodeGenFileType::ObjectFile;

if (TargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
  errs() << "TargetMachine can't emit a file of this type";
  return 1;
}

pass.run(*TheModule);
dest.flush();
```

运行和测试:  
编译代码:  
```bash
$ clang++ -g -O3 toy.cpp `llvm-config --cxxflags --ldflags --system-libs --libs all` -o toy
```
运行并定义一个简单的函数,完成后按 Ctrl-D  

```cpp
$ ./toy
ready> def average(x y) (x + y) * 0.5;
^D
Wrote output.o
```

写一个测试文件将目标文件链接起来进行测试:  
```cpp
#include <iostream>

extern "C" {
    double average(double, double);
}

int main() {
    std::cout << "average of 3.0 and 4.0: " << average(3.0, 4.0) << std::endl;
}
```

链接并运行:  
```bash
$ clang++ main.cpp output.o -o main
$ ./main
average of 3.0 and 4.0: 3.5
```


## chapter9 添加调试信息  
为什么这是一个难题?  
> 由于几个不同的原因，调试信息是一个难题 - 主要集中在优化代码上。首先，优化使得保持源位置变得更加困难。在 LLVM IR 中，我们在指令上保留每个 IR 级指令的原始源位置。优化过程应保留新创建指令的源位置，但合并的指令只能保留单个位置 - 这可能会导致在单步执行优化程序时跳转。其次，优化可以通过优化、与其他变量共享内存或难以跟踪的方式移动变量。出于本教程的目的，我们将避免优化（正如您将在接下来的一组补丁中看到的那样）。  

首先,我们无法通过JIT模式调试,所以需要把源代码稍作更改,变为m提前编译模式,修改的过程省略,我们聚焦于添加调试信息  
`DWARF`是什么?  
> DWARF 是许多编译器和调试器用来支持源代码级调试的调试信息文件格式。它满足许多过程语言（例如 C、C++ 和 Fortran）的要求，并且设计为可扩展到其他语言。DWARF 是独立于体系结构的，适用于任何处理器或操作系统。它广泛应用于Unix、Linux等操作系统以及单机环境中。

接下来需要创建一个小容器储存常用数据,为每种数据类型提供支持  
```cpp
static std::unique_ptr<DIBuilder> DBuilder;

struct DebugInfo {
  DICompileUnit *TheCU;
  DIType *DblTy;

  DIType *getDoubleTy();
} KSDbgInfo;

DIType *DebugInfo::getDoubleTy() {
  if (DblTy)
    return DblTy;

  DblTy = DBuilder->createBasicType("double", 64, dwarf::DW_ATE_float);
  return DblTy;
}
```

在main函数中,实例化DIBuilder类  

```cpp
DBuilder = std::make_unique<DIBuilder>(*TheModule);

KSDbgInfo.TheCU = DBuilder->createCompileUnit(
    dwarf::DW_LANG_C, DBuilder->createFile("fib.ks", "."),
    "Kaleidoscope Compiler", false, "", 0);
```

创建调试文件（或调试上下文）  
```cpp
DIFile *Unit = DBuilder->createFile(KSDbgInfo.TheCU->getFilename(),
                                    KSDbgInfo.TheCU->getDirectory());
```

构建我们的函数定义：Compile Unit  
```cpp
DIScope *FContext = Unit;
unsigned LineNo = 0;
unsigned ScopeLine = 0;
DISubprogram *SP = DBuilder->createFunction(
    FContext, P.getName(), StringRef(), Unit, LineNo,
    CreateFunctionType(TheFunction->arg_size()),
    ScopeLine,
    DINode::FlagPrototyped,
    DISubprogram::SPFlagDefinition);
TheFunction->setSubprogram(SP);
```

现在我们有了一个 DISubprogram，其中包含对该函数的所有元数据的引用。  

对于调试信息来说，最重要的是准确的源位置,接下来我们需要添加原位置信息  
```cpp
struct SourceLocation {
  int Line;
  int Col;
};
static SourceLocation CurLoc;
static SourceLocation LexLoc = {1, 0};

static int advance() {
  int LastChar = getchar();

  if (LastChar == '\n' || LastChar == '\r') {
    LexLoc.Line++;
    LexLoc.Col = 0;
  } else
    LexLoc.Col++;
  return LastChar;
}
```

添加到AST类中  
```cpp
class ExprAST {
  SourceLocation Loc;

  public:
    ExprAST(SourceLocation Loc = CurLoc) : Loc(Loc) {}
    virtual ~ExprAST() {}
    virtual Value* codegen() = 0;
    int getLine() const { return Loc.Line; }
    int getCol() const { return Loc.Col; }
    virtual raw_ostream &dump(raw_ostream &out, int ind) {
      return out << ':' << getLine() << ':' << getCol() << '\n';
    }
```

当我们创建一个新表达式时我们会向下传递：  
```cpp
LHS = std::make_unique<BinaryExprAST>(BinLoc, BinOp, std::move(LHS),
                                       std::move(RHS));
```
为了确保每条指令都获得正确的源位置信息，我们必须告知Builder何时到达新的源位置。我们为此使用一个小辅助函数：  
```cpp
void DebugInfo::emitLocation(ExprAST *AST) {
  if (!AST)
    return Builder->SetCurrentDebugLocation(DebugLoc());
  DIScope *Scope;
  if (LexicalBlocks.empty())
    Scope = TheCU;
  else
    Scope = LexicalBlocks.back();
  Builder->SetCurrentDebugLocation(
      DILocation::get(Scope->getContext(), AST->getLine(), AST->getCol(), Scope));
}
```
创建一个函数调用栈:  
```cpp
std::vector<DIScope *> LexicalBlocks;

KSDbgInfo.LexicalBlocks.push_back(SP);

// Pop off the lexical block for the function since we added it
// unconditionally.
KSDbgInfo.LexicalBlocks.pop_back();
```

在`FunctionAST::codegen`中添加代码,以便打印变量  
```cpp
// Record the function arguments in the NamedValues map.
NamedValues.clear();
unsigned ArgIdx = 0;
for (auto &Arg : TheFunction->args()) {
  // Create an alloca for this variable.
  AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, Arg.getName());

  // Create a debug descriptor for the variable.
  DILocalVariable *D = DBuilder->createParameterVariable(
      SP, Arg.getName(), ++ArgIdx, Unit, LineNo, KSDbgInfo.getDoubleTy(),
      true);

  DBuilder->insertDeclare(Alloca, D, DBuilder->createExpression(),
                          DILocation::get(SP->getContext(), LineNo, 0, SP),
                          Builder->GetInsertBlock());

  // Store the initial value into the alloca.
  Builder->CreateStore(&Arg, Alloca);

  // Add arguments to variable symbol table.
  NamedValues[std::string(Arg.getName())] = Alloca;
}
```

## chapter10 未实现的功能 可以拓展完善的功能  
- 全局变量
- 类型变量
- 数组 结构体 向量等
- 标准运行时
- 内存管理
- 异常处理
- 面向对象 泛型 复数 几何编程...