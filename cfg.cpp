#include "cfg.hpp"

#include <set>
#include <memory>
#include <iostream>


void cfg::explore_address(uint32_t instr_address, std::shared_ptr<BasicBlock> current_basic_block, std::set<uint32_t>& seen){
    if(seen.find(instr_address) != seen.end()){
        std::cout << "Already seen instruction " << std::hex << instr_address << "\n";
        return;
    }

    seen.emplace(instr_address);
    std::cout << "Going to explore "  << std::hex << instr_address << "\n";

    Instruction* new_instr = GetInstructionFactory().Build(instr_address, m_elf_file.data.data());
    if(new_instr == nullptr){
        std::cout << "Bad instruction!!\n";
        return;
    }

    std::cout << (*new_instr) << "\n";
}


cfg::cfg(ELFFile file, std::vector<Symbol> symbols)
    : m_elf_file(file)
{
    std::set<uint32_t> seen{};

    std::shared_ptr<BasicBlock> current_bb = std::make_shared<BasicBlock>(file.entry_offset);
    m_basic_blocks = { current_bb };

    explore_address(file.entry_offset, current_bb, seen);

    /*
    while(!symbols.empty()){
        Symbol actual_symbol = symbols.top();
        symbols.pop();

        if(actual_symbol.executable){
            explore_address(actual_symbol.address, current_bb, seen);
        }
    } */
}
