#include <iostream>
#include <vector>

#include "elf_parser.hpp"
#include "cfg.hpp"


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
    for(const auto& symbol : symbols){
        if(symbol.executable){
            std::cout << symbol.name << " at " << symbol.address << " is executable\n";
        }
    }



    return 0;
}
