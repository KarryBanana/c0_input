#pragma once

class Instruction{
  public:
    Instruction(int i_num, int num, bool has_num): // 初始化
      ins_num(i_num),
      op_num(num),
      has_op_num(has_num) {}
  public:
    int ins_num; // 虚拟机指令的编号
    int op_num; // 操作数
    bool has_op_num; // 指令是否有操作数
};