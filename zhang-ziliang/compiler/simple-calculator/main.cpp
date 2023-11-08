#include"calculator.hpp"

int main(int argc, char *argv[])
{
    std::string line;
    while (1)
    {
        std::getline(std::cin,line);
        if(line=="q") return 0;
        std::cout<<calculate(line)<<std::endl;
    }
    
    return 0;
}