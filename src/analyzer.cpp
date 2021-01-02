#include "../include/analyzer.h"
#include "../include/action_scope.h"
#include "../include/function_list.h"
#include <iomanip>
#include <iostream>
#include <fstream>
#include <string.h>


Stack s; // 模拟栈
std::vector<FunctionList> flist; // 函数列表
Layer l;
int cmp = 0; // 用来区分 br.true 和 br.false
int main_num = -1; // 用来记main函数的编号

char* Analyzer::reverseData(unsigned char *num, int len)
{
    unsigned char *tmp = (unsigned char *)malloc(sizeof(num));
    for(int i = 0; i<len; ++i){
        tmp[i] = num[len - i - 1];
    }
    return (char*)tmp;
}
std::pair<bool, std::optional<CompilationError>>
Analyzer::Analyze(std::string output) 
{
    //std::cout<<_tokens.size()<<std::endl;
    // 程序最开始, 全局变量是第0层
    struct ActionLayer act;
    act.layer = 0;
    l.action_layer.emplace_back(act);
    // 为第一个函数准备
    struct FunctionList f;
    flist.emplace_back(f);
    
    auto result = Program(output);
    if(result.has_value())
        return std::make_pair(bool(), result);
    else 
        return std::make_pair(true, std::optional<CompilationError>());

}

// <程序>
std::optional<CompilationError> Analyzer::Program(std::string output)
{
    //std::cout<<"---------------output addr is "<<output<<std::endl;
    std::ofstream out(output, std::ios::out | std::ios::binary); // 输出文件
    if( !out.is_open() ) {
        std::cout << "Error output file";
        exit(1);
    }
    int magic = 0x72303b3e;
    int version = 0x00000001;
    out.write(reverseData((unsigned char *)&magic, sizeof(magic)), sizeof(magic)); // magic
    out.write(reverseData((unsigned char *)&version, sizeof(version)), sizeof(version)); // version

    int tmp = 0; int *cnt = &tmp;
    while(true){
        // 预读一个token,看看是不是声明语句
        auto next = nextToken();
        if( !next.has_value() ) return {};
        if(next.value().GetType() != TokenType::CONST &&
        next.value().GetType() != TokenType::LET){
            //std::cout<<"-----no declare"<<std::endl;
            unreadToken();
            break;
        }
        std::optional<CompilationError> err;
        unreadToken();
        switch (next.value().GetType()) {
            // 这里需要你针对不同的预读结果来调用不同的子程序
            // 注意我们没有针对空语句单独声明一个函数，因此可以直接在这里返回
            
            case TokenType::CONST: {
                auto err = DeclareStatement(cnt);
                if (err.has_value())
                    return err;
                break;
            }
            case TokenType::LET: {
                auto err = DeclareStatement(cnt);
                if (err.has_value())
                    return err;
                break;
            }
            default:
                break;
        }
    }
    
    while(true){
         // 预读一个token,看看是不是函数 function部分
        auto next = nextToken();
        if(!next.has_value() || next.value().GetType() != TokenType::FN){
            unreadToken();
            break;
        }
        unreadToken();
        switch (next.value().GetType()) {
            case TokenType::FN: {
                auto err = Function();
                if( err.has_value())
                 return err;
                break;
            }
            default:
                break;
        }
    }
    if(main_num == -1) // 没有找到main函数
        return std::make_optional<CompilationError>(ErrorCode::NoMain); 
    if(out.is_open()){ // 全局变量的个数为 globas + funcs + _start函数
        int globa_num = l.action_layer.front().symbols.size() + flist.size();
        out.write(reverseData((unsigned char *)&globa_num, sizeof(globa_num)), sizeof(globa_num)); // 所有全局变量的个数 
        std::vector<Token> globas = l.action_layer.front().symbols;
        int len_globas = globas.size();
        for(int i = 0;i<len_globas;++i){ //先处理函数外的全局变量
            out.write(reverseData((unsigned char *)&globas[i].is_const, sizeof(bool)), sizeof(bool));
            int var_len = 0x00000008; // 变量值的字节数为4字节
            out.write(reverseData((unsigned char *)&var_len, sizeof(int)), sizeof(int)); // 变量值的长度
            long var_value = 0; // 全局变量初始都是0
            out.write(reverseData((unsigned char *)&var_value, sizeof(long)), sizeof(long)); // 后续补充string类型
        }
        bool _startIsConst = 0x01;
        out.write(reverseData((unsigned char *)&_startIsConst, sizeof(bool)), sizeof(bool)); // _start是全局
        int _startcnt = 0x06;
        out.write(reverseData((unsigned char *)&_startcnt, sizeof(int)), sizeof(int)); // 函数名长度
        char _start[] = "_start";
        out.write(_start, strlen(_start)); // _start函数名

        int func_num = flist.size(); // 函数的个数 包括_start
        out.write(reverseData((unsigned char *)&func_num, sizeof(int)), sizeof(int));

        int _startIdx = globas.size(); // _start在全局变量中的位置
        out.write(reverseData((unsigned char *)&_startIdx, sizeof(int)), sizeof(int));
        int _startReturn = 0;
        out.write(reverseData((unsigned char *)&_startReturn, sizeof(int)), sizeof(int)); // _start无return
        out.write(reverseData((unsigned char *)&_startIdx, sizeof(int)), sizeof(int)); // _start无params 因为值都是0，就偷懒了
        out.write(reverseData((unsigned char *)&_startIdx, sizeof(int)), sizeof(int)); // _start无局部变量
        int _startInstruc = flist.front()._instrucs.size();  // _start中的指令个数
        out.write(reverseData((unsigned char *)&_startInstruc, sizeof(int)), sizeof(int));
        // //std::cout<<"_start has "<<_startFuncInstruc<<" instrus"<<std::endl;
        for (int i = 0; i < _startInstruc; ++i) { // _start具体指令
            Instruction ins = flist.front()._instrucs[i];
            unsigned char instrucNum = ins.ins_num;
            out.write(reverseData((unsigned char *)&instrucNum, sizeof(char)), sizeof(char));  // 指令数
            if (ins.has_op_num){
                if(ins.ins_num == 0x01)
                    out.write(reverseData((unsigned char *)&ins.op_num, sizeof(long)), sizeof(long)); // push操作数是8字节
                else
                    out.write(reverseData((unsigned char *)&ins.op_num, sizeof(int)), sizeof(int)); // 否则是4字节
            }
        }
        for(int i = 1; i < flist.size(); ++i){ // 处理剩下的函数
            int funcIdx = globas.size() + i; // 函数在全局变量中的位置
            out.write(reverseData((unsigned char *)&funcIdx, sizeof(int)), sizeof(int));
            int func_return = !flist[i].void_return;  // 函数return 是void还是非void
            out.write(reverseData((unsigned char *)&func_return, sizeof(int)), sizeof(int));
            int arga_num = flist[i]._params.size(); // 函数参数个数 
            out.write(reverseData((unsigned char *)&arga_num, sizeof(int)), sizeof(int)); 
            int loca_num = flist[i]._vars.size(); // 函数局部变量个数
            out.write(reverseData((unsigned char *)&loca_num, sizeof(int)), sizeof(int));  
            int instruc_num = flist[i]._instrucs.size(); // 函数中的指令个数
            out.write(reverseData((unsigned char *)&instruc_num, sizeof(int)), sizeof(int));
            int FuncInstruc = flist[i]._instrucs.size();
            for (int j = 0; j < FuncInstruc; ++j) {  // _start具体指令
                Instruction ins = flist[i]._instrucs[j];
                unsigned char instrucNum = ins.ins_num;
                out.write(
                    reverseData((unsigned char*)&instrucNum, sizeof(char)),
                    sizeof(char));  // 指令数
                if (ins.has_op_num) {
                    if (ins.ins_num == 0x01)
                        out.write(reverseData((unsigned char*)&ins.op_num,
                                              sizeof(long)),
                                  sizeof(long));  // push操作数是8字节
                    else
                        out.write(reverseData((unsigned char*)&ins.op_num,
                                              sizeof(int)),
                                  sizeof(int));  // 否则是4字节
                }
            }
        }
    }
    out.close();
    return {};
}

// 声明语句
std::optional<CompilationError> Analyzer::DeclareStatement(int *cnt)
{
    auto tk = nextToken();
    // std::cout<<tk.value().GetType()<<std::endl;
    if ( !tk.has_value() ){
        // 报错
        return std::make_optional<CompilationError>(ErrorCode::InvalidInput);
    }

    if( tk.value().GetType() != CONST && tk.value().GetType() != TokenType::LET)
        // 报错
        return std::make_optional<CompilationError>(ErrorCode::NoDeclare);
    unreadToken();
    switch (tk.value().GetType()) {
        case TokenType::CONST: {
            std::cout<<"const const const"<<std::endl;
            auto err = ConstDeclareStatement(cnt);
            if (err.has_value())
                return err;
            break;
        }
        case TokenType::LET: {
            std::cout<<"let let let "<<std::endl;
            auto err = LetDeclareStatement(cnt);
            if (err.has_value())
                return err;
            break;
        }
        default:
            break;
    }
    return {};
}

// funciton函数语句
std::optional<CompilationError> Analyzer::Function()
{
    // std::cout<<"function declare"<<std::endl;
    // 读到一个函数
    struct FunctionList f;
    flist.emplace_back(f);
    // FN
    auto tk = nextToken();
    if ( !tk.has_value() || tk.value().GetType() != TokenType::FN)
        // 报错
         return std::make_optional<CompilationError>(ErrorCode::InvalidInput);
    // std::cout<<"function fn"<<std::endl;
    // IDENT
    auto idnt = nextToken();
    if ( !idnt.has_value() || idnt.value().GetType() != TokenType::IDENTIFIER)
        // 报错
         return std::make_optional<CompilationError>(ErrorCode::InvalidInput);
    if( !isFuncDuplic(idnt.value()) ) // 函数名重复
        return std::make_optional<CompilationError>(ErrorCode::DuplicateFunc);
    flist.back().func_name = idnt.value().GetValueString();
    if(idnt.value().GetValueString() == "main") // 看看是不是main函数
        main_num = flist.size() - 1; // main函数编号
    
    // std::cout<<"function name"<<std::endl;
    // (
    auto l_brace = nextToken();
    if ( !l_brace.has_value() || l_brace.value().GetType() != TokenType::LEFT_BRACKET)
        // 报错
        return std::make_optional<CompilationError>(ErrorCode::InvalidInput);
    // std::cout<<"function ("<<std::endl;
    // 函数参数 不一定有
    auto r_brace = nextToken();
    if( r_brace.value().GetType() != TokenType::RIGHT_BRACKET){
        unreadToken();
        auto param = nextToken();
        if( !param.has_value() || ( param.value().GetType() != TokenType::CONST &&
        param.value().GetType() != TokenType::IDENTIFIER))
            // 报错
             return std::make_optional<CompilationError>(ErrorCode::InvalidInput); 
        unreadToken();
        auto err = FunctionParamList();
         if (err.has_value())
            return err;
        r_brace = nextToken();
    }
    if( !r_brace.has_value() || r_brace.value().GetType() != TokenType::RIGHT_BRACKET)
        // 报错
        return std::make_optional<CompilationError>(ErrorCode::InvalidInput);
    
    auto arrow = nextToken();
    if( !arrow.has_value() || arrow.value().GetType() != TokenType::ARROW)
        // 报错
         return std::make_optional<CompilationError>(ErrorCode::InvalidInput);
    // std::cout<<"function ->"<<std::endl;
    auto ty = nextToken();
    if(!ty.has_value() || ty.value().GetType() != TokenType::TYPE)
        // 报错
         return std::make_optional<CompilationError>(ErrorCode::InvalidInput);
    if(ty.value().GetValueString() == "void")
        flist.back().void_return = true;
    else
        flist.back().void_return = false;
    
    std::cout<<"function type"<<std::endl;
    // 函数主体
    int cnt = 0;
    auto block_stmt = BlockStatement(&cnt);
    if( block_stmt.has_value() )
        // 报错
         return block_stmt;
    std::cout<<"this func has "<<cnt<<std::endl; 
    
    return {};
}

// function_param_list语句
std::optional<CompilationError> Analyzer::FunctionParamList()
{
    auto err = FunctionParam();
    if (err.has_value())
        return err;
    while (true) {
        auto next = nextToken();
        if (!next.has_value() || next.value().GetType() != TokenType::COMA) {
            unreadToken();
            break;
        }
         err = FunctionParam();
        if (err.has_value())
            return err;
    }
    return {};
}
// function_param 函数参数
std::optional<CompilationError> Analyzer::FunctionParam() 
{
    auto next = nextToken();
    if( !next.has_value())
        // 报错
         return std::make_optional<CompilationError>(ErrorCode::InvalidInput);
    if( next.value().GetType() == TokenType::CONST){
        // 常量声明操作blablabla
        next = nextToken();
        std::cout<<"param const"<<std::endl;
        // 添加到函数列表的参数数组中
        Token p(next.value().GetType(), next.value().GetValueString() );
        p.is_const = true;
        flist.back()._params.emplace_back(p);
    }
    
    if(next.value().GetType() != TokenType::IDENTIFIER)
        // 报错
        return std::make_optional<CompilationError>(ErrorCode::InvalidInput);

     // 添加到函数列表的参数数组中
    Token p(next.value().GetType(), next.value().GetValueString() );
    flist.back()._params.emplace_back(p);

    for(int i = 0;i<flist.back()._params.size(); ++i){
        std::cout<<"params has "<<flist.back()._params[i].GetValueString()<<std::endl;
    }

    next = nextToken();
    if( !next.has_value() || next.value().GetType() != TokenType::COLON)
        // 报错
        return std::make_optional<CompilationError>(ErrorCode::InvalidInput);
    std::cout<<"param colon"<<std::endl;
    next = nextToken();
    if( !next.has_value() || next.value().GetType() != TokenType::TYPE){
        // 报错
        return std::make_optional<CompilationError>(ErrorCode::InvalidInput);
    }
    if(next.value().GetValueString() == "void") // void类型参数报错
        return std::make_optional<CompilationError>(ErrorCode::VoidVar);
    if(next.value().GetValueString() == "int") // 看看参数啥类型
        flist.back()._params.back().is_int = true;
    if(next.value().GetValueString() == "double")
        flist.back()._params.back().is_int = true;

    return {};

}
// 表达式
std::optional<CompilationError> Analyzer::Statement(int *cnt)
{
    auto next = nextToken();
    TokenType tmp = next.value().GetType();
    unreadToken();
    if( tmp == LET){
        auto err = LetDeclareStatement(cnt);
        if(err.has_value()) return err;
    }
    else if(tmp == CONST){
        auto err = ConstDeclareStatement(cnt);
        if(err.has_value()) return err; 
    }
    else if(tmp == IF){
        auto err = IfStatement(cnt);
        if(err.has_value()) return err; 
    }
    else if(tmp == WHILE){
        auto err = WhileStatement(cnt);
        if(err.has_value()) return err; 
    }
    else if(tmp == RETURN){
        auto err = ReturnStatement(cnt);
        if(err.has_value()) return err; 
    }
    else if(tmp == SEMICOLON){
        auto err = EmptyStatement();
        if(err.has_value()) return err; 
    }
    else if(tmp == L_BRACE){
        auto err = BlockStatement(cnt);
        if(err.has_value()) return err; 
    }
    else {
        auto err = ExprStatement(cnt);
        if(err.has_value()) return err; 
    }
    return {};
}
// 代码块
std::optional<CompilationError> Analyzer::BlockStatement(int *cnt)
{
    int tmp = *cnt;
    auto next = nextToken();
    if( !next.has_value() || next.value().GetType() != TokenType::L_BRACE)
        // 报错
        return std::make_optional<CompilationError>(ErrorCode::InvalidInput);
    // std::cout<<"block {"<<std::endl;

    // 读到一个{，新增一层
    struct ActionLayer act;
    act.layer = l.action_layer.size();
    l.action_layer.emplace_back(act);
    while (true) {
        next = nextToken();
        if( next.value().GetType() == TokenType::R_BRACE ){
            unreadToken();
            break;
        }
        else{
            unreadToken();
            auto err = Statement(cnt);
            if( err.has_value() )
                return err;
        }
    }
    next = nextToken();
    if( !next.has_value() || next.value().GetType() != TokenType::R_BRACE)
        // 报错
        return std::make_optional<CompilationError>(ErrorCode::InvalidInput);

    /*读到一个} 弹出一层   注意0弹出可能报错*/
    l.action_layer.pop_back();
    std::cout<<"this block has "<<*cnt - tmp <<std::endl;
    return {};
}
// 表达式语句
std::optional<CompilationError> Analyzer::ExprStatement(int *cnt)
{
    auto err = Expression(cnt);
    if( err.has_value() )
        return err;
    auto next = nextToken();
    if( !next.has_value() || next.value().GetType() != TokenType::SEMICOLON )
        // 报错
        return std::make_optional<CompilationError>(ErrorCode::InvalidInput);
    return {};
}
// 变量声明
std::optional<CompilationError> Analyzer::LetDeclareStatement(int *cnt)
{
    auto next = nextToken();
    //std::cout<<"let declare start"<<next.value().GetType()<<std::endl;
    // let
    if( !next.has_value() || next.value().GetType() != TokenType::LET )
        // 报错
        return std::make_optional<CompilationError>(ErrorCode::InvalidInput);
    next = nextToken();
    // IDENT
    if( !next.has_value() || next.value().GetType() != TokenType::IDENTIFIER )
        // 报错
        return std::make_optional<CompilationError>(ErrorCode::InvalidInput);
    //std::cout<<"let declare ident"<<next.value().GetType()<<std::endl;
    auto ident_name = next.value().GetValueString();
    // :
    next = nextToken();
    if( !next.has_value() || next.value().GetType() != TokenType::COLON )
        // 报错
        return std::make_optional<CompilationError>(ErrorCode::InvalidInput);
    //std::cout<<"let declare colon"<<next.value().GetType()<<std::endl;
    // type
    next = nextToken();
    if( !next.has_value() || next.value().GetType() != TokenType::TYPE ){
        // 报错
        return std::make_optional<CompilationError>(ErrorCode::InvalidInput);
    }
    auto type = next.value().GetValueString();
    if(type == "void") // 变量声明不能是void
        return std::make_optional<CompilationError>(ErrorCode::VoidVar);
    // = 不一定有
    // std::cout<<"let declare type"<<next.value().GetType()<<std::endl;

    // 将变量加入到当前作用域中
    auto ident = next.value().GetType();
    //std::cout<<"ident name  is "<<ident_name<<std::endl;
    Token var(ident, ident_name);
    if(type == "int")
        var.is_int = true;
    if(type == "double")
        var.is_double = true;
    // 变量名重复
    if( isVarDuplic(var)){
        // std::cout<<"duplicate ------"<<std::endl;
        return std::make_optional<CompilationError>(ErrorCode::DuplicateVar);
    }
    flist.back()._vars.emplace_back(var);
    var.off = flist.back()._vars.size() - 1;
    l.action_layer.back().symbols.emplace_back(var);

    while (true) {
        next = nextToken();
        if( next.value().GetType() != ASSIGN_SIGN){
            unreadToken();
            break;
        }
        l.action_layer.back().symbols.back().is_initial = true;  // 变量初始化了，注意取得的变量是否正确
        //std::cout<<"now the layer is "<<l.action_layer.back().layer<<std::endl;
        if(l.action_layer.back().layer != 0){
            int off = flist.back()._vars.size() - 1; // offset
            std::cout<<"loca "<<off<<std::endl; // 加载局部变量地址到栈上
            s.pushItem(ADDR); (*cnt)++;
            flist.back()._instrucs.emplace_back(Instruction(0x0a,off,true)); // 存入指令
        }
        else{
            int off = l.action_layer.back().symbols.size() - 1;
            std::cout<<"globa "<<off<<std::endl; // 加载全局变量地址到栈上
            s.pushItem(ADDR); (*cnt)++;
            flist.back()._instrucs.emplace_back(Instruction(0x0c,off,true)); // 存入指令
        }

        auto err = Expression(cnt);
        if( err.has_value() )
            return err;
        std::cout<<"store.64 "<<std::endl;
        flist.back()._instrucs.emplace_back(Instruction(0x17,0,false)); // 存入指令
        (*cnt)++;
        s.popItem(); s.popItem(); // 赋值结束弹两个
    }
    // ;
    next = nextToken();
    if( !next.has_value() || next.value().GetType() != TokenType::SEMICOLON )
        // 报错
        return std::make_optional<CompilationError>(ErrorCode::InvalidInput);
    // std::cout<<"let declare semicolon"<<next.value().GetType()<<std::endl;
    
    return {};
}
// 常量声明
std::optional<CompilationError> Analyzer::ConstDeclareStatement(int *cnt)
{
    auto next = nextToken();
    // const
    if( !next.has_value() || next.value().GetType() != TokenType::CONST )
        // 报错
        return std::make_optional<CompilationError>(ErrorCode::InvalidInput);
    next = nextToken();
    // IDENT
    if( !next.has_value() || next.value().GetType() != TokenType::IDENTIFIER )
        // 报错
        return std::make_optional<CompilationError>(ErrorCode::InvalidInput);

    auto ident_name = next.value().GetValueString();
    auto ident = next.value().GetType();

    next = nextToken();
    // :
    if( !next.has_value() || next.value().GetType() != TokenType::COLON )
        // 报错
        return std::make_optional<CompilationError>(ErrorCode::InvalidInput);
    next = nextToken();
    // type
    if( !next.has_value() || next.value().GetType() != TokenType::TYPE )
        // 报错
        return std::make_optional<CompilationError>(ErrorCode::InvalidInput);
    // 获取当前变量的类型
    auto type = next.value().GetValueString();
    if(type == "void") // 变量声明不能是void
        return std::make_optional<CompilationError>(ErrorCode::VoidVar);
    // 将常变量加入到当前作用域中
    Token var(ident, ident_name);
    if(type == "int")
        var.is_int = true;
    if(type == "double")
        var.is_double = true;

    if (isVarDuplic(var))
        return std::make_optional<CompilationError>(ErrorCode::DuplicateVar);
    flist.back()._vars.emplace_back(var);
    var.off = flist.back()._vars.size() - 1;
    l.action_layer.back().symbols.emplace_back(var);

    if(l.action_layer.back().layer != 0){
        int off = flist.back()._vars.size() - 1;
        std::cout<<"loca "<<off<<std::endl; // 加载局部变量地址到栈上
        flist.back()._instrucs.emplace_back(Instruction(0x0a,off,true));
        (*cnt)++;
    }
    else{
        int off = l.action_layer.back().symbols.size() - 1;
        std::cout<<"globa "<<off<<std::endl; // 加载全局变量地址到栈上
        flist.back()._instrucs.emplace_back(Instruction(0x0c,off,true));
        s.pushItem(ADDR); (*cnt)++;
    }

    next = nextToken();
    // =
    if( !next.has_value() || next.value().GetType() != TokenType::ASSIGN_SIGN )
        // 报错
        return std::make_optional<CompilationError>(ErrorCode::InvalidInput);
    // std::cout<<" = = = ="<<std::endl;
    auto err = Expression(cnt);
    if( err.has_value() )
        return err;

    l.action_layer.back().symbols.back().is_const = true; // 赋值没问题
    flist.back()._vars.back().is_initial = true;
    
    std::cout<<"store.64 "<<std::endl;
    flist.back()._instrucs.emplace_back(Instruction(0x17,0,false));
    (*cnt)++;
    s.popItem(); s.popItem(); // 赋值结束弹两个

    next = nextToken();
    // ;
    if( !next.has_value() || next.value().GetType() != TokenType::SEMICOLON )
        // 报错
        return std::make_optional<CompilationError>(ErrorCode::InvalidInput);
    return {};
}
// if
std::optional<CompilationError> Analyzer::IfStatement(int *cnt)
{
    auto next = nextToken();
    // if
    if( !next.has_value() || next.value().GetType() != TokenType::IF )
        // 报错
        return std::make_optional<CompilationError>(ErrorCode::InvalidInput);
    // expr
    auto err = Expression(cnt);
    if( err.has_value() )
        return err;
    // block_stmt
    err = BlockStatement(cnt);
    if ( err.has_value() )
        return err;
    next = nextToken();
    if( next.value().GetType() == TokenType::ELSE){
        next = nextToken();
        if( !next.has_value() ){
            // 报错
            return std::make_optional<CompilationError>(ErrorCode::InvalidInput);
        }
        if(next.value().GetType() == TokenType::L_BRACE){
            err = BlockStatement(cnt);
            if( err.has_value() )
                return err;
        } else if( next.value().GetType() == TokenType:: IF){
            err = IfStatement(cnt);
            if( err.has_value() )
                return err;
        }else{
            // 报错
            return std::make_optional<CompilationError>(ErrorCode::InvalidInput);
        }
    }
    else{
        unreadToken();
    }
    return {};
}
// while
std::optional<CompilationError> Analyzer::WhileStatement(int *cnt)
{
    int tmp = *cnt; //保存cnt
    auto next = nextToken();
    // while
    if( !next.has_value() || next.value().GetType() != TokenType::WHILE )
        // 报错
        return std::make_optional<CompilationError>(ErrorCode::InvalidInput);
    // expr
    auto err = Expression(cnt);
    if( err.has_value() )
        return err;
    if( !s.checkTopNum() )
        return std::make_optional<CompilationError>(ErrorCode::CanNotCompare);
    int jump_ins_idx = flist.back()._instrucs.size();
    // 此处可以获得跳转指令的位置
    if(cmp == -1 || cmp == 0){
        std::cout<<"br.false"<<std::endl;
        flist.back()._instrucs.emplace_back(Instruction(0x42,0,true));
        (*cnt)++;
    }
    if(cmp == 1){
        std::cout<<"br.true"<<std::endl;
        flist.back()._instrucs.emplace_back(Instruction(0x43,0,true));
        (*cnt)++;
    }
    int jump_while = *cnt;
    // block_stmt
    err = BlockStatement(cnt);
    if( err.has_value() )
        return err;
    // 此处无条件跳转回到头
    std::cout<<"br "<<-(*cnt - tmp) + 1<<std::endl;
    flist.back()._instrucs.emplace_back(Instruction(0x41,-(*cnt - tmp) + 1, true));
    (*cnt)++;
    std::cout<<"should br "<<*cnt - jump_while<<std::endl;
    flist.back()._instrucs[jump_ins_idx].op_num = *cnt - jump_while;  // 把跳转的值填回去
    return {}; 
}
// return 
std::optional<CompilationError> Analyzer::ReturnStatement(int *cnt)
{
    auto next = nextToken();
    // return 
    if( !next.has_value() || next.value().GetType() != TokenType::RETURN )
        // 报错
        return std::make_optional<CompilationError>(ErrorCode::InvalidInput);
    while(true){
        next = nextToken();
        if( next.value().GetType() == TokenType::SEMICOLON){
            unreadToken();
            break;
        }else{
            auto err = Expression(cnt);
            if( err.has_value() )
                return err;
        }
    }
    next = nextToken();
    // ;
    if( !next.has_value() || next.value().GetType() != TokenType::SEMICOLON )
        // 报错
        return std::make_optional<CompilationError>(ErrorCode::InvalidInput);
    return {};
}
// 空代码块
std::optional<CompilationError> Analyzer::EmptyStatement()
{
    auto next = nextToken();
    // ;
    if( !next.has_value() || next.value().GetType() != TokenType::SEMICOLON )
        // 报错
        return std::make_optional<CompilationError>(ErrorCode::InvalidInput);
    return {};
}

// 语句
std::optional<CompilationError> Analyzer::Expression(int *cnt)
{
    auto err = EqualExpr(cnt); // EqualExpr为优先级最低的语句
    if( err.has_value() )
        return err;
    auto next = nextToken();
    if(!next.has_value() || next.value().GetType() != ASSIGN_SIGN){
        unreadToken();
        return {};
    }
    err = EqualExpr(cnt); // 如果接的是=，往后分析
    if(err.has_value()) return err;
    if (!s.checkAssign()) {
        // 报错
        return std::make_optional<CompilationError>(ErrorCode::CanNotAssign);
    }
    // 赋值
    std::cout << "store.64" << std::endl;
    flist.back()._instrucs.emplace_back(Instruction(0x17,0,false)); // 存入指令
    (*cnt)++;
    s.popItem(); s.popItem();
    s.pushItem(VOID); // 赋值表达式的结果是void
    return {};
}
// 优先级最低的语句
std::optional<CompilationError> Analyzer::EqualExpr(int *cnt)
{
    auto err = GreatExpr(cnt);
    if( err.has_value() )
        return err;
    while( true ){
        auto next = nextToken();
        std::cout<<next.value().GetType()<<std::endl;
        if( !next.has_value() ){
            unreadToken();
            break;
        }
        if( next.value().GetType() == TokenType:: GT){
            err = GreatExpr(cnt);
            if( err.has_value() ) return err;

            s.cmpInt();  // 后续添加比较浮点数, 2 > 1 1在栈顶
            s.setGreat(); // 如果 栈顶 > 0，置入1
            cmp = -1; // br.false
            (*cnt)+=2;
        }
        else if( next.value().GetType() == TokenType::LT){
            err = GreatExpr(cnt); 
            if( err.has_value() ) return err;

            s.cmpInt();  // 后续添加比较浮点数
            s.setLess(); // 如果 栈顶 < 0，置入1 
            cmp = -1; // br.false
            (*cnt)+=2;
        }
        else if(next.value().GetType() == TokenType:: GE ){
            err = GreatExpr(cnt); 
            if( err.has_value() ) return err;

            s.cmpInt();  // 后续添加比较浮点数
            s.setLess();
            cmp = 1; // br.true
            (*cnt)+=2;
        }
        else if(next.value().GetType() == TokenType:: LE  ){
            err = GreatExpr(cnt); 
            if( err.has_value() ) return err;

            s.cmpInt();  // 后续添加比较浮点数
            s.setGreat();
            cmp = 1; // br.true
            (*cnt)+=2;
        }
        else if( next.value().GetType() == TokenType:: EQUAL_SIGN){
            err = GreatExpr(cnt); 
            if( err.has_value() ) return err;

            s.cmpInt();  // 后续添加比较浮点数
            cmp = 1;
            (*cnt)+=1;
        }
        else if( next.value().GetType() == TokenType::NEQ_SIGN){
            err = GreatExpr(cnt); 
            if( err.has_value() ) return err;

            s.cmpInt();  // 后续添加比较浮点数
            cmp = -1;
            (*cnt)+=1;
        }
        else{
            unreadToken();
            break;
        }
    }
    return {};
}
// 优先级第二低的语句
std::optional<CompilationError> Analyzer::GreatExpr(int *cnt)
{
    auto err = PlusExpr(cnt);
    if( err.has_value() )
        return err;
    while (true) {
        auto next = nextToken();
        if( !next.has_value() ){
            unreadToken();
            break;
        }
        if( next.value().GetType() == TokenType:: PLUS_SIGN){
            std::cout<<"+ + + +"<<std::endl;
            err = PlusExpr(cnt);
            if( err.has_value() ) return err;
            // add 两个数
            if( !s.canOper() ) // 不能相加
                return std::make_optional<CompilationError>(ErrorCode::CanNotOper);
            s.popItem(); s.popItem();
            s.pushItem(INT_NUM); 
            std::cout<<"add.i"<<std::endl;
            flist.back()._instrucs.emplace_back(Instruction(0x20,0,false));
            (*cnt)++;
        }
        else if(next.value().GetType() == TokenType:: MINUS_SIGN ){
            std::cout<<"- - - -"<<std::endl;
            err = PlusExpr(cnt);
            if( err.has_value() ) return err;
            // sub 两个数
            if( !s.canOper() ) // 不能相减
                return std::make_optional<CompilationError>(ErrorCode::CanNotOper);
            s.popItem(); s.popItem();
            s.pushItem(INT_NUM);
            std::cout<<"sub.i"<<std::endl;
            flist.back()._instrucs.emplace_back(Instruction(0x21,0,false));
            (*cnt)++;
        }
        else{
            unreadToken();
            break;
        }
    }
    return {};
    
}
// 优先级第三低的语句
std::optional<CompilationError> Analyzer::PlusExpr(int *cnt)
{
    auto err = MultiExpr(cnt);
    if( err.has_value() )
        return err;
    while (true) {
        auto next = nextToken();
        if( !next.has_value() ){
            unreadToken();
            break;
        }
        else if(next.value().GetType() == TokenType:: MULTIPLICATION_SIGN ){
            std::cout<<"multi multi multi"<<std::endl;
            err = MultiExpr(cnt);
            if( err.has_value() ) return err;
            // multi 两个数
            if( !s.canOper() ) // 不能相乘
                return std::make_optional<CompilationError>(ErrorCode::CanNotOper);
            s.popItem(); s.popItem();
            s.pushItem(INT_NUM); 
            std::cout<<"mul.i"<<std::endl;
            flist.back()._instrucs.emplace_back(Instruction(0x22,0,false));
            (*cnt)++;
        }
        else if(next.value().GetType() == TokenType::DIVISION_SIGN ){
            std::cout<<"div div div"<<std::endl;
            err = MultiExpr(cnt);
            if( err.has_value() ) return err;
            // divide 两个数
            if( !s.canOper() ) // 不能相除
                return std::make_optional<CompilationError>(ErrorCode::CanNotOper);
            s.popItem(); s.popItem();
            s.pushItem(INT_NUM); 
            std::cout<<"div.i"<<std::endl;
            flist.back()._instrucs.emplace_back(Instruction(0x23,0,false));
            (*cnt)++;
        }
        else{
            // std::cout<<next.value().GetValueString()<<std::endl;
            unreadToken();
            break;
        }
    }
    return {};
}
// 优先级第四低的语句
std::optional<CompilationError> Analyzer::MultiExpr(int *cnt)
{
    auto err = AsExpr(cnt);
    if( err.has_value() )
        return err;
    std::cout<<"1111111"<<std::endl;
    while (true) {
        auto next = nextToken();
        if( !next.has_value() ){
            unreadToken();
            break;
        }
        else if(next.value().GetType() == TokenType::AS){
            next = nextToken();
            if( !next.has_value() || next.value().GetType() != TokenType::TYPE)
                return std::make_optional<CompilationError>(ErrorCode::InvalidInput);
        }
        else{
            unreadToken();
            break;
        }
    }
    return {};
}
// 优先级第四高的语句
std::optional<CompilationError> Analyzer::AsExpr(int *cnt)
{
    bool neg_cnt = false; // false代表正
    while (true) {
        auto next = nextToken();
        if( !next.has_value() || next.value().GetType() != TokenType::MINUS_SIGN){
            unreadToken();
            break;
        }
        neg_cnt = !neg_cnt;
        std::cout<<" ------ minus"<<std::endl;
    }
    auto err = BeforeExpr(cnt);
    if( err.has_value() )
        return err;
        
    if(neg_cnt){ // 前面有奇数个负号, 将栈顶取反
        std::cout<<"neg.i"<<std::endl;
        (*cnt)++;
    }
    return {};
}
// 优先级第三高的语句 前置语句
std::optional<CompilationError> Analyzer::BeforeExpr(int *cnt)
{
    auto next = nextToken();
    std::cout<<"token type is "<<next.value().GetValueString()<<std::endl;
    if( next.value().GetType() == LEFT_BRACKET){
        auto err = BracketExpr(cnt);
        if(err.has_value()) return err;
    } else if(next.value().GetType() == IDENTIFIER){
        // 函数调用 或者 变量
        next = nextToken();
        unreadToken();
        if( next.value().GetType() == TokenType::LEFT_BRACKET){
            auto err = CallExpr(cnt);
            if(err.has_value()) return err;
        }else{
            unreadToken();
            auto err = IdentExpr(cnt);
            if(err.has_value()) return err;
        }
    } else if(next.value().GetType() == STDIO ){
        // 标准输入输出
        auto err = StdIO(next.value(), cnt);
        if(err.has_value()) return err;
    } else if(next.value().GetType() == UNSIGNED_INTEGER) {
        // push 一个INT 
        s.pushItem(INT_NUM);
        std::string num = next.value().GetValueString();
        std::cout<<"push "<<num<<std::endl;
        flist.back()._instrucs.emplace_back(Instruction(0x01,std::stoi(num),true));
        (*cnt)++;
        // std::cout<<"it's a num"<<std::endl;
        return {};
    } else if( next.value().GetType() == STRING) {
        return {};
    }
    else {
        // 报错
        return std::make_optional<CompilationError>(ErrorCode::InvalidInput);
    }
    return {};
}
// 优先级第最高的语句 变量语句
std::optional<CompilationError> Analyzer::IdentExpr(int *cnt)
{
    bool flag_const = false;
    auto tk = nextToken();
    std::cout<<"now tk is :"<<tk.value().GetValueString()<<std::endl;
    int idx = flist.back().getVarParams(tk.value(), &flag_const);
    if(idx == -1){
        idx = flist.back().getVarLocal(tk.value(), &flag_const);
        if( idx == -1 ){
            idx = flist.back().getVarGloba(tk.value(), &flag_const);
            if(idx == -1)
                // 报错
                return std::make_optional<CompilationError>(ErrorCode::UndefinedVar);
            else {
                std::cout<<"globa "<<idx<<std::endl; // 加载全局变量地址入栈
                flist.back()._instrucs.emplace_back(Instruction(0x0c,idx,true));
                (*cnt)++;
                s.pushItem(ADDR);
            }
        } else {
            std::cout<<"loca "<<idx<<std::endl; // 加载局部变量地址入栈
            flist.back()._instrucs.emplace_back(Instruction(0x0a,idx,true));
            (*cnt)++;
            s.pushItem(ADDR);
        }
    } else {
        std::cout<<"arga "<<idx<<std::endl; // 加载函数参数地址入栈
        flist.back()._instrucs.emplace_back(Instruction(0x0b,idx,true));
        (*cnt)++;
        s.pushItem(ADDR);
    }
    auto next = nextToken(); // 看下一个是不是 =
    unreadToken();
    if( next.value().GetType() != TokenType::ASSIGN_SIGN ){
        std::cout<<"load.64"<<std::endl; // 如果后面没有 = 
        flist.back()._instrucs.emplace_back(Instruction(0x13,0,false));
        (*cnt)++;
        s.popItem(); s.pushItem(INT_NUM); // 后面要补充Double情况
    } else {
        if(flag_const == true){ // 如果const变量后面跟了一个=，报错
            return std::make_optional<CompilationError>(ErrorCode::AssignToConst);
        }
    }
    return {};
}
// 优先级第最高的语句 调用语句
std::optional<CompilationError> Analyzer::CallExpr(int *cnt)
{
    // 已经读到IDENT了 接下来读 (
    auto next = nextToken();
    if( !next.has_value() ||  next.value().GetType() != TokenType::LEFT_BRACKET){
        // 报错
        return std::make_optional<CompilationError>(ErrorCode::InvalidInput);
    }
    // 看看下面一个是不是 )
    next = nextToken();
    if(!next.has_value() ){
        // 报错
        return std::make_optional<CompilationError>(ErrorCode::InvalidInput);
    }
    if (next.value().GetType() != TokenType::RIGHT_BRACKET){
        auto err = Expression(cnt);
        if( err.has_value() )   return err;
        while(true){
            next = nextToken();
            // 不是括号了 跳出
            if( !next.has_value() || next.value().GetType() != COMA ){
                unreadToken();
                break;
            } else{ // 读到了逗号
                err = Expression(cnt);
                if( err.has_value() )   return err;
            }
        }
    }else{ //读到了也先回退
        unreadToken();
    }
    // 处理 )
    next = nextToken();
    if(!next.has_value() || next.value().GetType() != RIGHT_BRACKET){
        // 报错
        return std::make_optional<CompilationError>(ErrorCode::InvalidInput);
    }
    return {};
}

// 优先级最高的语句 括号表达式
std::optional<CompilationError> Analyzer::BracketExpr(int *cnt)
{
    std::cout<<"cong tou fen xi "<<std::endl;
    auto err = Expression(cnt); //括号里又可以重头分析
    if(err.has_value()) return err;
    auto next = nextToken();
    std::cout<<next.value().GetType()<<std::endl;
    if( !next.has_value() || next.value().GetType() != RIGHT_BRACKET ){
        //报错
        return std::make_optional<CompilationError>(ErrorCode::InvalidInput);
    }
    std::cout<<")))))))"<<std::endl;
    return {};
}
// 标准输入输出IO
std::optional<CompilationError> Analyzer::StdIO(Token t,int *cnt)
{
    // 函数名已经读入了
    auto next = nextToken(); // (
    if( t.GetValueString() == "getint" || t.GetValueString() == "getdouble" || t.GetValueString() == "getchar"){
        if( !next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET )
            return std::make_optional<CompilationError>(ErrorCode::InvalidInput);
        // )
        next = nextToken();
        if( !next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET )
            return std::make_optional<CompilationError>(ErrorCode::InvalidInput);
        if(t.GetValueString() == "getint"){
            std::cout<<"scan.i"<<std::endl;  (*cnt)++;
            flist.back()._instrucs.emplace_back(Instruction(0x50,0,false));
            s.pushItem(INT_NUM); //读入数字压栈
        } else if(t.GetValueString() == "getdouble") {
            std::cout<<"scan.f"<<std::endl;  (*cnt)++;
            flist.back()._instrucs.emplace_back(Instruction(0x52,0,false));
            s.pushItem(DOUBLE_NUM); //读入浮点数压栈
        } else{
            std::cout<<"scan.c"<<std::endl;  (*cnt)++;
            flist.back()._instrucs.emplace_back(Instruction(0x51,0,false));
            s.pushItem(INT_NUM); //读入字符压栈
        }
    } else if(t.GetValueString() == "putln") {
        if( !next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET )
            return std::make_optional<CompilationError>(ErrorCode::InvalidInput);
        next = nextToken(); // )
        if( !next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET )
            return std::make_optional<CompilationError>(ErrorCode::InvalidInput);
        std::cout<<"println"<<std::endl;  (*cnt)++;
        flist.back()._instrucs.emplace_back(Instruction(0x58,0,false));
    
    } else { // 剩下的都是print命令
        if( !next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET )
            return std::make_optional<CompilationError>(ErrorCode::InvalidInput);
        auto err = Expression(cnt);
        if (err.has_value() ) return err;
        // )
        next = nextToken();
        if( !next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET ){
            //std::cout<<"shit the tk is "<<next.value().GetValueString()<<std::endl;
            return std::make_optional<CompilationError>(ErrorCode::InvalidInput);
        }
        if(t.GetValueString() == "putint"){
            if (s._stack.back() == INT_NUM) {
                std::cout << "print.i" << std::endl; (*cnt)++;
                flist.back()._instrucs.emplace_back(Instruction(0x54,0,false));
                s.popItem();  //弹栈
            } else
                return std::make_optional<CompilationError>(ErrorCode::InvalidOutput);
            
        } else if(t.GetValueString() == "putdouble") {
            if (s._stack.back() == DOUBLE_NUM) {
                std::cout << "print.f" << std::endl; (*cnt)++;
                flist.back()._instrucs.emplace_back(Instruction(0x56,0,false));
                s.popItem();  //弹栈
            } else
                return std::make_optional<CompilationError>(ErrorCode::InvalidOutput); 
        } else{
            if (s._stack.back() == INT_NUM) {
                std::cout << "print.c" << std::endl; (*cnt)++;
                flist.back()._instrucs.emplace_back(Instruction(0x55,0,false));
                s.popItem();  //弹栈
            } else
                return std::make_optional<CompilationError>(ErrorCode::InvalidOutput);    
        }
    }
    return {};
}
// 取下一个token
std::optional<Token> Analyzer::nextToken()
{
    if(_offset == _tokens.size()) return {};
    Token ret = _tokens[_offset];
    _offset++;
    return ret;
}

void Analyzer::unreadToken()
{
    if (_offset == 0) {
        std::cout << "Reach token beginning" << std::endl;
        std::abort();
    }
    _offset--;
    return ;
}
// 判断作用域内变量是否重名
bool Analyzer::isVarDuplic(Token t)
{
    // 同一层内不能重名
    std::vector<Token> vars = l.action_layer.back().symbols;
    int len = vars.size();
    for( int i = 0; i < len; ++i){
        // std::cout<<"symbol table has "<<vars[i].GetValueString()<<" is initial "<<vars[i].is_initial<<std::endl;
        if(vars[i].GetValueString() == t.GetValueString())
            return true;
    }
    // 与函数参数不能重名
    std::vector<Token> params = flist.back()._params;
    int len_param = params.size();
    for ( int i = 0; i< len_param; ++i){
        if(params[i].GetValueString() == t.GetValueString() )
            return true;
    }
    return false;
}

bool Analyzer::isFuncDuplic(Token t)
{
    // 不能和其他函数名重名
    int len_funcs = flist.size();
    for(int i = 0;i<len_funcs - 1; ++i){ //不算上自己
        if(flist[i].func_name == t.GetValueString() )
            return false;
    }
    // 不能和全局变量重名
    std::vector<Token> globas =
        l.action_layer.front().symbols;  // 全局变量在作用域的最下面一层
    int len_globa = globas.size();
    for (int i = 0; i < len_globa; ++i) {
        if (globas[i].GetValueString() == t.GetValueString())
            return false;
    }
    return true;
}

int FunctionList::getVarParams(Token t, bool *flag_const)
{
    
    std::vector<Token> params = flist.back()._params;
    
    int len_param = params.size();
    // 看看是不是参数
    for(int i = 0;i< len_param; ++i){
        //std::cout<<"func has paras: "<<params[i].GetValueString()<<std::endl;
        if(params[i].GetValueString() == t.GetValueString()){
            if(params[i].is_const)
                *flag_const = true;
            return i;
        }
    }
    return -1;
}

int FunctionList::getVarLocal(Token t, bool *flag_const) {
    int layer = l.action_layer.size();
    // 看看是不是局部变量，从符号表里找
    for (int i = layer - 1; i >= 1; --i) { // 第0层是全局变量
        std::vector<Token> vars = l.action_layer[i].symbols;
        int len_vars = vars.size();
        for(int j = 0; j< len_vars; ++j){
            //std::cout<<"var is: "<<vars[j].GetValueString()<<std::endl;
            if (vars[j].GetValueString() == t.GetValueString()){
                if(vars[j].is_const)
                    *flag_const = true;
                return vars[j].off;
            }
        }
    }
    return -1;
}

int FunctionList::getVarGloba(Token t, bool *flag_const) {
    // 看看是不是全局变量
    std::vector<Token> globas = l.action_layer[0].symbols;
    for(int i = 0;i<globas.size(); ++i){
        std::cout<<globas[i].GetValueString()<<std::endl;
    }
    int len_globas = globas.size();
    for (int i = 0; i < len_globas; ++i) {
        if (globas[i].GetValueString() == t.GetValueString()){
            if(globas[i].is_const)
                    *flag_const = true;
            return i;
        }
    }
    return -1;
}

bool Stack::checkAssign()
{
    StackItem top = s._stack.back();
    s._stack.pop_back(); // 先弹一个
    StackItem second_top = s._stack.back();
    s._stack.emplace_back(top); // 放回去
    if (second_top == ADDR && (top == INT_NUM || top == DOUBLE_NUM ) )
        return true;
    return false;
}

bool Stack::checkTopNum()
{
    if(s._stack.back() == INT_NUM || s._stack.back() == DOUBLE_NUM)
        return true;
    return false;
}

void Stack::pushItem(StackItem item)
{
    s._stack.emplace_back(item);
    std::cout<<"stack size : "<<s._stack.size()<<std::endl;
    return ;
}

void Stack::popItem()
{
    s._stack.pop_back();
    return ;
}

bool Stack::canOper()
{
    StackItem top = s._stack.back();
    s._stack.pop_back();
    StackItem second_top = s._stack.back();
    s._stack.emplace_back(top); // 装回去
    if ( (top == INT || top == INT_NUM ) && 
            (second_top == INT || second_top == INT_NUM) )
        return true;
    if ( (top == DOUBLE || top == DOUBLE_NUM ) && 
            (second_top ==  DOUBLE || second_top == DOUBLE_NUM))
        return true;
    return false;
}

bool Stack::cmpInt()
{
    if ( !s.canOper() ) // 看栈顶两个元素能否比较
        return false;
    s.popItem(); s.popItem();
    s.pushItem(INT_NUM); // 比较结果为-1 0 1
    
    std::cout<<"cmp.i"<<std::endl;
    flist.back()._instrucs.emplace_back(Instruction(0x30,0,false));
    return true;
}

bool Stack::cmpDouble()
{
    if ( !s.canOper() ) // 看栈顶两个元素能否比较
        return false;
    s.popItem(); s.popItem();
    s.pushItem(INT_NUM); // 比较结果为-1 0 1
    
    std::cout<<"cmp.f"<<std::endl;
    flist.back()._instrucs.emplace_back(Instruction(0x52,0,false));
    return true;
}

void Stack::setLess()
{
    std::cout<<"set.lt"<<std::endl;
    flist.back()._instrucs.emplace_back(Instruction(0x39,0,false));
}

void Stack::setGreat()
{
    std::cout<<"set.gt"<<std::endl;
    flist.back()._instrucs.emplace_back(Instruction(0x3a,0,false));
}
