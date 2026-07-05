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

struct ExploreWork {
    uint32_t instr_address;
    std::shared_ptr<BasicBlock> current_basic_block;
    std::stack<uint32_t> return_stack;
    std::map<uint32_t, int> seen_twice;
    State state;
    int depth;
};


void cfg::explore_address(
    uint32_t instr_address, std::shared_ptr<BasicBlock> current_basic_block, std::stack<uint32_t> return_stack, 
    std::map<uint32_t, int> seen_twice, State state, int depth, int treshold
){
    std::vector<ExploreWork> worklist;
    worklist.push_back(ExploreWork{
        instr_address, current_basic_block, std::move(return_stack),
        std::move(seen_twice), std::move(state), depth
    });

    while(!worklist.empty()){
        ExploreWork work = std::move(worklist.back());
        worklist.pop_back();

        while(true){
            work.state.update_pc(work.instr_address);
            auto it_bb = seen.find(work.instr_address);
            std::shared_ptr<Instruction> new_instr{};

            //std::cout << "Going to explore 0x" << std::hex << work.instr_address << std::dec << "\n";

            if(it_bb != seen.end() && !it_bb->second->instructions.empty()){
                auto it_seen = work.seen_twice.find(work.instr_address);
                if(it_seen != work.seen_twice.end()){
                    if(it_seen->second >= treshold){
                        goto next_work_item;
                    }
                    work.seen_twice[work.instr_address]++;
                } else {
                    work.seen_twice[work.instr_address] = 1;
                }

                auto res = std::find_if(
                    work.current_basic_block->instructions.begin(),
                    work.current_basic_block->instructions.end(),
                    [&](std::shared_ptr<Instruction> instr){ return instr->address == work.instr_address; }
                );

                if(res == work.current_basic_block->instructions.end()){
                    std::cerr << "Unable to find the instruction inside the current bb\n";
                    goto next_work_item;
                }

                new_instr = *res;
            } else {
                new_instr = GetInstructionFactory().Build(work.instr_address, binary_file);
                if(new_instr == nullptr){
                    std::cerr << "Bad instruction!!\n";
                    goto next_work_item;
                }
                if(it_bb == seen.end())
                    seen.emplace(work.instr_address, work.current_basic_block);
                work.current_basic_block->addInstruction(new_instr);
            }

            std::set<uint32_t> next_addrs = new_instr->get_next_addrs(work.state);
            if(new_instr->is_ret && !work.return_stack.empty()){
                next_addrs = { work.return_stack.top() };
                work.return_stack.pop();
            }

            if(new_instr->is_call){
                work.return_stack.push(work.instr_address + 2 * new_instr->instruction_length);
            }

            if(next_addrs.empty()){
                goto next_work_item;
            }

            // In case of splits
            work.current_basic_block = seen.find(work.instr_address)->second;

            if(next_addrs.size() == 1 && !new_instr->modify_control_flow){
                uint32_t only_addr = *next_addrs.begin();
                auto it = seen.find(only_addr);
                bool needs_new_block = (it != seen.end() && it->second != work.current_basic_block);

                if(!needs_new_block){
                    work.instr_address = only_addr;
                    continue;
                }
            }

            for(uint32_t next_addr : next_addrs){
                auto it = seen.find(next_addr);

                if((new_instr->modify_control_flow && it != seen.end()) 
                    || (!new_instr->modify_control_flow && it != seen.end() && it->second != work.current_basic_block))
                {
                    std::shared_ptr<BasicBlock> new_bb = splitBlock(it->second, it->first);

                    if(std::find(work.current_basic_block->successors.begin(), work.current_basic_block->successors.end(), new_bb) == work.current_basic_block->successors.end()){
                        work.current_basic_block->successors.emplace_back(new_bb);
                        //std::cout << "NEW BRANCH\n";
                        worklist.push_back(ExploreWork{
                            next_addr, new_bb, work.return_stack,
                            {}, work.state, work.depth + 1
                        });
                    } else {
                        worklist.push_back(ExploreWork{
                            next_addr, new_bb, work.return_stack,
                            work.seen_twice, work.state, work.depth + 1
                        });
                    }
                } 
                else if(new_instr->modify_control_flow){
                    std::shared_ptr<BasicBlock> new_bb = std::make_shared<BasicBlock>(next_addr);

                    m_basic_blocks.emplace_back(new_bb);
                    seen.emplace(next_addr, new_bb);
                    work.current_basic_block->successors.emplace_back(new_bb);

                    //std::cout << "NEW BRANCH2\n";
                    worklist.push_back(ExploreWork{
                        next_addr, new_bb, work.return_stack,
                        {}, work.state, work.depth + 1
                    });
                }
                else {
                    worklist.push_back(ExploreWork{
                        next_addr, work.current_basic_block, work.return_stack,
                        work.seen_twice, work.state, work.depth + 1
                    });
                }
            }

            goto next_work_item;
        }

        next_work_item:;
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


void cfg::disassemble(std::vector<std::string>& symbols_to_disassemble, bool use_symbols, bool add){
    if(binary_file != nullptr){
        if(!add){
            seen = {};
            m_basic_blocks = {};
        }

        std::shared_ptr<BasicBlock> current_bb{};

        if(symbols_to_disassemble.empty()){
            current_bb = std::make_shared<BasicBlock>(binary_file->entry_addr);
            m_basic_blocks = { current_bb };

            explore_address(binary_file->entry_addr, current_bb, {}, {}, State(binary_file, true));
        } else {
            use_symbols = true;
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
    //std::cout << depth << " " << path.size() << ": \n";
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
        std::cout << "No successors to BasicBlock at address " << std::hex << bb->first_address << std::dec << " " << path.size() << "\n";
        //std::cout << "You should maybe add new transitions to the CFG!\n";

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
        std::cout << "PATHHHHHHHH: " << path.size() <<  "\n";
        printPath(path);
    }
}
