#include "Instruction.hpp"
#include "binary_loader.hpp"

#include <iomanip>

std::string uint16_to_hex_string(uint16_t n){
    std::stringstream stream;
    stream << "0x" << std::hex << n << std::dec;
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
            else if(source == SR)
                return AddressingMode::CONSTANT_MODE8;
            else if(source == CG)
                return AddressingMode::CONSTANT_MODE_NEG1;
            return AddressingMode::INDIRECT_AUTOINCREMENT;
        default:
            std::cerr << "Unable to find the right mode\n";
            return AddressingMode::INVALID_MODE;
    }
}

// We've parse_mode(parse_mode) = id lol
uint8_t Instruction::parse_mode(AddressingMode Am){
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
        case AddressingMode::CONSTANT_MODE_NEG1:
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


bool Format1Instruction::consume(uint32_t instruction_address, std::shared_ptr<BinaryLoader> binary_file){
    instruction_length = 1;
    address = instruction_address;
    uint16_t instruction_header;
    try{
        instruction_header = binary_file->read_memory(instruction_address);
    } catch (int exc){
        return false;
    }

    
    if(instruction_header >> 12 != opcode)
        return false;
    
    source = (instruction_header >> 8) & 0b1111;
    As = parse_mode((instruction_header >> 4) & 0b11, source);
    if(should_get_complement(As)){
        instruction_length++;
        
        try{
            source_complement = binary_file->read_memory(instruction_address + 2);
        } catch (int exc){
            return false;
        }
    }


    destination = instruction_header & 0b1111;
    Ad = parse_mode((instruction_header >> 7) & 1, destination);
    if(should_get_complement(Ad)){
        try{
            destination_complement = binary_file->read_memory(instruction_address + 2 * instruction_length);
        } catch (int exc){
            return false;
        }

        instruction_length++;
    }

    byte_instruction = (instruction_header >> 6) & 1;

    if(destination == PC && opcode == MOV){
        if(As == AddressingMode::INDIRECT_AUTOINCREMENT && source == SP)
            is_ret = true;
        modify_control_flow = true;
    } 
    else {
        modify_control_flow = false;
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
    header += (parse_mode(Ad) & 1) << 7;
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


bool Format2Instruction::consume(uint32_t instruction_address, std::shared_ptr<BinaryLoader> binary_file) {
    uint16_t instruction_header;
    try{
        instruction_header = binary_file->read_memory(instruction_address);
    } catch (int exc){
        return false;
    }

    instruction_length = 1;
    address = instruction_address;
    modify_control_flow = (opcode == CALL || opcode == RETI ? true : false);
    is_call = opcode == CALL;

    if((instruction_header >> 10 != FORMAT_2_PREFIX) || (((instruction_header >> 7) & 0b111) != opcode))
        return false;

    source = instruction_header & 0b1111;
    As = parse_mode((instruction_header >> 4) & 0b11, source);
    if(should_get_complement(As)){
        instruction_length++;

        try{
            source_complement = binary_file->read_memory(instruction_address + 2);
        } catch (int exc){
            return false;
        }
    }

    byte_instruction = (instruction_header >> 6) & 1;

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


bool Format3Instruction::consume(uint32_t instruction_address, std::shared_ptr<BinaryLoader> binary_file){
    uint16_t instruction_header;
    try{
        instruction_header = binary_file->read_memory(instruction_address);
    } catch (int exc){
        return false;
    }

    instruction_length = 1;
    address = instruction_address;
    modify_control_flow = true;
    if((instruction_header >> 13 != FORMAT_3_PREFIX) || (((instruction_header >> 10) & 0b111) != condition))
        return false;

    pc_offset = sign_extend(instruction_header & 0b1111111111);
    return true;
}


int Format3Instruction::get_instruction_timing(){
    return 2;
}


std::array<uint16_t, 3> Format3Instruction::get_instruction(){
    return {(uint16_t)((FORMAT_3_PREFIX << 13) + (condition << 10) + (pc_offset & 0b1111111111)), 0, 0};
}


std::set<uint32_t> Format3Instruction::get_next_addrs(State &state)
{
    std::set<uint32_t> next_addrs = {address + 2 + 2 * pc_offset};
    if(condition != JMP)
        next_addrs.insert(address + 2 * instruction_length);

    return next_addrs;
}


std::string Format3Instruction::abstractGetString(std::string instruction_name){
    return instruction_name + " $" + std::to_string(2 + 2 * pc_offset);
}


std::optional<uint16_t> parse_parameter(uint8_t parameter, AddressingMode am, uint16_t parameter_extension, State& state, bool byte){
    switch(am){
        case AddressingMode::CONSTANT_MODE0:
            return std::make_optional(0);
        case AddressingMode::CONSTANT_MODE1:
            return std::make_optional(1);
        case AddressingMode::CONSTANT_MODE2:
            return std::make_optional(2);
        case AddressingMode::CONSTANT_MODE4:
            return std::make_optional(4);
        case AddressingMode::CONSTANT_MODE8:
            return std::make_optional(8);
        case AddressingMode::CONSTANT_MODE_NEG1:
            return std::make_optional(-1);
        case AddressingMode::REGISTER_MODE:
            return state.read_register(parameter, byte);
        case AddressingMode::INDEXED_MODE:
        case AddressingMode::SYMBOLIC_MODE:
            return state.read_memory(state.read_register(parameter) + parameter_extension, byte);
        case AddressingMode::ABSOLUTE_MODE:
            return state.read_memory(parameter_extension, byte);
        case AddressingMode::INDIRECT_REGISTER_MODE:
            return state.read_memory(state.read_register(parameter), byte);
        case AddressingMode::INDIRECT_AUTOINCREMENT:
            return state.read_memory(state.read_register(parameter), byte);
            // Warning: we also do the increment here
            state.write_register(parameter, state.read_register(parameter) + 1);
        case AddressingMode::IMMEDIATE_MODE:
            return std::make_optional(parameter_extension);
        default:
            std::cerr << "Unable to find the right mode\n";
            return -1;
    }
}


std::pair<bool, std::optional<uint16_t>> parse_destination(uint8_t parameter, AddressingMode am, uint16_t parameter_extension, State& state){
    switch(am){
        case AddressingMode::REGISTER_MODE:
            return {false, parameter};
        case AddressingMode::INDEXED_MODE:
        case AddressingMode::SYMBOLIC_MODE:
            return {true, state.read_register(parameter) + parameter_extension};
        case AddressingMode::ABSOLUTE_MODE:
            return {true, parameter_extension};
        case AddressingMode::INDIRECT_REGISTER_MODE:
        case AddressingMode::INDIRECT_AUTOINCREMENT:
        case AddressingMode::IMMEDIATE_MODE:
        case AddressingMode::CONSTANT_MODE0:
        case AddressingMode::CONSTANT_MODE1:
        case AddressingMode::CONSTANT_MODE2:
        case AddressingMode::CONSTANT_MODE4:
        case AddressingMode::CONSTANT_MODE8:
        case AddressingMode::CONSTANT_MODE_NEG1:
        default:
            std::cerr << "Unable to find the right mode\n";
            return {false, -1};
    }
}


std::pair<bool, std::optional<uint32_t>> parse_parameters_wrapper(uint8_t parameter, AddressingMode am, uint16_t parameter_extension, State& state, bool byte){
    std::optional<uint32_t> ret_value{};
    try{
        ret_value = parse_parameter(parameter, am, parameter_extension, state, byte);
    } catch (int exc){
        state.clear_gp_registers();
        try {
            ret_value = parse_parameter(parameter, am, parameter_extension, state, byte);
        } catch (int snd) {
            return std::make_pair(false, ret_value);
        }
    }

    return std::make_pair(true, ret_value);
}


std::set<uint32_t> MOVInstruction::get_next_addrs(State& state){
    auto [is_valid, p_source] = parse_parameters_wrapper(source, As, source_complement, state, byte_instruction);
    if(!is_valid)
        return {};

    auto [is_memory, location] = parse_destination(destination, Ad, destination_complement, state);

    if(is_memory)
        state.write_memory(location, p_source, byte_instruction);
    else 
        state.write_register(location.value(), p_source, byte_instruction);
    
    if(Ad == AddressingMode::REGISTER_MODE && destination == PC && p_source)
        return { p_source.value() };

    return {address + 2 * instruction_length };
}


std::set<uint32_t> ADDInstruction::get_next_addrs(State &state)
{
    auto [is_valid, p_source] = parse_parameters_wrapper(source, As, source_complement, state, byte_instruction);
    auto [is_valid_dest, p_destination] = parse_parameters_wrapper(destination, Ad, destination_complement, state, byte_instruction);
    if(!is_valid || !is_valid_dest)
        return {};
        
    auto [is_memory, location] = parse_destination(destination, Ad, destination_complement, state);

    std::optional<uint16_t> addition = p_source + p_destination;
    if(is_memory)
        state.write_memory(location, addition, byte_instruction);
    else 
        state.write_register(location.value(), addition, byte_instruction);

    if(Ad == AddressingMode::REGISTER_MODE && destination == PC && addition)
        return { addition.value() };

    return {address + 2 * instruction_length };    
}


std::set<uint32_t> ADDCInstruction::get_next_addrs(State &state)
{
    // TODO: Manage carry bit
    auto [is_memory, location] = parse_destination(destination, Ad, destination_complement, state);
    if(is_memory)
        state.write_memory(location, std::nullopt, byte_instruction);
    else 
        state.write_register(location.value(), std::nullopt, byte_instruction);

    return {address + 2 * instruction_length };
}


std::set<uint32_t> SUBCInstruction::get_next_addrs(State &state)
{
    // TODO: Manage carry bit
    auto [is_memory, location] = parse_destination(destination, Ad, destination_complement, state);
    if(is_memory)
        state.write_memory(location, std::nullopt, byte_instruction);
    else 
        state.write_register(location.value(), std::nullopt, byte_instruction);

    return {address + 2 * instruction_length };
}


std::set<uint32_t> SUBInstruction::get_next_addrs(State &state)
{
    auto [is_valid, p_source] = parse_parameters_wrapper(source, As, source_complement, state, byte_instruction);
    auto [is_valid_dest, p_destination] = parse_parameters_wrapper(destination, Ad, destination_complement, state, byte_instruction);
    if(!is_valid || !is_valid_dest)
        return {};
        
    auto [is_memory, location] = parse_destination(destination, Ad, destination_complement, state);

    std::optional<uint16_t> substraction = p_source - p_destination;
    if(is_memory)
        state.write_memory(location, substraction, byte_instruction);
    else 
        state.write_register(location.value(), substraction, byte_instruction);

    if(Ad == AddressingMode::REGISTER_MODE && destination == PC && substraction)
        return { substraction.value() };

    return {address + 2 * instruction_length };  
}


std::set<uint32_t> DADDInstruction::get_next_addrs(State &state)
{
    // TODO: Manage carry bit
    auto [is_memory, location] = parse_destination(destination, Ad, destination_complement, state);
    if(is_memory)
        state.write_memory(location, std::nullopt, byte_instruction);
    else 
        state.write_register(location.value(), std::nullopt, byte_instruction);

    return {address + 2 * instruction_length };
}


std::set<uint32_t> BICInstruction::get_next_addrs(State &state)
{
    auto [is_valid, p_source] = parse_parameters_wrapper(source, As, source_complement, state, byte_instruction);
    auto [is_valid_dest, p_destination] = parse_parameters_wrapper(destination, Ad, destination_complement, state, byte_instruction);
    if(!is_valid || !is_valid_dest)
        return {};
        
    auto [is_memory, location] = parse_destination(destination, Ad, destination_complement, state);

    std::optional<uint16_t> bic_result = op_not(p_source) & p_destination;
    if(is_memory)
        state.write_memory(location, bic_result, byte_instruction);
    else 
        state.write_register(location.value(), bic_result, byte_instruction);

    if(Ad == AddressingMode::REGISTER_MODE && destination == PC && bic_result)
        return { bic_result.value() };

    return {address + 2 * instruction_length };  
}


std::set<uint32_t> BISInstruction::get_next_addrs(State &state)
{
    auto [is_valid, p_source] = parse_parameters_wrapper(source, As, source_complement, state, byte_instruction);
    auto [is_valid_dest, p_destination] = parse_parameters_wrapper(destination, Ad, destination_complement, state, byte_instruction);
    if(!is_valid || !is_valid_dest)
        return {};
        
    auto [is_memory, location] = parse_destination(destination, Ad, destination_complement, state);

    std::optional<uint16_t> bis_result = p_source | p_destination;
    if(is_memory)
        state.write_memory(location, bis_result, byte_instruction);
    else 
        state.write_register(location.value(), bis_result, byte_instruction);

    if(Ad == AddressingMode::REGISTER_MODE && destination == PC && bis_result)
        return { bis_result.value() };

    return {address + 2 * instruction_length };
}


std::set<uint32_t> XORInstruction::get_next_addrs(State &state)
{
    auto [is_valid, p_source] = parse_parameters_wrapper(source, As, source_complement, state, byte_instruction);
    auto [is_valid_dest, p_destination] = parse_parameters_wrapper(destination, Ad, destination_complement, state, byte_instruction);
    if(!is_valid || !is_valid_dest)
        return {};
        
    auto [is_memory, location] = parse_destination(destination, Ad, destination_complement, state);

    std::optional<uint16_t> xor_result = p_source ^ p_destination;
    if(is_memory)
        state.write_memory(location, xor_result, byte_instruction);
    else 
        state.write_register(location.value(), xor_result, byte_instruction);

    if(Ad == AddressingMode::REGISTER_MODE && destination == PC && xor_result)
        return { xor_result.value() };

    return {address + 2 * instruction_length };  
}


std::set<uint32_t> ANDInstruction::get_next_addrs(State &state)
{
    auto [is_valid, p_source] = parse_parameters_wrapper(source, As, source_complement, state, byte_instruction);
    auto [is_valid_dest, p_destination] = parse_parameters_wrapper(destination, Ad, destination_complement, state, byte_instruction);
    if(!is_valid || !is_valid_dest)
        return {};
        
    auto [is_memory, location] = parse_destination(destination, Ad, destination_complement, state);

    std::optional<uint16_t> and_result = p_source & p_destination;
    if(is_memory)
        state.write_memory(location, and_result, byte_instruction);
    else 
        state.write_register(location.value(), and_result, byte_instruction);

    if(Ad == AddressingMode::REGISTER_MODE && destination == PC && and_result)
        return { and_result.value() };

    return {address + 2 * instruction_length };  
}


std::set<uint32_t> RRCInstruction::get_next_addrs(State &state)
{
    // TODO: Manage carry bit
    auto [is_memory, location] = parse_destination(source, As, source_complement, state);
    if(is_memory)
        state.write_memory(location, std::nullopt, byte_instruction);
    else 
        state.write_register(location.value(), std::nullopt, byte_instruction);

    return {address + 2 * instruction_length };
}


std::set<uint32_t> SWPBInstruction::get_next_addrs(State &state)
{
    auto [is_valid, p_source] = parse_parameters_wrapper(source, As, source_complement, state, byte_instruction);
    if(!is_valid)
        return {};
        
    auto [is_memory, location] = parse_destination(source, As, source, state);

    std::optional<uint16_t> swap_result = op_swpb(p_source);
    if(is_memory)
        state.write_memory(location, swap_result, byte_instruction);
    else 
        state.write_register(location.value(), swap_result, byte_instruction);

    if(As == AddressingMode::REGISTER_MODE && source == PC && swap_result)
        return { swap_result.value() };

    return {address + 2 * instruction_length }; 
}


std::set<uint32_t> RRAInstruction::get_next_addrs(State &state)
{
    auto [is_valid, p_source] = parse_parameters_wrapper(source, As, source_complement, state, byte_instruction);
    if(!is_valid)
        return {};
        
    auto [is_memory, location] = parse_destination(source, As, source, state);

    std::optional<uint16_t> shift_result = op_arith_right_shift(p_source);
    if(is_memory)
        state.write_memory(location, shift_result, byte_instruction);
    else 
        state.write_register(location.value(), shift_result, byte_instruction);

    if(As == AddressingMode::REGISTER_MODE && source == PC && shift_result)
        return { shift_result.value() };

    return {address + 2 * instruction_length }; 
}


std::set<uint32_t> SXTInstruction::get_next_addrs(State &state)
{
    auto [is_valid, p_source] = parse_parameters_wrapper(source, As, source_complement, state, byte_instruction);
    if(!is_valid)
        return {};
        
    auto [is_memory, location] = parse_destination(source, As, source, state);

    std::optional<uint16_t> extend_result = op_sxt(p_source);
    if(is_memory)
        state.write_memory(location, extend_result, byte_instruction);
    else 
        state.write_register(location.value(), extend_result, byte_instruction);

    if(As == AddressingMode::REGISTER_MODE && source == PC && extend_result)
        return { extend_result.value() };

    return {address + 2 * instruction_length }; 
}


std::set<uint32_t> PUSHInstruction::get_next_addrs(State &state)
{
    auto [is_valid, p_source] = parse_parameters_wrapper(source, As, source_complement, state, byte_instruction);
    if(!is_valid)
        return {};

    std::optional<uint16_t> sp = state.read_register(SP);
    state.write_memory(sp, p_source);
    state.write_register(SP, sp - 1);

    return {address + 2 * instruction_length }; 
}


std::set<uint32_t> CALLInstruction::get_next_addrs(State &state)
{
    auto [is_valid, p_source] = parse_parameters_wrapper(source, As, source_complement, state, byte_instruction);
    if(!is_valid)
        return {};
    
    std::optional<uint16_t> sp = state.read_register(SP);
    state.write_memory(sp, p_source);
    state.write_register(SP, sp - 1);

    if(p_source)
        return { p_source.value() };
    return {};
}
