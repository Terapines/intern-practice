#include "AST/AST.h"
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

using namespace llvm;

//===----------------------------------------------------------------------===//
// Abstract Syntax Tree (aka Parse Tree)
//===----------------------------------------------------------------------===//
namespace {

raw_ostream &indent(raw_ostream &O, int size) {
  return O << std::string(size, ' ');
}

/// ExprAST - Base class for all expression nodes.

  ExprAST::ExprAST(SourceLocation Loc = CurLoc) : Loc(Loc) {}
  ExprAST::~ExprAST() {}
  int ExprAST::getLine() const { return Loc.Line; }
  int ExprAST::getCol() const { return Loc.Col; }
  raw_ostream &ExprAST::dump(raw_ostream &out, int ind) {
    return out << ':' << getLine() << ':' << getCol() << '\n';
  }


/// NumberExprAST - Expression class for numeric literals like "1.0".

  NumberExprAST::NumberExprAST(double Val) : Val(Val) {}
  raw_ostream &NumberExprAST::dump(raw_ostream &out, int ind) {
    return ExprAST::dump(out << Val, ind);
  }

/// VariableExprAST - Expression class for referencing a variable, like "a".
  VariableExprAST::VariableExprAST(SourceLocation Loc, const std::string &Name)
      : ExprAST(Loc), Name(Name) {}
  const std::string &VariableExprAST::getName() const { return Name; }
  raw_ostream &VariableExprAST::dump(raw_ostream &out, int ind)   {
    return ExprAST::dump(out << Name, ind);
  }

/// UnaryExprAST - Expression class for a unary operator.

UnaryExprAST::UnaryExprAST(char Opcode, std::unique_ptr<ExprAST> Operand)
    : Opcode(Opcode), Operand(std::move(Operand)) {}
raw_ostream &UnaryExprAST::dump(raw_ostream &out, int ind) {
  ExprAST::dump(out << "unary" << Opcode, ind);
  Operand->dump(out, ind + 1);
  return out;
}

/// BinaryExprAST - Expression class for a binary operator.

BinaryExprAST::BinaryExprAST(SourceLocation Loc, char Op, std::unique_ptr<ExprAST> LHS,
              std::unique_ptr<ExprAST> RHS)
    : ExprAST(Loc), Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
raw_ostream &BinaryExprAST::dump(raw_ostream &out, int ind) {
  ExprAST::dump(out << "binary" << Op, ind);
  LHS->dump(indent(out, ind) << "LHS:", ind + 1);
  RHS->dump(indent(out, ind) << "RHS:", ind + 1);
  return out;
}


/// CallExprAST - Expression class for function calls.

  CallExprAST::CallExprAST(SourceLocation Loc, const std::string &Callee,
              std::vector<std::unique_ptr<ExprAST>> Args)
      : ExprAST(Loc), Callee(Callee), Args(std::move(Args)) {}
  
  raw_ostream &CallExprAST::dump(raw_ostream &out, int ind) {
    ExprAST::dump(out << "call " << Callee, ind);
    for (const auto &Arg : Args)
      Arg->dump(indent(out, ind + 1), ind + 1);
    return out;
  }


/// IfExprAST - Expression class for if/then/else.

  IfExprAST::IfExprAST(SourceLocation Loc, std::unique_ptr<ExprAST> Cond,
            std::unique_ptr<ExprAST> Then, std::unique_ptr<ExprAST> Else)
      : ExprAST(Loc), Cond(std::move(Cond)), Then(std::move(Then)),
        Else(std::move(Else)) {}
  
  raw_ostream &IfExprAST::dump(raw_ostream &out, int ind) {
    ExprAST::dump(out << "if", ind);
    Cond->dump(indent(out, ind) << "Cond:", ind + 1);
    Then->dump(indent(out, ind) << "Then:", ind + 1);
    Else->dump(indent(out, ind) << "Else:", ind + 1);
    return out;
  }


/// ForExprAST - Expression class for for/in.

  ForExprAST::ForExprAST(const std::string &VarName, std::unique_ptr<ExprAST> Start,
             std::unique_ptr<ExprAST> End, std::unique_ptr<ExprAST> Step,
             std::unique_ptr<ExprAST> Body)
      : VarName(VarName), Start(std::move(Start)), End(std::move(End)),
        Step(std::move(Step)), Body(std::move(Body)) {}
  
  raw_ostream &ForExprAST::dump(raw_ostream &out, int ind) {
    ExprAST::dump(out << "for", ind);
    Start->dump(indent(out, ind) << "Cond:", ind + 1);
    End->dump(indent(out, ind) << "End:", ind + 1);
    Step->dump(indent(out, ind) << "Step:", ind + 1);
    Body->dump(indent(out, ind) << "Body:", ind + 1);
    return out;
  }


/// VarExprAST - Expression class for var/in
  VarExprAST::VarExprAST(
      std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames,
      std::unique_ptr<ExprAST> Body)
      : VarNames(std::move(VarNames)), Body(std::move(Body)) {}
  
  raw_ostream &VarExprAST::dump(raw_ostream &out, int ind) {
    ExprAST::dump(out << "var", ind);
    for (const auto &NamedVar : VarNames)
      NamedVar.second->dump(indent(out, ind) << NamedVar.first << ':', ind + 1);
    Body->dump(indent(out, ind) << "Body:", ind + 1);
    return out;
  }

/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes), as well as if it is an operator.

  PrototypeAST::PrototypeAST(SourceLocation Loc, const std::string &Name,
               std::vector<std::string> Args, bool IsOperator ,
               unsigned Prec)
      : Name(Name), Args(std::move(Args)), IsOperator(IsOperator),
        Precedence(Prec), Line(Loc.Line) {}
  const std::string &PrototypeAST::getName() const { return Name; }

  bool PrototypeAST::isUnaryOp() const { return IsOperator && Args.size() == 1; }
  bool PrototypeAST::isBinaryOp() const { return IsOperator && Args.size() == 2; }

  char PrototypeAST::getOperatorName() const {
    assert(isUnaryOp() || isBinaryOp());
    return Name[Name.size() - 1];
  }

  unsigned PrototypeAST::getBinaryPrecedence() const { return Precedence; }
  int PrototypeAST::getLine() const { return Line; }


/// FunctionAST - This class represents a function definition itself.

  FunctionAST::FunctionAST(std::unique_ptr<PrototypeAST> Proto,
              std::unique_ptr<ExprAST> Body)
      : Proto(std::move(Proto)), Body(std::move(Body)) {}
  Function *codegen();
  raw_ostream &FunctionAST::dump(raw_ostream &out, int ind) {
    indent(out, ind) << "FunctionAST\n";
    ++ind;
    indent(out, ind) << "Body:";
    return Body ? Body->dump(out, ind) : out << "null\n";
  }
} // end anonymous namespace