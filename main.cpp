#include <iostream>
#include <vector>

#include "elf_parser.hpp"


int main(int argc, char** argv){
    if(argc < 2){
        std::cout << "You should provide a binary to disassemble\n";
        return 1;
    }

    ELFFile my_file{};
    std::string elf_name{ argv[1] };

    if(!loadELF(elf_name, my_file))
        return 1;

    std::vector<Symbol> symbols{ parseSymbolTable(my_file) };
    printSymbolNames(symbols);

    return 0;
}
