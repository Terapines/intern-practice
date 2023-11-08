#pragma once
#include <iostream>
#include <cctype>
#include <cstdio>
#include <map>
#include <string>
#include <vector>
namespace
{

char getChar();

int get_token();

struct ASTNode;

int cal(char op, ASTNode *lhs, ASTNode *rhs);

int next_token();

ASTNode *expression_parser();

ASTNode *numeric_parser();

// 括号解析
ASTNode *paran_parser();

ASTNode *Base_Parser();

ASTNode *binary_op_parser(int Old_Prec, ASTNode *LHS);

ASTNode *expression_parser();

// op优先级
void init_precedence();
}

int calculate(std::string str);
