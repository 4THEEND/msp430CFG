#pragma once

#include <vector>
#include <map>
#include <memory>
#include <stack>

#include "Instruction.hpp"
#include "binary_loader.hpp"
#include "Timings.hpp"


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
using Path = std::vector<std::shared_ptr<Instruction>>;


class cfg
{
private:
    std::vector<std::shared_ptr<BasicBlock>> m_basic_blocks;
    AddressAssign seen;
    std::shared_ptr<BinaryLoader> binary_file;

public:
    cfg(std::shared_ptr<BinaryLoader> file) : binary_file(file){};
    void exportCFGToDOT(const std::string& filename);
    void disassemble(std::vector<std::string>& symbols_to_disassemble);
    void add_edge(uint32_t source, uint32_t destination);
    void walkthrough(uint32_t begining_addr, Timings& timings);

private:
    std::shared_ptr<BasicBlock> splitBlock(std::shared_ptr<BasicBlock> current_block, uint32_t addr);
    void explore_address(uint32_t instr_address, std::shared_ptr<BasicBlock> current_basic_block, std::stack<uint32_t> return_stack);
    std::vector<Path> walkthrough_bb(Timings& timings, std::shared_ptr<BasicBlock> bb, Path path, int depth = 0);
};
