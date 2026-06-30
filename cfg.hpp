#pragma once

#include <vector>
#include <set>
#include <memory>

#include "Instruction.hpp"
#include "elf_parser.hpp"

struct BasicBlock
{
    std::vector<Instruction> instructions;

    uint32_t first_address;
    uint32_t block_length;

    std::vector<std::shared_ptr<BasicBlock>> successors;

    BasicBlock(uint32_t address) : first_address(address), block_length(0) {};
};


class cfg
{
private:
    std::vector<std::shared_ptr<BasicBlock>> m_basic_blocks;
    ELFFile m_elf_file;

public:
    cfg(ELFFile file, std::vector<Symbol> symbols);

private:
    void explore_address(uint32_t instr_address, std::shared_ptr<BasicBlock> current_basic_block, std::set<uint32_t>& seen);
};
