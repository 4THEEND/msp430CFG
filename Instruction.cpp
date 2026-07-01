#include "Instruction.hpp"
#include "elf_parser.hpp"

#include <iomanip>

std::string uint16_to_hex_string(uint16_t n){
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
            return uint16_to_hex_string(extension) + "(" + regs_assoc[reg] + ")";
        case AddressingMode::ABSOLUTE_MODE:
            return "&" + uint16_to_hex_string(extension);
        case AddressingMode::INDIRECT_REGISTER_MODE:
            return "@" + regs_assoc[reg];
        case AddressingMode::INDIRECT_AUTOINCREMENT:
            return "@" + regs_assoc[reg] + "+";
        case AddressingMode::IMMEDIATE_MODE:
            return "#" + uint16_to_hex_string(extension);
        case AddressingMode::CONSTANT_MODE0:
            return "#" + std::to_string(0);
        case AddressingMode::CONSTANT_MODE1:
            return "#" + std::to_string(1);
        case AddressingMode::CONSTANT_MODE2:
            return "#" + std::to_string(2);
        case AddressingMode::CONSTANT_MODE4:
            return "#" + std::to_string(4);
        case AddressingMode::CONSTANT_MODE8:
            return "#" + std::to_string(8);
        case AddressingMode::CONSTANT_MODE_NEG1:
            return "#" + std::to_string(-1);
        default:
            return "";
    }
}


uint16_t Instruction::read_memory(uint32_t address, uint8_t* memory, ELFFile& file){
    uint32_t offset = getAddressOffset(file, address);
    uint16_t content = memory[offset + 1];
    content <<= 8;
    content += memory[offset];

    return content;
}


AddressingMode Instruction::parse_mode(uint8_t Am, uint8_t source){
    switch(Am){
        case AS_MODE_0:
            if(source == CG)
                return AddressingMode::CONSTANT_MODE0;
            return AddressingMode::REGISTER_MODE;
        case AS_MODE_1:
            if(source == SR)
                return AddressingMode::ABSOLUTE_MODE;
            else if(source == PC)
                return AddressingMode::SYMBOLIC_MODE;
            else if(source == CG)
                return AddressingMode::CONSTANT_MODE1;
            return  AddressingMode::INDEXED_MODE;
        case AS_MODE_2:
            if(source == CG)
                return AddressingMode::CONSTANT_MODE2;
            else if(source == SR)
                return AddressingMode::CONSTANT_MODE4;
            return AddressingMode::INDIRECT_REGISTER_MODE;
        case AS_MODE_3:
            if(source == PC)
                return AddressingMode::IMMEDIATE_MODE;
            else if(source == CG)
                return AddressingMode::CONSTANT_MODE8;
            return AddressingMode::INDIRECT_AUTOINCREMENT;
        default:
            std::cerr << "Unable to find the right mode\n";
            return AddressingMode::INVALID_MODE;
    }
}

// We've parse_mode(parse_mode) = id lol
uint8_t Instruction::parse_mode(AddressingMode Am, bool is_source){
    switch(Am){
        case AddressingMode::CONSTANT_MODE0:
        case AddressingMode::REGISTER_MODE:
            return AS_MODE_0;
        case AddressingMode::ABSOLUTE_MODE:
        case AddressingMode::SYMBOLIC_MODE:
        case AddressingMode::CONSTANT_MODE1:
        case AddressingMode::INDEXED_MODE:
            return AS_MODE_1;
        case AddressingMode::CONSTANT_MODE2:
        case AddressingMode::CONSTANT_MODE4:
        case AddressingMode::INDIRECT_REGISTER_MODE:
            return AS_MODE_2;
        case AddressingMode::IMMEDIATE_MODE:
        case AddressingMode::CONSTANT_MODE8:
        case AddressingMode::INDIRECT_AUTOINCREMENT:
            return AS_MODE_3;
        default:
            std::cerr << "Unable to find the right mode\n";
            return -1;
    }

}

bool Instruction::should_get_complement(AddressingMode Am){
    return (Am == AddressingMode::INDEXED_MODE)
        || (Am == AddressingMode::SYMBOLIC_MODE)
        || (Am == AddressingMode::ABSOLUTE_MODE)
        || (Am == AddressingMode::IMMEDIATE_MODE);
}


bool Format1Instruction::consume(uint32_t instruction_address, uint8_t* memory, ELFFile& file){
    instruction_length = 1;
    address = instruction_address;
    uint16_t instruction_header = read_memory(instruction_address, memory, file);
    if(instruction_header >> 12 != opcode)
        return false;
    
    source = (instruction_header >> 8) & 0b1111;
    As = parse_mode((instruction_header >> 4) & 0b11, source);
    if(should_get_complement(As)){
        instruction_length++;
        source_complement = read_memory(instruction_address + 2, memory, file);
    }


    destination = instruction_header & 0b1111;
    Ad = parse_mode((instruction_header >> 7) & 1, destination);
    if(should_get_complement(Ad)){
        destination_complement = read_memory(instruction_address + 2 * instruction_length, memory, file);
        instruction_length++;
    }

    byte_instruction = (instruction_header >> 6) & 1;

    if(destination == PC){
        if(As == AddressingMode::INDIRECT_AUTOINCREMENT && source == SP){
            is_ret = true;
        }
        modify_control_flow = true;
        if(As == AddressingMode::IMMEDIATE_MODE 
            || As == AddressingMode::CONSTANT_MODE0
            || As == AddressingMode::CONSTANT_MODE1
            || As == AddressingMode::CONSTANT_MODE2
            || As == AddressingMode::CONSTANT_MODE4
            || As == AddressingMode::CONSTANT_MODE8
            || As == AddressingMode::CONSTANT_MODE_NEG1
        )
            next_addrs = { source_complement };
    } else {
        modify_control_flow = false;
        next_addrs = {instruction_address + 2 * instruction_length };
    }
    return true;
} 


int Format1Instruction::get_instruction_timing(){
    int dadd_penalty = opcode == DADD ? 1 : 0;
    switch(As){
        case AddressingMode::REGISTER_MODE:
        case AddressingMode::CONSTANT_MODE0:
        case AddressingMode::CONSTANT_MODE1:
        case AddressingMode::CONSTANT_MODE2:
        case AddressingMode::CONSTANT_MODE4:
        case AddressingMode::CONSTANT_MODE8:
        case AddressingMode::CONSTANT_MODE_NEG1:
            if(Ad == AddressingMode::REGISTER_MODE 
                || Ad == AddressingMode::CONSTANT_MODE0 
                || Ad == AddressingMode::CONSTANT_MODE1 
                || Ad == AddressingMode::CONSTANT_MODE2 
                || Ad == AddressingMode::CONSTANT_MODE4 
                || Ad == AddressingMode::CONSTANT_MODE8 
                || Ad == AddressingMode::CONSTANT_MODE_NEG1){
                if(destination == PC)
                    return 2 + dadd_penalty;
                return 1 + dadd_penalty;
            }
            else if(Ad == AddressingMode::INDEXED_MODE || Ad == AddressingMode::SYMBOLIC_MODE || Ad == AddressingMode::ABSOLUTE_MODE)
                return 4 + get_rom_penalty() + dadd_penalty;
        case AddressingMode::INDEXED_MODE:
        case AddressingMode::SYMBOLIC_MODE:
        case AddressingMode::ABSOLUTE_MODE:
            if(Ad == AddressingMode::REGISTER_MODE){
                if(destination == PC)
                    return 4 + dadd_penalty;
                return 3 + dadd_penalty;
            }
            else if(Ad == AddressingMode::INDEXED_MODE || Ad == AddressingMode::SYMBOLIC_MODE || Ad == AddressingMode::ABSOLUTE_MODE)
                return 6 + get_rom_penalty() + dadd_penalty;
        case AddressingMode::INDIRECT_REGISTER_MODE:
        case AddressingMode::INDIRECT_AUTOINCREMENT:
        case  AddressingMode::IMMEDIATE_MODE:
            if(Ad == AddressingMode::REGISTER_MODE){
                if(destination == PC)
                    return 3 + dadd_penalty;
                return 2 + dadd_penalty;
            }
            else if(Ad == AddressingMode::INDEXED_MODE || Ad == AddressingMode::SYMBOLIC_MODE || Ad == AddressingMode::ABSOLUTE_MODE)
                return 5 + get_rom_penalty() + dadd_penalty;
    }  
    return 0;
}


std::array<uint16_t, 3> Format1Instruction::get_instruction(){
    uint16_t header{};

    header += opcode << 12;
    header += (source & 0b1111) << 8;
    header += (parse_mode(Ad, false) & 1) << 7;
    header += (byte_instruction & 1) << 6;
    header += (parse_mode(As) & 0b11) << 4;
    header += destination & 0b1111;

    if(should_get_complement(As)){
        if(should_get_complement(Ad))
            return {header, source_complement, destination_complement};
        return {header, source_complement, 0};
    }
    else if(should_get_complement(Ad))
        return {header, destination_complement, 0};

    return {header, 0, 0};
}


std::string Format1Instruction::abstractGetString(std::string instruction_name){
    return instruction_name 
        + (byte_instruction ? ".b " : " ") 
        + decorate_reg(source, As, source_complement) 
        + ", " 
        + decorate_reg(destination, Ad, destination_complement);
}


bool Format2Instruction::consume(uint32_t instruction_address, uint8_t* memory, ELFFile& file) {
    uint16_t instruction_header = read_memory(instruction_address, memory, file);
    instruction_length = 1;
    address = instruction_address;
    modify_control_flow = (opcode == CALL || opcode == RETI ? true : false);
    if((instruction_header >> 10 != FORMAT_2_PREFIX) || (((instruction_header >> 7) & 0b111) != opcode))
        return false;

    source = instruction_header & 0b1111;
    As = parse_mode((instruction_header >> 4) & 0b11, source);
    if(should_get_complement(As)){
        instruction_length++;
        source_complement = read_memory(instruction_address + 2, memory, file);
    }

    byte_instruction = (instruction_header >> 6) & 1;
    if(opcode == RETI)
        next_addrs = {};
    else if(opcode != CALL)
        next_addrs = {instruction_address + 2 * instruction_length };
    else {
        is_call = true;
        if(As == AddressingMode::IMMEDIATE_MODE 
            || As == AddressingMode::CONSTANT_MODE0
            || As == AddressingMode::CONSTANT_MODE1
            || As == AddressingMode::CONSTANT_MODE2
            || As == AddressingMode::CONSTANT_MODE4
            || As == AddressingMode::CONSTANT_MODE8
            || As == AddressingMode::CONSTANT_MODE_NEG1
        )
            next_addrs = { source_complement };
    }
    return true;
}


int Format2Instruction::get_instruction_timing(){
    switch(As){
        case AddressingMode::REGISTER_MODE:
        case AddressingMode::CONSTANT_MODE0: 
        case AddressingMode::CONSTANT_MODE1:
        case AddressingMode::CONSTANT_MODE2: 
        case AddressingMode::CONSTANT_MODE4: 
        case AddressingMode::CONSTANT_MODE8: 
        case AddressingMode::CONSTANT_MODE_NEG1:
            if(opcode == PUSH)
                return 3 + get_rom_penalty();
            else if(opcode == CALL)
                // 4 on MSP430
                return 3 + get_rom_penalty();
            return 1;

        case AddressingMode::INDEXED_MODE: 
        case AddressingMode::SYMBOLIC_MODE:
        case AddressingMode::ABSOLUTE_MODE:
            if(opcode == PUSH)
                return 5 + get_rom_penalty();
            else if(opcode == CALL)
                        return 5 + get_rom_penalty();
            return 4 + get_rom_penalty();
        
        
        case AddressingMode::INDIRECT_REGISTER_MODE:
            if(opcode == PUSH)
                return 4 + get_rom_penalty();
            else if(opcode == CALL)
                return 4 + get_rom_penalty();
            return 3 + get_rom_penalty();
        
        case AddressingMode::INDIRECT_AUTOINCREMENT:
            if(opcode == PUSH)
                // 5 on MSP430
                return 4 + get_rom_penalty();
            else if(opcode == CALL)
                // 5 on MSP430
                return 4 + get_rom_penalty();
            return 3 + get_rom_penalty();
        
        case AddressingMode::IMMEDIATE_MODE:
            if(opcode == PUSH)
                return 4 + get_rom_penalty();
            else if(opcode == CALL)
                return 4 + get_rom_penalty();
    }

    return 0;
}


std::array<uint16_t, 3> Format2Instruction::get_instruction(){
    uint16_t header{};

    header += FORMAT_2_PREFIX << 10;
    header += (opcode & 0b111) << 7;
    header += (byte_instruction & 1) << 6;
    header += (parse_mode(As) & 0b11) << 4;
    header += source & 0b1111;

    if(should_get_complement(As))
        return {header, source_complement, 0};
        
    return {header, 0, 0};
}


std::string Format2Instruction::abstractGetString(std::string instruction_name){
    return instruction_name 
        + (byte_instruction ? ".b " : " ") 
        + decorate_reg(source, As, source_complement);
}


int16_t sign_extend(int16_t addr){
    if(addr & 0b1000000000){
        addr |= 0b1111110000000000;
    }
    return addr;
}


bool Format3Instruction::consume(uint32_t instruction_address, uint8_t* memory, ELFFile& file){
    uint16_t instruction_header = read_memory(instruction_address, memory, file);
    instruction_length = 1;
    address = instruction_address;
    modify_control_flow = true;
    if((instruction_header >> 13 != FORMAT_3_PREFIX) || (((instruction_header >> 10) & 0b111) != condition))
        return false;

    pc_offset = sign_extend(instruction_header & 0b1111111111);
    next_addrs = {instruction_address + 2 + 2 * pc_offset};
    if(condition != JMP)
        next_addrs.emplace_back(instruction_address + 2 * instruction_length);
    return true;
}


int Format3Instruction::get_instruction_timing(){
    return 2;
}


std::array<uint16_t, 3> Format3Instruction::get_instruction(){
    return {(uint16_t)((FORMAT_3_PREFIX << 13) + (condition << 10) + (pc_offset & 0b1111111111)), 0, 0};
}


std::string Format3Instruction::abstractGetString(std::string instruction_name){
    return instruction_name + " $" + std::to_string(2 + 2 * pc_offset);
}
