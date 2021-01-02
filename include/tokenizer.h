#pragma once

#include <string>
#include <vector>
#include <optional>
#include <cctype>
#include "token.h"
#include "error.h"

class Tokenizer {

  // 状态机的所有状态
  enum DFAState {
    INITIAL_STATE,
    UNSIGNED_INTEGER_STATE,
    PLUS_SIGN_STATE,
    MINUS_SIGN_STATE,
    DIVISION_SIGN_STATE,
    MULTIPLICATION_SIGN_STATE,
    IDENTIFIER_STATE,
    ASSIGN_SIGN_STATE, // =
    EQUAL_SIGN_STATE, // ==
    HALF_NEQ_STATE, // 只读入了一个!的状态
    NEQ_SIGN_STATE, // !=
    STRING_STATE, // 字符串常量
    LT_STATE, // <
    GT_STATE, // >
    LE_STATE, // <=
    GE_STATE, // >=
    LEFTBRACKET_STATE,
    RIGHTBRACKET_STATE,
    L_BRACE_STATE, // {
    R_BRACE_STATE, // }
    ARROW_STATE, // ->
    COLON_STATE,
    COMA_STATE,
    SEMICOLON_STATE,
  };
  public:
  //构造方法
  Tokenizer(std::string input)
      : line_buffer(), _ptr(0) {}

  public:
    // 得到所有的tokens
    std::pair<std::vector<Token>, std::optional<CompilationError>> AllTokens(std::string input);
    

  private:
    void readLine(std::ifstream*  file);
    // nextToken()
    // nextPos()
    // currentPos()
    //previousPos()
    bool isEOF();
    void unreadLast();
    void nextPos();
    void previousPos();
    std::optional<char> nextChar();
    std::pair<std::optional<Token>, std::optional<CompilationError> > nextToken();
  
  private:
    // 指向下一个要读取的字符
    uint64_t _ptr;
    bool read_first_char;
    std::string line_buffer;
};