#include"calculator.hpp"
#include <iostream>
#include <cctype>
#include <cstdio>
#include <map>
#include <string>
#include <vector>
namespace
{
enum Token_Type
{
    EOF_TOKEN = 0,
    IDENTIFIER_TOKEN,
    NUMERIC_TOKEN
};

std::string line;
int idx = 0;
std::string Identifier_string;
int Numeric_Val;

char getChar()
{
    if (idx >= line.size())
        return EOF;
    return line[idx++];
}

int get_token()
{
    int LastChar = ' ';

    while (isspace(LastChar))
        LastChar = getChar();

    // 如果是字母开头
    if (isalpha(LastChar))
        return EOF_TOKEN;

    // 如果是数字开头
    if (isdigit(LastChar))
    {
        std::string NumStr;
        do
        {
            NumStr += LastChar;
            LastChar = getChar();
        } while (isdigit(LastChar));

        Numeric_Val = strtod(NumStr.c_str(), 0);
        return NUMERIC_TOKEN;
    }

    if (LastChar == EOF)
        return EOF_TOKEN;

    int ThisChar = LastChar;
    LastChar = getChar();
    return ThisChar;
}

struct ASTNode
{
    int m_val;

public:
    ASTNode(int val) : m_val(val) {}
};

int cal(char op, ASTNode *lhs, ASTNode *rhs)
{
    switch (op)
    {
    case '+':
        return lhs->m_val + rhs->m_val;
        break;
    case '-':
        return lhs->m_val - rhs->m_val;
        break;
    case '*':
        return lhs->m_val * rhs->m_val;
        break;
    case '/':
        return lhs->m_val / rhs->m_val;
        break;
    case '%':
        return lhs->m_val % rhs->m_val;
        break;

    default:
        return 0;
        break;
    }
}

int Current_token;
int next_token() { return Current_token = get_token(); }

std::map<char, int> Operator_Precedence;
// 获取op优先级
int getBinOpPrecedence()
{
    if (!isascii(Current_token))
        return -1;

    int TokPrec = Operator_Precedence[Current_token];
    if (TokPrec <= 0)
        return -1;
    return TokPrec;
}

ASTNode *expression_parser();

ASTNode *numeric_parser()
{
    ASTNode *Result = new ASTNode(Numeric_Val);
    next_token();
    return Result;
}

// 括号解析
ASTNode *paran_parser()
{
    next_token();
    ASTNode *V = expression_parser();
    if (!V)
        return 0;

    if (Current_token != ')')
        return 0;
    next_token();
    return V;
}

ASTNode *Base_Parser()
{
    switch (Current_token)
    {
    default:
        return 0;
    case NUMERIC_TOKEN:
        return numeric_parser();
    case '(':
        return paran_parser();
    }
}

ASTNode *binary_op_parser(int Old_Prec, ASTNode *LHS)
{
    while (1)
    {
        // Precedence：优先级
        int Operator_Prec = getBinOpPrecedence();

        // 如果当前op优先级低于前面的op，停止递归，前面的表达式构成一个单独的子树
        // 停止递归是为了让上一层继续解析处理,这一小段已经是一个独立的子树了
        // 比如我这一层是小括号里的内容，那么即使后面还有内容，也不用归我管了
        if (Operator_Prec < Old_Prec)
            return LHS;

        int BinOp = Current_token;
        next_token();

        ASTNode *RHS = Base_Parser();
        if (!RHS)
            return 0;

        // 如果当前op优先级低于后面的op，继续递归
        int Next_Prec = getBinOpPrecedence();
        if (Operator_Prec < Next_Prec)
        {
            // 拿到右子树
            RHS = binary_op_parser(Operator_Prec + 1, RHS);
            if (RHS == 0)
                return 0;
        }
        // 合并左右子树，继续往后解析
        //  LHS = new BinaryAST(std::to_string(BinOp), LHS, RHS);
        LHS = new ASTNode(cal(BinOp, LHS, RHS));
    }
}

ASTNode *expression_parser()
{
    // 递归解析二元表达式
    ASTNode *LHS = Base_Parser();
    if (!LHS)
        return 0;
    return binary_op_parser(0, LHS);
}

// op优先级
void init_precedence()
{
    Operator_Precedence['-'] = 1;
    Operator_Precedence['+'] = 2;
    Operator_Precedence['/'] = 3;
    Operator_Precedence['*'] = 4;
    Operator_Precedence['%'] = 5;
}
}

int calculate(std::string str)
{
    init_precedence();
    line=str;
    idx=0;
    next_token();
    auto node = expression_parser();
    if(node)
    {
        return node->m_val;
    }
    throw std::invalid_argument("表达式格式错误！");
}



// void mainLoop()
// {
//     while (1)
//     {
//         std::getline(std::cin,line);
//         if(line=="q") return;
//         idx = 0;
//         next_token();
//         auto node = expression_parser();
//         if(node)
//         {
//             int ans = node->m_val;
//         std::cout << ans << std::endl;
//         }
//         else
//         {
//             std::cout << "syntax error!" << std::endl;
//         }
//     }
// }

// int main(int argc, char *argv[])
// {
//     init_precedence();
//     mainLoop();
//     return 0;
// }
