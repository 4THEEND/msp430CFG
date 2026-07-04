#include "cfg.hpp"

#include <iostream>
#include <algorithm>
#include <sstream>


std::shared_ptr<BasicBlock> cfg::splitBlock(std::shared_ptr<BasicBlock> current_block, uint32_t addr){
    //std::cout << "Splitting at " << std::hex << addr << " " << current_block->first_address << std::dec << "\n";
    if(addr == current_block->first_address){
        return current_block;
    }
        

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
    uint32_t instr_address, std::shared_ptr<BasicBlock> current_basic_block, std::stack<uint32_t> return_stack, 
    std::map<uint32_t, int> seen_twice, State state, int treshold
){
    
    state.update_pc(instr_address);
    auto it_bb = seen.find(instr_address);
    std::shared_ptr<Instruction> new_instr{};
    
    //std::cout << "Going to explore 0x"  << std::hex << instr_address << std::dec << "\n";
    //std::cout << state.dumpState() << "\n";

    if(it_bb != seen.end()){
        auto it_seen = seen_twice.find(instr_address);
        if(it_seen != seen_twice.end()){
            if(it_seen->second >= treshold)
                return;
            seen_twice[instr_address]++;
        } else {
            seen_twice[instr_address] = 1;
        }

        auto res = std::find_if(
            current_basic_block->instructions.begin(), 
            current_basic_block->instructions.end(), 
            [=](std::shared_ptr<Instruction> instr){ return instr->address == instr_address; }
        );

        if(res == current_basic_block->instructions.end()){
            std::cerr << "Unable to find the instruction inside the current bb\n";
            return;
        }

        new_instr = *res;
    } else {
        new_instr = GetInstructionFactory().Build(instr_address, binary_file);
        if(new_instr == nullptr){
            std::cerr << "Bad instruction!!\n";
            return;       
        }
        seen.emplace(instr_address, current_basic_block);
        current_basic_block->addInstruction(new_instr);
    }

    if(new_instr == nullptr){
        std::cerr << "Bad instruction!!\n";
        return;
    }

    //std::cout << (*new_instr) << "\n";

    std::set<uint32_t> next_addrs = new_instr->get_next_addrs(state);
    if(new_instr->is_ret && !return_stack.empty()){
        if(return_stack.top() == 0x9e10)
            std::cout << "COUCOU2\n";
        next_addrs = { return_stack.top() };
        return_stack.pop();
    }

    if(new_instr->is_call){
        if((instr_address + 2 * new_instr->instruction_length) == 0x9e10)
            std::cout << "COUCOU\n";
        return_stack.push(instr_address + 2 * new_instr->instruction_length);
    }

    for(uint32_t next_addr : next_addrs){
        // In case of splits
        current_basic_block = seen.find(instr_address)->second;

        auto it = seen.find(next_addr);
        if((new_instr->modify_control_flow && it != seen.end()) 
            || (!new_instr->modify_control_flow && it != seen.end() && it->second != current_basic_block))
        {
            std::shared_ptr<BasicBlock> new_bb = splitBlock(it->second, it->first);

            if(std::find(current_basic_block->successors.begin(), current_basic_block->successors.end(), new_bb) == current_basic_block->successors.end()){
                current_basic_block->successors.emplace_back(new_bb);
                seen_twice = {};
            }
            //std::cout << seen_twice.size() << " " << new_bb << "\n";
            explore_address(next_addr, new_bb, return_stack, seen_twice, state);
        } 
        else if(new_instr->modify_control_flow){
            std::shared_ptr<BasicBlock> new_bb = std::make_shared<BasicBlock>(next_addr);

            m_basic_blocks.emplace_back(new_bb);
            current_basic_block->successors.emplace_back(new_bb);

            explore_address(next_addr, new_bb, return_stack, {}, state);
        }
        else {
            // Only one address inside
            explore_address(next_addr, current_basic_block, return_stack, seen_twice, state);
        }
    }

}


std::string get_name(uint32_t addr, std::vector<Symbol>& symbols){
    for(const auto& symbol : symbols){
        // Don't want local symbols (generated by the compiler)
        if(symbol.address == addr && symbol.name.rfind(".L", 0) != 0 && symbol.name.rfind("L0", 0) != 0){
            return symbol.name;
        }
    }
    return "";

}


void cfg::exportCFGToDOT(const std::string &filename)
{
    std::ofstream out(filename);
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

        std::string func_name = get_name(block->first_address, binary_file->symbols_table);
        if(func_name != "")
            label << func_name << ":\\l"; 

        for (const auto &instr : block->instructions)
        {
            label << "0x" << std::hex << instr->address << ": " << *instr << "\\l"; 
        }
        // construct a node along with the label
        out << "  \"" << std::hex << block->first_address << std::dec << "\" [label=\"" << label.str() << "\"];\n"; 
    }

    // construct edges between block and its successors
    for (const auto block : m_basic_blocks)
    {
        for (auto succ : block->successors)
        {
            out << "  \"" << std::hex << block->first_address << std::dec << "\" -> \"" << std::hex << succ->first_address << std::dec << "\";\n"; 
        }
    }

    out << "}\n";
    out.close();
    std::cout << "CFG exported to " << filename << "\n";
}


void cfg::disassemble(std::vector<std::string>& symbols_to_disassemble, bool use_symbols){
    if(binary_file != nullptr){
        seen = {};
        m_basic_blocks = {};

        std::shared_ptr<BasicBlock> current_bb{};

        if(symbols_to_disassemble.empty()){
            current_bb = std::make_shared<BasicBlock>(binary_file->entry_addr);
            m_basic_blocks = { current_bb };

            explore_address(binary_file->entry_addr, current_bb, {}, {}, State(binary_file, true));
        }

        if(binary_file->support_symbols && use_symbols){
            for(const auto& symbol : binary_file->symbols_table){
                if(symbol.executable && symbol.name.rfind(".L", 0) != 0 && symbol.name.rfind("L0", 0) != 0){
                    if(symbols_to_disassemble.empty() && binary_file->is_func(symbol)
                    || (!symbols_to_disassemble.empty() 
                    && std::find(symbols_to_disassemble.begin(), symbols_to_disassemble.end(), symbol.name) != symbols_to_disassemble.end()
                    )) {
                        std::cout << "Reversing from symbol " << symbol.name << "\n";
                        
                        auto it = seen.find(symbol.address);
                        if(it != seen.end()){
                            current_bb = splitBlock(it->second, symbol.address);
                        } else {
                            current_bb = std::make_shared<BasicBlock>(symbol.address);
                            m_basic_blocks.emplace_back(current_bb);
                        }

                        explore_address(symbol.address, current_bb, {}, {}, State(binary_file, true));
                    }
                }
            }
        }
    }
}


void cfg::add_edge(uint32_t source, uint32_t destination){
    auto it_source = seen.find(source);
    if(it_source == seen.end()){
        std::cerr << "Unable to find the BasicBlock containing " << std::hex << source << std::dec << "\n";
        return;
    }

    auto it_destination = seen.find(destination);
    if(it_destination == seen.end()){
        std::cerr << "Unable to find the BasicBlock containing " << std::hex << destination << std::dec << "\n";
        return;
    }

    auto it_bb_source = std::find(it_source->second->successors.begin(), it_source->second->successors.end(), it_destination->second);
    if(it_bb_source != it_source->second->successors.end()){
        std::cout << "Edge already exists\n";
        return;
    }

    it_source->second->successors.push_back(it_destination->second);
    std::cout << "Edge successfully added!\n";
}


std::vector<Path> cfg::walkthrough_bb(Timings& timings, std::shared_ptr<BasicBlock> bb, Path path, int depth){
    std::cout << depth << " " << path.size() << ": \n";
    for(std::shared_ptr<Instruction> instruction : bb->instructions){
        if(path.size() >= timings.trace_length()){
            std::cout << "Went through the entire trace!\n";
            return { path };
        }

        std::cout << std::hex << instruction->address << std::dec << "\n";
        if(timings[path.size()] == -1 || instruction->get_instruction_timing() == timings[path.size()])
            path.push_back(instruction);
        else {
            std::cout << "Dropping " << std::hex << instruction->address << std::dec << " " << path.size() << "\n";
            std::cout << *instruction << " " << instruction->get_instruction_timing() << " " << timings[path.size()] << "\n";
            return {};
        }
    }

    if(bb->successors.size() == 0){
        std::cout << "No successors to BasicBlock at address " << std::hex << bb->first_address << std::dec << "\n";
        std::cout << "You should maybe add new transitions to the CFG!\n";

        return { path };
    }

    std::vector<Path> new_paths{};
    for(std::shared_ptr<BasicBlock> next_bb : bb->successors){
        std::vector<Path> p = walkthrough_bb(timings, next_bb, path, depth + 1);
        new_paths.insert(new_paths.end(), p.begin(), p.end());
    }

    return new_paths;
}


void printPath(const Path& path){
    for(auto instruction : path){
        std::cout << std::hex << instruction->address << std::dec << " " << *instruction << "\n";
    }
}


void cfg::walkthrough(uint32_t begining_addr, Timings& timings){
    auto it_source = seen.find(begining_addr);
    if(it_source == seen.end()){
        std::cout << "Unable to find the BasicBlock containing " << std::hex << begining_addr << std::dec << "\n";
        return;
    }
    else if(it_source->second->first_address != begining_addr){
        std::cout << "We must begin at the begining of a BasicBlock\n";
        return;
    }

    std::vector<Path> paths{ walkthrough_bb(timings, it_source->second, {}) };
    for(const Path& path : paths){
        std::cout << "PATHHHHHHHH\n";
        printPath(path);
    }
}
