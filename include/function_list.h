#pragma once

#include "token.h"
#include "vector"
#include "analyzer.h"
#include "instruction.h"

class FunctionList{
  public:
    int getVarParams(Token t, bool *flag_const);
    int getVarLocal(Token t, bool *flag_const);
    int getVarGloba(Token t, bool *flag_const);
  
  public:
    bool void_return; // 是否是void return
    std::string func_name; // 函数名
    std::vector<Token> _vars; // 局部变量
    std::vector<Token> _params; // 参数
    std::vector<Instruction> _instrucs; // 函数的指令集合
};