#include "State.hpp"


State::State(uint32_t base_pc, bool unkown)
    : modified_memory() {
    for(int i = 0; i < registers.size(); i++){
        if(unkown)
            registers[i] = std::nullopt;
        else
            registers[i] = 0;
    }
    registers[PC] = base_pc;
}
