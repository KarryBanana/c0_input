#pragma once

#include <cstddef>  // for std::size_t
#include <cstdint>
#include <map>
#include <optional>
#include <utility>
#include <vector>

#include "error.h"
#include "token.h"
#include "action_scope.h"
#include "Stack.h"

class Analyzer {
  public:
    Analyzer(std::vector<Token> tk)
    : _tokens(tk), _offset(0){}
  public:
    std::pair<bool, std::optional<CompilationError>> Analyze(std::string output);

  private:
    // <程序>
    std::optional<CompilationError> Program(std::string output);
    std::optional<CompilationError> DeclareStatement(int *cnt);
    std::optional<CompilationError> Function();
    // 函数参数列表
    std::optional<CompilationError> FunctionParamList();
    // 函数参数
    std::optional<CompilationError> FunctionParam();

    // 表达式
    std::optional<CompilationError> Statement(int *cnt);
    // 变量声明
    std::optional<CompilationError> LetDeclareStatement(int *cnt);
    // 常量声明
    std::optional<CompilationError> ConstDeclareStatement(int *cnt);
    // 代码块声明
    std::optional<CompilationError> BlockStatement(int *cnt);
    // 表达式声明
    std::optional<CompilationError> ExprStatement(int *cnt);
    // if
    std::optional<CompilationError> IfStatement(int *cnt);
    //while
    std::optional<CompilationError> WhileStatement(int *cnt);
    // return 
    std::optional<CompilationError> ReturnStatement(int *cnt);
    // 空代码块
    std::optional<CompilationError> EmptyStatement();

    // 语句
    std::optional<CompilationError> Expression(int *cnt);
    // 优先级最低的语句
    std::optional<CompilationError> EqualExpr(int *cnt);
    // 优先级第二低的语句
    std::optional<CompilationError> GreatExpr(int *cnt);
    // 优先级第三低的语句
    std::optional<CompilationError> PlusExpr(int *cnt);
    // 优先级第四低的语句
    std::optional<CompilationError> MultiExpr(int *cnt);
    // 优先级第四高的语句
    std::optional<CompilationError> AsExpr(int *cnt);
    // 优先级第三高的语句 前置语句
    std::optional<CompilationError> BeforeExpr(int *cnt);
    // 优先级第最高的语句 变量语句
    std::optional<CompilationError> IdentExpr(int *cnt);
    // 优先级第最高的语句 调用语句
    std::optional<CompilationError> CallExpr(int *cnt);
    // 优先级最高的语句 括号表达式
    std::optional<CompilationError> BracketExpr(int *cnt);
    // 标准输入输出
    std::optional<CompilationError> StdIO(Token t,int *cnt);

    // 取下一个token
    std::optional<Token> nextToken();
    // 回退一个token
    void unreadToken();
    // 判断作用域内变量是否重名
    bool isVarDuplic(Token t);
    // 判断函数名是否重复
    bool isFuncDuplic(Token t);
    char* reverseData(unsigned char *num, int len);

  private:
    std::vector<Token> _tokens; //存放词法分析完的tokens
    std::size_t _offset; // 指向的token
};