#pragma once

#include <vector>

enum StackItem{
    ADDR, //地址
    INT_NUM, 
    DOUBLE_NUM,
    VOID,
    INT, // 变量
    DOUBLE // 变量
};

class Stack{
  public:
     void pushItem(StackItem);
     void popItem();
     bool canOper();
     bool cmpInt();
     bool cmpDouble();
     void setLess();
     void setGreat();
     bool checkAssign(); // 检查赋值是否可行
     bool checkTopNum(); // 检查是否能够cmp
  public:
     std::vector<StackItem> _stack;

};


