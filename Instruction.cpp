#include "Instruction.hpp"

#include <iomanip>


std::string to_hex_string(uint16_t n){
    std::stringstream stream;
    stream << "0x" << std::hex << n;
    return stream.str();
}


std::string Instruction::decorate_reg(uint8_t reg, AddressingMode Am, uint16_t extension){
    switch(Am){
        case AddressingMode::REGISTER_MODE:
            return regs_assoc[reg];
        case AddressingMode::INDEXED_MODE:
        case AddressingMode::SYMBOLIC_MODE:
            return to_hex_string(extension) + "(" + regs_assoc[reg] + ")";
        case AddressingMode::ABSOLUTE_MODE:
            return "&" + to_hex_string(extension);
        case AddressingMode::INDIRECT_REGISTER_MODE:
            return "@" + regs_assoc[reg];
        case AddressingMode::INDIRECT_AUTOINCREMENT:
            return "@" + regs_assoc[reg] + "+";
        case AddressingMode::IMMEDIATE_MODE:
        case AddressingMode::CONSTANT_MODE0:
        case AddressingMode::CONSTANT_MODE1:
        case AddressingMode::CONSTANT_MODE2:
        case AddressingMode::CONSTANT_MODE4:
        case AddressingMode::CONSTANT_MODE8:
        case AddressingMode::CONSTANT_MODE_NEG1:
            return "#" + to_hex_string(extension);
        default:
            return "";
    }
}


uint16_t Instruction::read_memory(uint32_t address, uint8_t* memory){
    uint16_t content = memory[address + 1];
    content <<= 8;
    content += memory[address];

    return content;
}


AddressingMode Instruction::parse_mode(uint8_t Am, uint8_t source){
    switch(Am){
        case AS_MODE_0:
            return AddressingMode::REGISTER_MODE;
        case AS_MODE_1:
            if(source == SR)
                return AddressingMode::ABSOLUTE_MODE;
            else if(source == PC)
                return AddressingMode::SYMBOLIC_MODE;
            return  AddressingMode::INDEXED_MODE;
        case AS_MODE_2:
            return AddressingMode::INDIRECT_REGISTER_MODE;
        case AS_MODE_3:
            if(source == PC)
                return AddressingMode::IMMEDIATE_MODE;
            return AddressingMode::INDIRECT_AUTOINCREMENT;
        default:
            std::cerr << "Unable to find the right mode\n";
            return AddressingMode::INVALID_MODE;
    }
}


AddressingMode Instruction::parse_mode(AddressingMode Am, uint16_t immediate){
    if(immediate == 0)
        return AddressingMode::CONSTANT_MODE0;
    else if(immediate == 1)
        return AddressingMode::CONSTANT_MODE1;
    else if(immediate == 2)
        return AddressingMode::CONSTANT_MODE2;
    else if(immediate == 4)
        return AddressingMode::CONSTANT_MODE4;
    else if(immediate == 8)
        return AddressingMode::CONSTANT_MODE8;
    else if(immediate == -1)
        return AddressingMode::CONSTANT_MODE_NEG1;
    return Am;
}


bool Instruction::should_get_complement(AddressingMode Am){
    return (Am == AddressingMode::INDEXED_MODE)
        || (Am == AddressingMode::SYMBOLIC_MODE)
        || (Am == AddressingMode::ABSOLUTE_MODE)
        || (Am == AddressingMode::IMMEDIATE_MODE);
}


bool Format1Instruction::consume(uint32_t instruction_address, uint8_t* memory){
    instruction_length = 1;
    uint16_t instruction_header = read_memory(instruction_address, memory);
    if(instruction_header >> 12 != opcode)
        return false;
    
    source = (instruction_header >> 8) & 0b1111;
    As = parse_mode((instruction_header >> 4) & 0b11, source);
    if(should_get_complement(As)){
        instruction_length++;
        source_complement = read_memory(instruction_address + 2, memory);
        As = parse_mode(As, source_complement);
    }


    destination = instruction_header & 0b1111;
    Ad = parse_mode((instruction_header >> 7) & 1, destination);
    if(should_get_complement(Ad)){
        instruction_length++;
        destination_complement = read_memory(instruction_address + 4, memory);
        Ad = parse_mode(Ad, destination_complement);
    }

    byte_instruction = (instruction_header >> 6) & 1;
    return true;
} 


std::string Format1Instruction::abstractGetString(std::string instruction_name){
    return instruction_name 
        + (byte_instruction ? ".s " : " ") 
        + decorate_reg(source, As, source_complement) 
        + ", " 
        + decorate_reg(destination, Ad, destination);
}


bool Format2Instruction::consume(uint32_t instruction_address, uint8_t* memory) {
    uint16_t instruction_header = read_memory(instruction_address, memory);
    instruction_length = 1;
    if((instruction_header >> 10 == FORMAT_2_PREFIX) && (((instruction_header >> 7) & 0b111) == opcode))
        return false;

    source = instruction_header & 0b1111;
    As = parse_mode((instruction_header >> 4) & 0b11, source);
    if(should_get_complement(As)){
        instruction_length++;
        source_complement = read_memory(instruction_address + 2, memory);
        As = parse_mode(As, source_complement);
    }

    byte_instruction = (instruction_header >> 6) & 1;
    return true;
}


std::string Format2Instruction::abstractGetString(std::string instruction_name){
    return instruction_name 
        + (byte_instruction ? ".s " : " ") 
        + decorate_reg(source, As, source_complement);
}


bool Format3Instruction::consume(uint32_t instruction_address, uint8_t* memory){
    uint16_t instruction_header = read_memory(instruction_address, memory);
    instruction_length = 1;
    if((instruction_header >> 13 == FORMAT_3_PREFIX) && (((instruction_header >> 10) & 0b111) == condition))
        return false;

    pc_offset = (instruction_header & 0b1111111111);
    return true;
}


std::string Format3Instruction::abstractGetString(std::string instruction_name){
    return instruction_name + "#" + to_hex_string(pc_offset);
}
