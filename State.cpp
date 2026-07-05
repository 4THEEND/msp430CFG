#include "State.hpp"

#include <sstream>
#include <iostream>


State::State(std::shared_ptr<BinaryLoader> loader, bool unkown)
    : modified_memory(), binary_file(loader) {
    for(int i = 0; i < registers.size(); i++){
        if(unkown)
            registers[i] = std::nullopt;
        else
            registers[i] = 0;
    }

    registers[CG] = 0;
    registers[PC] = std::nullopt;
}


void State::clear_gp_registers(){
    for(int i = 1; i < NB_REGISTERS; i++){
        registers[i] = std::nullopt;
    }

    registers[CG] = 0;
    modified_memory = {};
 }


std::optional<uint16_t> State::read_register(uint8_t reg, bool byte){
    std::optional<uint16_t> ret_value{};
    if(reg == SP && registers[PC] && binary_file->is_inside_ipe(registers[PC].value()))
        ret_value = registers[IPE_SP];
    ret_value = registers[reg];

    if(!byte || !ret_value)
        return ret_value;

    return std::make_optional(ret_value.value() & 0b1111111100000000);
}


void State::write_register(uint8_t reg, std::optional<uint16_t> val, bool byte){
    if(reg == SP && registers[PC] && binary_file->is_inside_ipe(registers[PC].value()))
        reg = IPE_SP;
    
    if(!byte || !val){
        registers[reg] = val;
        return;
    }

    if(!registers[reg])
        return;

    registers[reg] = (registers[reg].value() & 0b1111111100000000) + (val.value() & 0b11111111);

}


std::optional<uint8_t> State::read_byte(uint32_t addr){
    auto mem_it = modified_memory.find(addr);
    if(mem_it == modified_memory.end()){
        uint16_t res = binary_file->read_memory(addr);
        if(res == -1)
            return std::nullopt;
        return res & 0b11111111;
    }
    
    return mem_it->second;
}


std::optional<uint16_t> State::read_memory(std::optional<uint32_t> addr, bool byte){
    if(!addr)
        return std::nullopt;

    if(byte)
        return read_byte(addr.value());

    std::optional<uint8_t> bits = read_byte(addr.value() + 1);
    if(!bits)
        return bits;

    uint16_t content = bits.value();
    content <<= 8;

    bits = read_byte(addr.value());
    if(!bits)
        return bits;
    content += bits.value();

    return content;
}


void State::write_memory(std::optional<uint32_t> addr, std::optional<uint16_t> val, bool byte){
    if(!addr)
        return;

    if(!byte){
        if(val)
            modified_memory[addr.value() + 1] = val.value() >> 8;
        else 
            modified_memory[addr.value() + 1] = val;
    }

    if(val)
        modified_memory[addr.value()] = val.value();
    else
        modified_memory[addr.value()] = val;
}


std::string State::dumpState(){
    std::stringstream dump;

    dump << "=============[State Dump]=============\n";
    for(int i = 0; i < regs_assoc.size(); i++){
        if(registers[i])
            dump << regs_assoc[i] << ": 0x" << std::hex << registers[i].value() << std::dec << "\n";
        else
            dump << regs_assoc[i] << ": UNKNOWN\n";
    }
    dump << "Memory:\n";
    for(auto [addr, val] : modified_memory){
        if(val)
            dump << std::hex << "0x" << addr << ": 0x" << (uint16_t)val.value() << std::dec << "\n";
        else
            dump << std::hex << "0x" << addr << ": UNKNOWN\n";
    }

    dump << "======================================";
    return dump.str();
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


std::optional<uint16_t> operator^(std::optional<uint16_t> o1, std::optional<uint16_t> o2) {
    if (o1) {
        if (o2) {
            return std::make_optional(o1.value() ^ o2.value());
        }
    }
    return std::nullopt;
}


std::optional<uint16_t> operator&(std::optional<uint16_t> o1, std::optional<uint16_t> o2) {
    if (o1) {
        if (o2) {
            return std::make_optional(o1.value() & o2.value());
        }
    }
    return std::nullopt;
}


std::optional<uint16_t> operator|(std::optional<uint16_t> o1, std::optional<uint16_t> o2) {
    if (o1) {
        if (o2) {
            return std::make_optional(o1.value() & o2.value());
        }
    }
    return std::nullopt;
}


std::optional<uint16_t> op_not(std::optional<uint16_t> o){
    if(o)
        return ~o.value();
    return std::nullopt;
}


std::optional<uint16_t> op_arith_right_shift(std::optional<uint16_t> o)
{
    if(o)
        return (o.value() >> 1) | (o.value() & (1 << 15));
    return std::nullopt;
}


std::optional<uint16_t> op_swpb(std::optional<uint16_t> o)
{
    if(o)
        return (o.value() >> 8) + (o.value() << 8); 
    return std::nullopt;
}


std::optional<uint16_t> op_sxt(std::optional<uint16_t> o)
{
    if(o){
        if(o.value() & (1 << 7)){
            return o.value() | 0b1111111100000000;
        }
        return o.value() & 0b11111111;
    }
    return std::nullopt;
}
