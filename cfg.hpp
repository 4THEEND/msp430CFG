#pragma once

#include <vector>
#include <map>
#include <memory>
#include <stack>

#include "Instruction.hpp"
#include "elf_parser.hpp"


struct BasicBlock
{
    std::vector<std::shared_ptr<Instruction>> instructions;

    uint32_t first_address;

    std::vector<std::shared_ptr<BasicBlock>> successors;

    BasicBlock(uint32_t address) : first_address(address) {};
    inline void addInstruction(std::shared_ptr<Instruction> instruction){ instructions.emplace_back(instruction); };
    inline int nb_instruction(){ return instructions.size(); };
};

using AddressAssign = std::map<uint32_t, std::shared_ptr<BasicBlock>>;


class cfg
{
private:
    std::vector<std::shared_ptr<BasicBlock>> m_basic_blocks;
    ELFFile m_elf_file;
    std::vector<Symbol> m_symbols;

public:
    cfg(ELFFile file, std::vector<Symbol>& symbols, std::vector<std::string>& symbols_to_disassemble);
    void exportCFGToDOT(const std::string& filename);

private:
    std::shared_ptr<BasicBlock> splitBlock(std::shared_ptr<BasicBlock> current_block, uint32_t addr, AddressAssign& seen);
    void explore_address(uint32_t instr_address, std::shared_ptr<BasicBlock> current_basic_block, AddressAssign& seen, std::stack<uint32_t> return_stack);
};
