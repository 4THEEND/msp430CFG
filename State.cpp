#include "State.hpp"

#include <sstream>
#include <iostream>


State::State(uint32_t base_pc, std::shared_ptr<BinaryLoader> loader, bool unkown)
    : modified_memory(), binary_file(loader) {
    for(int i = 0; i < registers.size(); i++){
        if(unkown)
            registers[i] = std::nullopt;
        else
            registers[i] = 0;
    }
    registers[PC] = base_pc;
}


std::optional<uint16_t> State::read_register(uint8_t reg){
    if(reg == SP && registers[PC] && binary_file->is_inside_ipe(registers[PC].value()))
        return registers[IPE_SP];
    return registers[reg];
}


void State::write_register(uint8_t reg, std::optional<uint16_t> val){
    if(reg == SP && registers[PC] && binary_file->is_inside_ipe(registers[PC].value()))
        registers[IPE_SP] = val;
    registers[reg] = val;
}


std::optional<uint16_t> State::read_memory(std::optional<uint32_t> addr){
    if(!addr)
        return std::nullopt;

    return std::nullopt;
}


void State::write_memory(std::optional<uint32_t> addr, std::optional<uint16_t> val){

}


std::optional<uint16_t> operator+(std::optional<uint16_t> o1, std::optional<uint16_t> o2) {
    if (o1) {
        if (o2) {
            return std::make_optional(o1.value() + o2.value());
        }
    }
    return std::nullopt;
}


std::optional<uint16_t> operator-(std::optional<uint16_t> o1, std::optional<uint16_t> o2) {
    if (o1) {
        if (o2) {
            return std::make_optional(o1.value() - o2.value());
        }
    }
    return std::nullopt;
}


std::string State::dumpState(){
    std::stringstream dump;

    dump << "=============[State Dump]=============\n";
    for(int i = 0; i < regs_assoc.size(); i++){
        if(registers[i])
            dump << regs_assoc[i] << ": " << std::hex << registers[i].value() << std::dec << "\n";
        else
            dump << regs_assoc[i] << ": UNKNOWN\n";
    }

    dump << "======================================";
    return dump.str();
}
