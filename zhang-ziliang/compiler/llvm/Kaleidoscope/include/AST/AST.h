#include "../lexer/lexer.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/Transforms/Scalar.h"
#include <cctype>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

#ifndef _AST_H
#define _AST_H
using namespace llvm;

//===----------------------------------------------------------------------===//
// Abstract Syntax Tree (aka Parse Tree)
//===----------------------------------------------------------------------===//
// namespace {

raw_ostream &indent(raw_ostream &O, int size) ;

/// ExprAST - Base class for all expression nodes.
class ExprAST {
  SourceLocation Loc;

public:
  ExprAST(SourceLocation Loc = CurLoc);
  virtual ~ExprAST();
  virtual Value *codegen() = 0;
  int getLine() const ;
  int getCol() const ;
  virtual raw_ostream &dump(raw_ostream &out, int ind) ;
};

/// NumberExprAST - Expression class for numeric literals like "1.0".
class NumberExprAST : public ExprAST {
  double Val;

public:
  NumberExprAST(double Val) ;
  raw_ostream &dump(raw_ostream &out, int ind) override ;
  Value *codegen() override;
};

/// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAST : public ExprAST {
  std::string Name;

public:
  VariableExprAST(SourceLocation Loc, const std::string &Name);
  const std::string &getName() const ;
  Value *codegen() override;
  raw_ostream &dump(raw_ostream &out, int ind) override ;
};

/// UnaryExprAST - Expression class for a unary operator.
class UnaryExprAST : public ExprAST {
  char Opcode;
  std::unique_ptr<ExprAST> Operand;

public:
  UnaryExprAST(char Opcode, std::unique_ptr<ExprAST> Operand);
  Value *codegen() override;
  raw_ostream &dump(raw_ostream &out, int ind) override ;
};

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST {
  char Op;
  std::unique_ptr<ExprAST> LHS, RHS;

public:
  BinaryExprAST(SourceLocation Loc, char Op, std::unique_ptr<ExprAST> LHS,
                std::unique_ptr<ExprAST> RHS);
  Value *codegen() override;
  raw_ostream &dump(raw_ostream &out, int ind) override ;
};

/// CallExprAST - Expression class for function calls.
class CallExprAST : public ExprAST {
  std::string Callee;
  std::vector<std::unique_ptr<ExprAST>> Args;

public:
  CallExprAST(SourceLocation Loc, const std::string &Callee,
              std::vector<std::unique_ptr<ExprAST>> Args);
  Value *codegen() override;
  raw_ostream &dump(raw_ostream &out, int ind) override ;
};

/// IfExprAST - Expression class for if/then/else.
class IfExprAST : public ExprAST {
  std::unique_ptr<ExprAST> Cond, Then, Else;

public:
  IfExprAST(SourceLocation Loc, std::unique_ptr<ExprAST> Cond,
            std::unique_ptr<ExprAST> Then, std::unique_ptr<ExprAST> Else);
  Value *codegen() override;
  raw_ostream &dump(raw_ostream &out, int ind) override ;
};

/// ForExprAST - Expression class for for/in.
class ForExprAST : public ExprAST {
  std::string VarName;
  std::unique_ptr<ExprAST> Start, End, Step, Body;

public:
  ForExprAST(const std::string &VarName, std::unique_ptr<ExprAST> Start,
             std::unique_ptr<ExprAST> End, std::unique_ptr<ExprAST> Step,
             std::unique_ptr<ExprAST> Body);
  Value *codegen() override;
  raw_ostream &dump(raw_ostream &out, int ind) override ;
};

/// VarExprAST - Expression class for var/in
class VarExprAST : public ExprAST {
  std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames;
  std::unique_ptr<ExprAST> Body;

public:
  VarExprAST(
      std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames,
      std::unique_ptr<ExprAST> Body);
  Value *codegen() override;
  raw_ostream &dump(raw_ostream &out, int ind) override ;
};

/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes), as well as if it is an operator.
class PrototypeAST {
  std::string Name;
  std::vector<std::string> Args;
  bool IsOperator;
  unsigned Precedence; // Precedence if a binary op.
  int Line;

public:
  PrototypeAST(SourceLocation Loc, const std::string &Name,
               std::vector<std::string> Args, bool IsOperator = false,
               unsigned Prec = 0);
  Function *codegen();
  const std::string &getName() const ;

  bool isUnaryOp() const ;
  bool isBinaryOp() const ;

  char getOperatorName() const ;

  unsigned getBinaryPrecedence() const ;
  int getLine() const ;
};

/// FunctionAST - This class represents a function definition itself.
class FunctionAST {
  std::unique_ptr<PrototypeAST> Proto;
  std::unique_ptr<ExprAST> Body;

public:
  FunctionAST(std::unique_ptr<PrototypeAST> Proto,
              std::unique_ptr<ExprAST> Body);
  Function *codegen();
  raw_ostream &dump(raw_ostream &out, int ind) ;
};
// } // end anonymous namespace

#endif
