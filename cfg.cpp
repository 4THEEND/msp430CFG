#include "cfg.hpp"

#include <iostream>
#include <algorithm>
#include <sstream>


std::shared_ptr<BasicBlock> cfg::splitBlock(std::shared_ptr<BasicBlock> current_block, uint32_t addr, AddressAssign& seen){
    std::cout << "Splitting at " << std::hex << addr << "\n";
    if(addr == current_block->first_address)
        return current_block;

    std::shared_ptr<BasicBlock> new_bb = std::make_shared<BasicBlock>(addr);
    m_basic_blocks.emplace_back(new_bb);

    auto res = std::find_if(
        current_block->instructions.begin(), 
        current_block->instructions.end(), 
        [=](std::shared_ptr<Instruction> instr){ return instr->address == addr; }
    );

    if(res == current_block->instructions.end()){
        std::cerr << "Unable to find the instruction inside the bb\n";
        return current_block;
    }

    std::size_t index = std::distance(std::begin(current_block->instructions), res);
    std::vector<std::shared_ptr<Instruction>> split_lo(current_block->instructions.begin(), current_block->instructions.begin() + index);
    std::vector<std::shared_ptr<Instruction>> split_hi(current_block->instructions.begin() + index, current_block->instructions.end());

    new_bb->successors = current_block->successors;
    current_block->successors = {new_bb};

    current_block->instructions = split_lo;
    new_bb->instructions = split_hi;

    for(auto instr : new_bb->instructions)
        seen[instr->address] = new_bb;

    return new_bb;
}


void cfg::explore_address(
    uint32_t instr_address, std::shared_ptr<BasicBlock> current_basic_block, 
    AddressAssign& seen, std::stack<uint32_t> return_stack
){
    if(seen.find(instr_address) != seen.end()){
        std::cout << "Already seen instruction " << std::hex << instr_address << "\n";
        return;
    }

    seen.emplace(instr_address, current_basic_block);
    std::cout << "Going to explore "  << std::hex << instr_address << "\n";

    std::shared_ptr<Instruction> new_instr = GetInstructionFactory().Build(instr_address, m_elf_file.data.data(), m_elf_file);
    if(new_instr == nullptr){
        std::cerr << "Bad instruction!!\n";
        return;
    }

    std::cout << (*new_instr) << "\n";
    current_basic_block->addInstruction(new_instr);

    std::vector<uint32_t> next_addrs = new_instr->next_addrs;
    if(new_instr->is_ret && !return_stack.empty()){
        uint32_t next_addr = return_stack.top();
        return_stack.pop();

        auto it = seen.find(next_addr);
        if(it != seen.end()){
            std::shared_ptr<BasicBlock> new_bb = splitBlock(it->second, it->first, seen);
            current_basic_block->successors.emplace_back(new_bb);
        } else {
            std::shared_ptr<BasicBlock> new_bb = std::make_shared<BasicBlock>(next_addr);

            m_basic_blocks.emplace_back(new_bb);
            current_basic_block->successors.emplace_back(new_bb);

            explore_address(next_addr, new_bb, seen, return_stack);
        }
    }
    else if(new_instr->modify_control_flow){
        for(uint32_t next_addr : next_addrs){
            // In case of splits
            current_basic_block = seen.find(instr_address)->second;

            auto it = seen.find(next_addr);
            if(it != seen.end()){
                std::shared_ptr<BasicBlock> new_bb = splitBlock(it->second, it->first, seen);
                current_basic_block->successors.emplace_back(new_bb);
            } else {
                std::shared_ptr<BasicBlock> new_bb = std::make_shared<BasicBlock>(next_addr);

                m_basic_blocks.emplace_back(new_bb);
                current_basic_block->successors.emplace_back(new_bb);

                if(new_instr->is_call)
                    return_stack.push(instr_address + 2 * new_instr->instruction_length);
                explore_address(next_addr, new_bb, seen, return_stack);
            }
        }
    } 
    else {
        // Only one address inside
        explore_address(next_addrs.front(), current_basic_block, seen, return_stack);
    }
}


void cfg::exportCFGToDOT(const std::string &filename)
{
    std::ofstream out(filename + ".dot");
    if (!out.is_open())
    {
        std::cerr << "Failed to open file for CFG output.\n";
        return;
    }

    out << "digraph CFG {\n";
    out << "  node [shape=box fontname=\"Courier\"];\n";

    for (const auto block : m_basic_blocks)
    {
        std::stringstream label;
        //construct a label with all instructions along with their address
        for (const auto &instr : block->instructions)
        {
            label << "0x" << std::hex << instr->address << ": " << *instr << "\\l"; 
        }
        // construct a node along with the label
        out << "  \"" << std::hex << block->first_address << "\" [label=\"" << label.str() << "\"];\n"; 
    }

    // construct edges between block and its successors
    for (const auto block : m_basic_blocks)
    {
        for (auto succ : block->successors)
        {
            out << "  \"" << std::hex << block->first_address << "\" -> \"" << std::hex << succ->first_address << "\";\n"; 
        }
    }

    out << "}\n";
    out.close();
    std::cout << "CFG exported to " << filename << ".dot\n";
}


cfg::cfg(ELFFile file, std::vector<Symbol> symbols)
    : m_elf_file(file)
{
    AddressAssign seen{};
    std::stack<uint32_t> return_stack{};

    std::shared_ptr<BasicBlock> current_bb = std::make_shared<BasicBlock>(file.entry_addr);
    m_basic_blocks = { current_bb };

    explore_address(file.entry_addr, current_bb, seen, return_stack);

    /*
    while(!symbols.empty()){
        Symbol actual_symbol = symbols.top();
        symbols.pop();

        if(actual_symbol.executable){
            explore_address(actual_symbol.address, current_bb, seen);
        }
    } */
}
