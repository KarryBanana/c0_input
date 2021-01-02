#include "../include/tokenizer.h"
#include <cstdio>
#include <cctype>
#include <fstream>
#include <sstream>


bool detect_comment = false;
void Tokenizer::readLine(std::ifstream *file)
{
  std::getline(*file, line_buffer);
  // 测试文件按行读入
  std::cout <<line_buffer << "\n";
  return;
}

std::pair<std::vector<Token>, std::optional<CompilationError>>
Tokenizer::AllTokens(std::string input)
{
  std::vector<Token> result;
  // 测试逐行读入
  std::ifstream f(input);
  if (!f.is_open())
  {
    std::cout << "Error opening file";
    exit(1);
  }
  int cnt = 0;
  while (f.peek() != EOF)
  {
    readLine(&f);
  }
  printf("token finish!\n");
  return std::make_pair(result, std::optional<CompilationError>());
}

std::pair<std::optional<Token>, std::optional<CompilationError>>
Tokenizer::nextToken()
{
  // 用于存储已经读到的组成当前token字符
  std::stringstream ss;
  // 分析token的结果，作为此函数的返回值
  std::pair<std::optional<Token>, std::optional<CompilationError>> result;
  // <行号，列号>，表示当前token的第一个字符在源代码中的位置
  // std::pair<int64_t, int64_t> pos;
  // 记录当前自动机的状态，进入此函数时是初始状态
  DFAState current_state = DFAState::INITIAL_STATE;
  // 这是一个死循环，除非主动跳出
  // 每一次执行while内的代码，都可能导致状态的变更
  while (true)
  {
    // 读一个字符，请注意auto推导得出的类型是std::optional<char>
    // 这里其实有两种写法
    // 1. 每次循环前立即读入一个 char
    // 2. 只有在可能会转移的状态读入一个 char
    // 因为我们实现了 unread，为了省事我们选择第一种
    auto current_char = nextChar();
    //printf("case 2\n");
    // 针对当前的状态进行不同的操作
    switch (current_state)
    {
    case INITIAL_STATE:
    {
      // 已经读到了文件尾
      if (!current_char.has_value()){
        printf("line eol\n");
        return std::make_pair(
            std::optional<Token>(),
            std::make_optional<CompilationError>(ErrorCode::ErrEOF));
      }
      // 获取读到的字符的值，注意auto推导出的类型是char
      // 注意文件末尾有一个\0  !!!debug here 1 day
      auto ch = current_char.value();
      // 标记是否读到了不合法的字符，初始化为否
      bool invalid = false;

      if (isspace(ch) || ch == '\0'){                         // 读到的字符是空白字符（空格、换行、制表符等）
        current_state = DFAState::INITIAL_STATE;
      } // 保留当前状态为初始状态，此处直接break也是可以的
      else if (!isprint(ch)){ 
        printf("is not print!\n");                    // control codes and backspace
        invalid = true;
      }
      else if (isdigit(ch)) // 读到的字符是数字
        current_state =
            DFAState::UNSIGNED_INTEGER_STATE;       // 切换到无符号整数的状态
      else if (isalpha(ch) || ch == '_')            // 读到的字符是英文字母或是下划线
        current_state = DFAState::IDENTIFIER_STATE; // 切换到标识符的状态
      else
      {
        switch (ch)
        {
        case '=': // 如果读到的字符是`=`，则切换到等于号的状态
          current_state = DFAState::ASSIGN_SIGN_STATE;
          break;
        case '-':
          // 请填空：切换到减号的状态
          current_state = DFAState::MINUS_SIGN_STATE;
          break;
        case '+':
          // 请填空：切换到加号的状态
          current_state = DFAState::PLUS_SIGN_STATE;
          break;
        case '*':
          // 请填空：切换状态
          current_state = DFAState::MULTIPLICATION_SIGN_STATE;
          break;
        case '!':
          current_state = DFAState::HALF_NEQ_STATE;
          break;
        case '/':
          // 请填空：切换状态
          current_state = DFAState::DIVISION_SIGN_STATE;
          break;
        case '<':
          current_state = DFAState::LT_STATE;
          break;
        case '>':
          current_state = DFAState::GT_STATE;
          break;
        case '(':
          current_state = DFAState::LEFTBRACKET_STATE;
          break;
        case ')':
          current_state = DFAState::RIGHTBRACKET_STATE;
          break;
        case '{':
          current_state = DFAState::L_BRACE_STATE;
          break;
        case '}':
          current_state = DFAState::R_BRACE_STATE;
          break;
        case '"':
          current_state = DFAState::STRING_STATE;
          break;
        case ',':
          current_state = DFAState::COMA_STATE;
          break;
        case ':':
          current_state = DFAState::COLON_STATE;
          break;
        case ';':
          current_state = DFAState::SEMICOLON_STATE;
          break;
        // 不接受的字符导致的不合法的状态
        default:
          invalid = true;
          break;
        }
      }
      // 如果读到的字符导致了状态的转移，说明它是一个token的第一个字符
      if (current_state != DFAState::INITIAL_STATE){
        ;
      }
        // pos = previousPos();  // 记录该字符的的位置为token的开始位置
        // 读到了不合法的字符
      if (invalid)
      {
        printf("invalid detected!\n");
        // 回退这个字符
         unreadLast();
        // 返回编译错误：非法的输入
        return std::make_pair(std::optional<Token>(),
                              std::make_optional<CompilationError>(ErrorCode::InvalidInput));
      }
      // 如果读到的字符导致了状态的转移，说明它是一个token的第一个字符
      if (current_state != DFAState::INITIAL_STATE) // ignore white spaces
        ss << ch;                                   // 存储读到的字符
      break;
    }

      // 当前状态是无符号整数
    case UNSIGNED_INTEGER_STATE:
    {
      // 请填空：
      // 如果当前已经读到了文件尾，则解析已经读到的字符串为整数
      //     解析成功则返回无符号整数类型的token，否则返回编译错误
      // 如果读到的字符是数字，则存储读到的字符
      // 如果读到的字符不是数字，则回退读到的字符，并解析已经读到的字符串为整数
      //     解析成功则返回无符号整数类型的token，否则返回编译错误
      if (!current_char.has_value())
      {
        std::string num;
        ss >> num;
        ss.clear();
        return std::make_pair(std::make_optional<Token>(TokenType::UNSIGNED_INTEGER,
                                                        num),
                              std::optional<CompilationError>());
      }
      auto ch = current_char.value();
      if (isdigit(ch))
        ss << ch;
      else
      {
        unreadLast();
        std::string num;
        ss >> num;
        // std::cout<<num<<"\n";
        ss.clear();

        // return num
        return std::make_pair(std::make_optional<Token>(TokenType::UNSIGNED_INTEGER,
                                                        num),
                              std::optional<CompilationError>());
      }
      break;
    }
    case STRING_STATE:
    {
      if (!current_char.has_value())
      { // 字符串常量不允许读到行末尾还未结束
        return std::make_pair(std::optional<Token>(),
                              std::make_optional<CompilationError>(ErrorCode::InvalidInput));
        break;
      }
      auto ch = current_char.value();
      if (ch != '"' && ch != '\\')
        ss << ch;
      else if (ch == '\\')
      { //出现
        auto ch_peek = nextChar();
        if (ch_peek == '\\' || '\'' || '"' || 'n' || 'r' || 't')
        { //peek一下，没有问题就回退
          unreadLast();
          ss << ch; //没问题，读入
        }
        else
        {
          unreadLast(); //出现了其他字符
          return std::make_pair(std::optional<Token>(),
                                std::make_optional<CompilationError>(ErrorCode::InvalidInput));
        }
      }
      else if (ch == '"')
      { // 结束了
        std::string str;
        ss >> str;
        int len = str.length();
        str = str.substr(1, len - 1);
        ss.clear();
        return std::make_pair(std::make_optional<Token>(TokenType::STRING,
                                                        str),
                              std::optional<CompilationError>());
      }
      break;
    }
    case IDENTIFIER_STATE:
    {
      // 请填空：
      // 如果当前已经读到了文件尾，则解析已经读到的字符串
      //     如果解析结果是关键字，那么返回对应关键字的token，否则返回标识符的token
      // 如果读到的是字符或字母，则存储读到的字符
      // 如果读到的字符不是上述情况之一，则回退读到的字符，并解析已经读到的字符串
      //     如果解析结果是关键字，那么返回对应关键字的token，否则返回标识符的token
      if (!current_char.has_value())
      {
        std::string str;
        ss >> str;
        std::cout << str << "\n";
        if (str == "fn")
          return std::make_pair(std::make_optional<Token>(TokenType::FN,
                                                          str),
                                std::optional<CompilationError>()); //返回关键字

        else if (str == "let")
          return std::make_pair(std::make_optional<Token>(TokenType::LET,
                                                          str),
                                std::optional<CompilationError>()); //返回关键字

        else if (str == "as")
          return std::make_pair(std::make_optional<Token>(TokenType::AS,
                                                          str),
                                std::optional<CompilationError>()); //返回关键字

        else if (str == "const")
          return std::make_pair(std::make_optional<Token>(TokenType::CONST,
                                                          str),
                                std::optional<CompilationError>()); //返回关键字

        else if (str == "while")
          return std::make_pair(std::make_optional<Token>(TokenType::WHILE,
                                                          str),
                                std::optional<CompilationError>()); //返回关键字

        else if (str == "if")
          return std::make_pair(std::make_optional<Token>(TokenType::IF,
                                                          str),
                                std::optional<CompilationError>()); //返回关键字

        else if (str == "else")
          return std::make_pair(std::make_optional<Token>(TokenType::ELSE,
                                                          str),
                                std::optional<CompilationError>()); //返回关键字

        else if (str == "return")
          return std::make_pair(std::make_optional<Token>(TokenType::RETURN,
                                                          str),
                                std::optional<CompilationError>()); //返回关键字
        else if (str == "break")
          return std::make_pair(std::make_optional<Token>(TokenType::BREAK,
                                                          str),
                                std::optional<CompilationError>());
        else if (str == "continue")
          return std::make_pair(std::make_optional<Token>(TokenType::CONTINUE,
                                                          str),
                                std::optional<CompilationError>());
        else if( str == "int")
          return std::make_pair(std::make_optional<Token>(TokenType::TYPE,
                                                          str),
                                std::optional<CompilationError>());
        else if(str == "void")
          return std::make_pair(std::make_optional<Token>(TokenType::TYPE,
                                                          str),
                                std::optional<CompilationError>());
        else if(str == "double")
          return std::make_pair(std::make_optional<Token>(TokenType::TYPE,
                                                          str),
                                std::optional<CompilationError>());
        else if(str == "getint" || str == "getdouble" || str == "getchar"
             || str == "putint" || str == "putdouble" || str == "putchar"
             || str == "putstr" || str == "putln")
             return std::make_pair(std::make_optional<Token>(TokenType::STDIO,
                                                          str),
                                std::optional<CompilationError>());
        else
          return std::make_pair(std::make_optional<Token>(TokenType::IDENTIFIER,
                                                          str),
                                std::optional<CompilationError>());
      }
      auto ch = current_char.value();
      if (isalpha(ch) || isdigit(ch) || ch == '_')
      { //是字母、数字、下划线
        ss << ch;
      }
      //非以上三种情况 回退
      else
      {
        unreadLast();
        std::string str;
        ss >> str;
        ss.clear();
        if (str == "fn")
          return std::make_pair(std::make_optional<Token>(TokenType::FN,
                                                          str),
                                std::optional<CompilationError>()); //返回关键字

        else if (str == "let")
          return std::make_pair(std::make_optional<Token>(TokenType::LET,
                                                          str),
                                std::optional<CompilationError>()); //返回关键字

        else if (str == "as")
          return std::make_pair(std::make_optional<Token>(TokenType::AS,
                                                          str),
                                std::optional<CompilationError>()); //返回关键字

        else if (str == "const")
          return std::make_pair(std::make_optional<Token>(TokenType::CONST,
                                                          str),
                                std::optional<CompilationError>()); //返回关键字

        else if (str == "while")
          return std::make_pair(std::make_optional<Token>(TokenType::WHILE,
                                                          str),
                                std::optional<CompilationError>()); //返回关键字

        else if (str == "if")
          return std::make_pair(std::make_optional<Token>(TokenType::IF,
                                                          str),
                                std::optional<CompilationError>()); //返回关键字

        else if (str == "else")
          return std::make_pair(std::make_optional<Token>(TokenType::ELSE,
                                                          str),
                                std::optional<CompilationError>()); //返回关键字

        else if (str == "return")
          return std::make_pair(std::make_optional<Token>(TokenType::RETURN,
                                                          str),
                                std::optional<CompilationError>()); //返回关键字
        else if (str == "break")
          return std::make_pair(std::make_optional<Token>(TokenType::BREAK,
                                                          str),
                                std::optional<CompilationError>());
        else if (str == "continue")
          return std::make_pair(std::make_optional<Token>(TokenType::CONTINUE,
                                                          str),
                                std::optional<CompilationError>());
        else if( str == "int")
          return std::make_pair(std::make_optional<Token>(TokenType::TYPE,
                                                          str),
                                std::optional<CompilationError>());
        else if(str == "void")
          return std::make_pair(std::make_optional<Token>(TokenType::TYPE,
                                                          str),
                                std::optional<CompilationError>());
        else if(str == "double")
          return std::make_pair(std::make_optional<Token>(TokenType::TYPE,
                                                          str),
                                std::optional<CompilationError>());
        else if(str == "getint" || str == "getdouble" || str == "getchar"
             || str == "putint" || str == "putdouble" || str == "putchar"
             || str == "putstr" || str == "putln")
             return std::make_pair(std::make_optional<Token>(TokenType::STDIO,
                                                          str),
                                std::optional<CompilationError>());
        else
          return std::make_pair(std::make_optional<Token>(TokenType::IDENTIFIER,
                                                          str),
                                std::optional<CompilationError>());
      }
      break;
    }
      // 如果当前状态是加号
    case PLUS_SIGN_STATE:
    {
      // 请思考这里为什么要回退，在其他地方会不会需要
      unreadLast(); // Yes, we unread last char even if it's an EOF.
      // return ;  return 加号信息
      return std::make_pair(std::make_optional<Token>(TokenType::PLUS_SIGN,
                                                      '+'),
                            std::optional<CompilationError>());
      break;
    }
      // 当前状态为减号的状态
    case MINUS_SIGN_STATE:
    {
      auto ch = current_char.value();
      if (ch == '>')
      { // 如果减号后面是>。构成了箭头符号
        ss << ch;
        current_state = DFAState::ARROW_STATE;
        break;
      }
      // 请填空：回退，并返回减号token
      unreadLast();
      return std::make_pair(std::make_optional<Token>(TokenType::MINUS_SIGN,
                                                      '-'),
                            std::optional<CompilationError>());
      break;
    }

      // 请填空：
      // 对于其他的合法状态，进行合适的操作
      // 比如进行解析、返回token、返回编译错误
    case MULTIPLICATION_SIGN_STATE:
    {
      unreadLast();
      return std::make_pair(std::make_optional<Token>(TokenType::MULTIPLICATION_SIGN,
                                                      '*'),
                            std::optional<CompilationError>());
      break;
    }
    case DIVISION_SIGN_STATE:
    {
      auto ch = current_char.value();
      if( ch == '/'){ //  /后面再有一个/，说明是注释
        detect_comment = true;
        break;
      }
      unreadLast();
      return std::make_pair(std::make_optional<Token>(TokenType::DIVISION_SIGN,
                                                      '/'),
                            std::optional<CompilationError>());
      break;
    }
    case ASSIGN_SIGN_STATE:
    { // =
      auto ch = current_char.value();
      if (ch == '=')
      { // 如果等号后面继续接了一个等号
        ss << ch;
        current_state = DFAState::EQUAL_SIGN_STATE;
        break;
      }
      unreadLast();
      return std::make_pair(std::make_optional<Token>(TokenType::ASSIGN_SIGN,
                                                      '='),
                            std::optional<CompilationError>());
      break;
    }
    case EQUAL_SIGN_STATE:
    { // ==
      unreadLast();
      std::string str;
      ss >> str;
      ss.clear();
      return std::make_pair(std::make_optional<Token>(TokenType::EQUAL_SIGN,
                                                      str),
                            std::optional<CompilationError>());
      break;
    }
    case HALF_NEQ_STATE:
    {
      auto ch = current_char.value();
      if (ch == '=')
      {
        ss << ch;
        current_state = DFAState::NEQ_SIGN_STATE;
        break;
      }
      unreadLast(); //下一个字符不是=，报错
      return std::make_pair(std::optional<Token>(),
                            std::make_optional<CompilationError>(ErrorCode::InvalidInput));
      break;
    }
    case NEQ_SIGN_STATE:
    { // !=
      unreadLast();
      std::string str;
      ss >> str;
      ss.clear();
      return std::make_pair(std::make_optional<Token>(TokenType::NEQ_SIGN,
                                                      str),
                            std::optional<CompilationError>());
      break;
    }
    case LT_STATE:
    { // <
      auto ch = current_char.value();
      if (ch == '=')
      { // 如果等号后面继续接了一个等号
        ss << ch;
        current_state = DFAState::LE_STATE;
        break;
      }
      unreadLast();
      return std::make_pair(std::make_optional<Token>(TokenType::LT,
                                                      '<'),
                            std::optional<CompilationError>());
      break;
    }
    case GT_STATE:
    { // >
      auto ch = current_char.value();
      if (ch == '=')
      { // 如果等号后面继续接了一个等号
        ss << ch;
        current_state = DFAState::GE_STATE;
        break;
      }
      unreadLast();
      return std::make_pair(std::make_optional<Token>(TokenType::GT,
                                                      '>'),
                            std::optional<CompilationError>());
      break;
    }
    case LE_STATE:
    { // <=
      unreadLast();
      std::string str;
      ss >> str;
      ss.clear();
      return std::make_pair(std::make_optional<Token>(TokenType::LE,
                                                      str),
                            std::optional<CompilationError>());
      break;
    }
    case GE_STATE:
    { // >=
      unreadLast();
      std::string str;
      ss >> str;
      ss.clear();
      return std::make_pair(std::make_optional<Token>(TokenType::GT,
                                                      str),
                            std::optional<CompilationError>());
      break;
    }
    case LEFTBRACKET_STATE:
    {
      unreadLast();
      return std::make_pair(std::make_optional<Token>(TokenType::LEFT_BRACKET,
                                                      '('),
                            std::optional<CompilationError>());
      break;
    }
    case RIGHTBRACKET_STATE:
    {
      unreadLast();
      return std::make_pair(std::make_optional<Token>(TokenType::RIGHT_BRACKET,
                                                      ')'),
                            std::optional<CompilationError>());
      break;
    }
    case L_BRACE_STATE:
    {
      unreadLast();
      return std::make_pair(std::make_optional<Token>(TokenType::L_BRACE,
                                                      '{'),
                            std::optional<CompilationError>());
      break;
    }
    case R_BRACE_STATE:
    {
      unreadLast();
      return std::make_pair(std::make_optional<Token>(TokenType::R_BRACE,
                                                      '}'),
                            std::optional<CompilationError>());
      break;
    }
    case COMA_STATE:
    {
      unreadLast();
      return std::make_pair(std::make_optional<Token>(TokenType::COMA,
                                                      ','),
                            std::optional<CompilationError>());
      break;
    }
    case ARROW_STATE:{
      unreadLast();
      std::string str;
      ss >> str;
      ss.clear();
      return std::make_pair(std::make_optional<Token>(TokenType::ARROW,
                                                      str),
                            std::optional<CompilationError>());
      break;
    }
    case COLON_STATE:
    {
      unreadLast();
      return std::make_pair(std::make_optional<Token>(TokenType::COLON,
                                                      ':'),
                            std::optional<CompilationError>());
      break;
    }
    case SEMICOLON_STATE:
    {
      unreadLast();
      // printf("after semi is %d\n", nextChar());
      return std::make_pair(std::make_optional<Token>(TokenType::SEMICOLON,
                                                      ';'),
                            std::optional<CompilationError>());
      printf("semi break!\n");
      break;
    }
      // 预料之外的状态，如果执行到了这里，说明程序异常
    default:
      //DieAndPrint("unhandled state.");
      // 报错
      std::cout << "program exception occurs" << std::endl;
      std::abort();
      break;
    }
    if(detect_comment) // 发现了注释，退出循环
      break;
  }
  // 预料之外的状态，如果执行到了这里，说明程序异常
  // return std::make_pair(std::optional<Token>(),
  //                       std::optional<CompilationError>(ErrorCode::Unexpected));
}

std::optional<char> Tokenizer::nextChar()
{
  if (isEOF()){
    return {}; // 读到头了
  }
  nextPos();
  std::optional<char> result = line_buffer[_ptr];
  return result;
}

void Tokenizer::nextPos()
{
  if(!read_first_char){
    read_first_char = true; //最开始不加
    return ;
  }
  ++_ptr;
  return;
}

void Tokenizer::previousPos()
{
  --_ptr;
  return;
}

bool Tokenizer::isEOF()
{
  // printf("line length:%d\n",  line_buffer.size());
  return _ptr >= line_buffer.size();
}

void Tokenizer::unreadLast()
{
  if (_ptr == 0)
  {
    std::cout << "Reach line beginning" << std::endl;
    std::abort();
  }
  previousPos();
}

#define IS_FUNC(f)                                                          \
  inline bool f(char ch) { return std::f(static_cast<unsigned char>(ch)); } \
  using __let_this_macro_end_with_a_simicolon_##f = int

IS_FUNC(isprint);
IS_FUNC(isspace);
IS_FUNC(isblank);
IS_FUNC(isalpha);
IS_FUNC(isupper);
IS_FUNC(islower);
IS_FUNC(isdigit);
