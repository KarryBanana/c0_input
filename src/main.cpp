//main.cpp
#include <iostream>
#include "../include/analyzer.h"
#include "../include/tokenizer.h"
#include <fstream>

std::pair<std::vector<Token>, std::optional<CompilationError>> Tokenize(std::string input)
{
    Tokenizer tk(input);
    return tk.AllTokens(input);
}

bool Analyze(std::string input, std::string output)
{
    auto tk = Tokenize(input);
    return true;
}

int main(int argc, char** argv)
{
    //词法分析
    // auto tk = Tokenize("../token_test.txt");
    // if(tk.second.has_value())
    //     std::cout<<"tokenize failed!"<<std::endl;
    // else{
    //     int len = tk.first.size();
    //     std::cout<<len<<std::endl;
    //     for (int i = 0; i < len; ++i){
    //         std::cout<<tk.first[i].GetType()<<" "<<tk.first[i].GetValueString()<<std::endl;
    //     }
    // }
    bool flag = Analyze(argv[1], argv[2]); // analyzer_test
    if(!flag)
        return -1;
    return 0;
}
