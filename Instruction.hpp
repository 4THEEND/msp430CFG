#pragma once

#include "elf_parser.hpp"

#include <inttypes.h>
#include <iostream>
#include <string>
#include <functional>
#include <vector>
#include <array>
#include <memory>


// Two operands instruction's opcodes
#define MOV     0b0100
#define ADD     0b0101
#define ADDC    0b0110
#define SUBC    0b0111
#define SUB     0b1000
#define CMP     0b1001
#define DADD    0b1010
#define BIT     0b1011
#define BIC     0b1100
#define BIS     0b1101
#define XOR     0b1110
#define AND     0b1111

// One operand instruction's opcodes
#define FORMAT_2_PREFIX 0b000100
#define RRC     0b000
#define SWPB    0b001
#define RRA     0b010
#define SXT     0b011
#define PUSH    0b100
#define CALL    0b101
#define RETI    0b110

// Jump type two instructions conditions
#define FORMAT_3_PREFIX 0b001
#define JNZ     0b000
#define JZ      0b001
#define JLO     0b010
#define JHS     0b011
#define JN      0b100
#define JGE     0b101
#define JL      0b110
#define JMP     0b111

// Addressing modes
#define AS_MODE_0   0b00
#define AS_MODE_1   0b01 
#define AS_MODE_2   0b10
#define AS_MODE_3   0b11

#define AD_MODE_0    0
#define AD_MODE_1    1

enum class AddressingMode {
    REGISTER_MODE,
    INDEXED_MODE,
    SYMBOLIC_MODE,
    ABSOLUTE_MODE,
    INDIRECT_REGISTER_MODE,
    INDIRECT_AUTOINCREMENT,
    IMMEDIATE_MODE,

    CONSTANT_MODE0,
    CONSTANT_MODE1,
    CONSTANT_MODE2,
    CONSTANT_MODE4,
    CONSTANT_MODE8,
    CONSTANT_MODE_NEG1,

    INVALID_MODE
};


// Registers
enum RegisterEnum {
    PC,
    SP,
    SR,
    CG,
    R4,
    R5,
    R6,
    R7,
    R8,
    R9,
    R10,
    R11,
    R12,
    R13,
    R14,
    R15
};


class Instruction {
public:
    std::array<std::string, 16> regs_assoc{
        "pc", "sp", "sr", "zero", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"
    };
    uint32_t address;
    //length of instruction in words
    int instruction_length;
    bool is_ret = false;
    bool is_call = false;
    bool modify_control_flow;
    std::vector<uint32_t> next_addrs;
    
public:
    virtual std::string getString() = 0;
    virtual bool consume(uint32_t instruction_address, uint8_t* memory, ELFFile& file) = 0;

    friend std::ostream& operator<<(std::ostream& os, Instruction& instr){
        os << instr.getString();
        return os;
    };

protected:
    uint16_t read_memory(uint32_t address, uint8_t* memory, ELFFile& file);
    
    bool should_get_complement(AddressingMode Am);
    AddressingMode parse_mode(uint8_t Am, uint8_t source);
    AddressingMode parse_mode(AddressingMode Am, uint16_t immediate);

    std::string decorate_reg(uint8_t reg, AddressingMode Am, uint16_t source_extension);
};


class Format1Instruction : public Instruction {
protected:
    uint8_t opcode;

    AddressingMode As;
    uint8_t source;
    uint16_t source_complement;

    AddressingMode Ad;
    uint8_t destination;
    uint16_t destination_complement;

    bool byte_instruction;
    
public:
    Format1Instruction(uint8_t op) : opcode(op) {};
    bool consume(uint32_t instruction_address, uint8_t* memory, ELFFile& file) override;

protected:
    std::string abstractGetString(std::string instruction_name);
};


class MOVInstruction : public Format1Instruction {
public:
    MOVInstruction() : Format1Instruction(MOV) {};

    std::string getString() {
        if(is_ret)
            return "ret";
        return abstractGetString("mov");
    }
};

class ADDInstruction : public Format1Instruction {
public:
    ADDInstruction() : Format1Instruction(ADD) {};

    std::string getString() {
        return abstractGetString("add");
    }
};

class ADDCInstruction : public Format1Instruction {
public:
    ADDCInstruction() : Format1Instruction(ADDC) {};

    std::string getString() {
        return abstractGetString("addc");
    }
};

class SUBCInstruction : public Format1Instruction {
public:
    SUBCInstruction() : Format1Instruction(SUBC) {};

    std::string getString() {
        return abstractGetString("subc");
    }
};

class SUBInstruction : public Format1Instruction {
public:
    SUBInstruction() : Format1Instruction(SUB) {};

    std::string getString() {
        return abstractGetString("sub");
    }
};

class CMPInstruction : public Format1Instruction {
public:
    CMPInstruction() : Format1Instruction(CMP) {};

    std::string getString() {
        return abstractGetString("cmp");
    }
};

class DADDInstruction : public Format1Instruction {
public:
    DADDInstruction() : Format1Instruction(DADD) {};

    std::string getString() {
        return abstractGetString("dadd");
    }
};

class BITInstruction : public Format1Instruction {
public:
    BITInstruction() : Format1Instruction(BIT) {};

    std::string getString() {
        return abstractGetString("bit");
    }
};

class BICInstruction : public Format1Instruction {
public:
    BICInstruction() : Format1Instruction(BIC) {};

    std::string getString() {
        return abstractGetString("bic");
    }
};

class BISInstruction : public Format1Instruction {
public:
    BISInstruction() : Format1Instruction(BIS) {};

    std::string getString() {
        return abstractGetString("bis");
    }
};

class XORInstruction : public Format1Instruction {
public:
    XORInstruction() : Format1Instruction(XOR) {};

    std::string getString() {
        return abstractGetString("xor");
    }
};

class ANDInstruction : public Format1Instruction {
public:
    ANDInstruction() : Format1Instruction(AND) {};

    std::string getString() {
        return abstractGetString("and");
    }
};


class Format2Instruction : public Instruction {
protected:
    uint8_t opcode;
    bool byte_instruction;

    AddressingMode As;
    uint8_t source;
    uint16_t source_complement;

public:
    bool consume(uint32_t instruction_address, uint8_t* memory, ELFFile& file) override;

protected:
    Format2Instruction(uint8_t op) : opcode(op) {};
    std::string abstractGetString(std::string instruction_name);
};

class RRCInstruction : public Format2Instruction {
public:
    RRCInstruction() : Format2Instruction(RRC) {};

    std::string getString() {
        return abstractGetString("rrc");
    }
};

class SWPBInstruction : public Format2Instruction {
public:
    SWPBInstruction() : Format2Instruction(SWPB) {};

    std::string getString() {
        return abstractGetString("swpb");
    }
};

class RRAInstruction : public Format2Instruction {
public:
    RRAInstruction() : Format2Instruction(RRA) {};

    std::string getString() {
        return abstractGetString("rra");
    }
};

class SXTInstruction : public Format2Instruction {
public:
    SXTInstruction() : Format2Instruction(SXT) {};

    std::string getString() {
        return abstractGetString("sxt");
    }
};

class PUSHInstruction : public Format2Instruction {
public:
    PUSHInstruction() : Format2Instruction(PUSH) {};

    std::string getString() {
        return abstractGetString("push");
    }
};

class CALLInstruction : public Format2Instruction {
public:
    CALLInstruction() : Format2Instruction(CALL) {};

    std::string getString() {
        return abstractGetString("call");
    }
};

class RETIInstruction : public Format2Instruction {
public:
    RETIInstruction() : Format2Instruction(RETI) {};

    std::string getString() {
        return abstractGetString("reti");
    }
};


class Format3Instruction : public Instruction {
private:
    uint8_t condition;
    int16_t pc_offset;
public:
    Format3Instruction(uint8_t c) : condition(c) {};
    bool consume(uint32_t instruction_address, uint8_t* memory, ELFFile& file) override;

protected:
    std::string abstractGetString(std::string instruction_name);
};

class JNZInstruction : public Format3Instruction {
public:
    JNZInstruction() : Format3Instruction(JNZ) {};

    std::string getString() {
        return abstractGetString("jnz");
    }
};

class JZInstruction : public Format3Instruction {
public:
    JZInstruction() : Format3Instruction(JZ) {};

    std::string getString() {
        return abstractGetString("jz");
    }
};

class JLOInstruction : public Format3Instruction {
public:
    JLOInstruction() : Format3Instruction(JLO) {};

    std::string getString() {
        return abstractGetString("jlo");
    }
};

class JHSInstruction : public Format3Instruction {
public:
    JHSInstruction() : Format3Instruction(JHS) {};

    std::string getString() {
        return abstractGetString("jhs");
    }
};

class JNInstruction : public Format3Instruction {
public:
    JNInstruction() : Format3Instruction(JN) {};

    std::string getString() {
        return abstractGetString("jn");
    }
};

class JGEInstruction : public Format3Instruction {
public:
    JGEInstruction() : Format3Instruction(JGE) {};

    std::string getString() {
        return abstractGetString("jge");
    }
};

class JLInstruction : public Format3Instruction {
public:
    JLInstruction() : Format3Instruction(JL) {};

    std::string getString() {
        return abstractGetString("jl");
    }
};

class JMPInstruction : public Format3Instruction {
public:
    JMPInstruction() : Format3Instruction(JMP) {};

    std::string getString() {
        return abstractGetString("jmp");
    }
};


class InstructionFactory {
public:
    typedef std::function<std::shared_ptr<Instruction>()> Builder;

    int Register(Builder const& builder) {
        instruction_types.emplace_back(builder);
        return instruction_types.size();
    }

    std::shared_ptr<Instruction> Build(uint32_t instruction_address, uint8_t* memory, ELFFile& file) const {
        for(const Builder& b: instruction_types){
            std::shared_ptr<Instruction> inst = b();
            if(inst->consume(instruction_address, memory, file)){
                return inst;
            }
        }
        return nullptr;
  }

private:
  std::vector<Builder> instruction_types;
};

inline InstructionFactory& GetInstructionFactory() { static InstructionFactory F; return F; }

template <typename Derived>
std::shared_ptr<Instruction> instructionBuilder() { return std::make_shared<Derived>(); }


static const int i_mov = GetInstructionFactory().Register(instructionBuilder<MOVInstruction>);
static const int i_add = GetInstructionFactory().Register(instructionBuilder<ADDInstruction>);
static const int i_addc = GetInstructionFactory().Register(instructionBuilder<ADDCInstruction>);
static const int i_subc = GetInstructionFactory().Register(instructionBuilder<SUBCInstruction>);
static const int i_sub = GetInstructionFactory().Register(instructionBuilder<SUBInstruction>);
static const int i_cmp = GetInstructionFactory().Register(instructionBuilder<CMPInstruction>);
static const int i_dadd = GetInstructionFactory().Register(instructionBuilder<DADDInstruction>);
static const int i_bic = GetInstructionFactory().Register(instructionBuilder<BICInstruction>);
static const int i_bit = GetInstructionFactory().Register(instructionBuilder<BITInstruction>);
static const int i_bis = GetInstructionFactory().Register(instructionBuilder<BISInstruction>);
static const int i_xor = GetInstructionFactory().Register(instructionBuilder<XORInstruction>);
static const int i_and = GetInstructionFactory().Register(instructionBuilder<ANDInstruction>);

static const int i_rrc = GetInstructionFactory().Register(instructionBuilder<RRCInstruction>);
static const int i_swpb = GetInstructionFactory().Register(instructionBuilder<SWPBInstruction>);
static const int i_rra = GetInstructionFactory().Register(instructionBuilder<RRAInstruction>);
static const int i_sxt = GetInstructionFactory().Register(instructionBuilder<SXTInstruction>);
static const int i_push = GetInstructionFactory().Register(instructionBuilder<PUSHInstruction>);
static const int i_call = GetInstructionFactory().Register(instructionBuilder<CALLInstruction>);
static const int i_reti = GetInstructionFactory().Register(instructionBuilder<RETIInstruction>);

static const int i_jnz = GetInstructionFactory().Register(instructionBuilder<JNZInstruction>);
static const int i_jz = GetInstructionFactory().Register(instructionBuilder<JZInstruction>);
static const int i_jlo = GetInstructionFactory().Register(instructionBuilder<JLOInstruction>);
static const int i_jhs = GetInstructionFactory().Register(instructionBuilder<JHSInstruction>);
static const int i_jn = GetInstructionFactory().Register(instructionBuilder<JNInstruction>);
static const int i_jge = GetInstructionFactory().Register(instructionBuilder<JGEInstruction>);
static const int i_jl = GetInstructionFactory().Register(instructionBuilder<JLInstruction>);
static const int i_jmp = GetInstructionFactory().Register(instructionBuilder<JMPInstruction>);
