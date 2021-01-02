#pragma once

#include <any>
#include <string>
#include <iostream>

enum TokenType {
  NULL_TOKEN,
  UNSIGNED_INTEGER,
  STRING, // 字符串
  IDENTIFIER,
  STDIO, // 标准输入输出
  FN,
  LET,
  AS,
  WHILE,
  IF,
  ELSE,
  RETURN,
  TYPE,
  BREAK,
  CONTINUE,
  CONST,
  PLUS_SIGN,
  MINUS_SIGN,
  MULTIPLICATION_SIGN,
  DIVISION_SIGN,
  EQUAL_SIGN, // ==
  ASSIGN_SIGN, // =
  NEQ_SIGN, // !=
  LT, // <
  GT, // >
  LE, // <=
  GE, // >=
  LEFT_BRACKET, // (
  RIGHT_BRACKET, // )
  L_BRACE, // {
  R_BRACE, // }
  ARROW, // ->
  COLON, // :
  COMA, // ,
  SEMICOLON, // ;
};

class Token{
public:
  Token(TokenType type, std::any value):
    _type(type),
    _value(value),
    is_const(false), // 默认不是常量
    is_initial(false),
    is_int(false),
    is_double(false),
    off(0) {} // 默认没初始化 
public:
    TokenType GetType() const { return _type; };
    std::string GetValueString() const 
    {
      try {
        return std::any_cast<std::string>(_value);
      } catch (const std::bad_any_cast & ) {
          }
      try {
        return std::string(1, std::any_cast<char>(_value));
      }   catch (const std::bad_any_cast &) {
          }
      try {
        return std::to_string(std::any_cast<int32_t>(_value));
      } catch (const std::bad_any_cast &) {
          }
      return "Invalid Value";
    }
  private:
      TokenType _type;
      std::any _value;
  public:
    bool is_const;
    bool is_initial;
    bool is_int;
    bool is_double;
    int off;
};