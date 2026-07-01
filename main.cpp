#include <iostream>
#include <vector>

#include <argparse/argparse.hpp>

#include "binary_loader.hpp"
#include "cfg.hpp"


int main(int argc, char** argv){
    argparse::ArgumentParser program("ipe-disasm");

    program.add_argument("binary")
        .help("Binary of which we want to build the cfg");
    
    program.add_argument("-o")
        .help("Name of the output .dot file")
        .default_value("cfg.dot");

    program.add_argument("-s", "--symbols")
        .help("Where we want to start our recursive disassembly")
        .nargs(0, -1);

    try {
        program.parse_args(argc, argv);
    }
    catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    ELFFile my_file{};
    std::string elf_name{ program.get("binary") };

    if(!my_file.loadBinary(elf_name))
        return 1;

    if(!my_file.getEntryOffset())
        return 1;

    std::vector<Symbol> symbols{ my_file.parseSymbolTable() };

    auto symbols_to_start = program.get<std::vector<std::string>>("-s");

    cfg binary_cfg{ &my_file, symbols, symbols_to_start };
    binary_cfg.exportCFGToDOT(program.get("-o"));

    return 0;
}
