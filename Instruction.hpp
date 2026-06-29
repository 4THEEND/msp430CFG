#pragma once

#include <inttypes.h>
#include <iostream>
#include <functional>
#include <vector>


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
#define AS_REGISTER_MODE            0b00
#define AS_INDEXED_MODE             0b01
#define AS_SYMBOLIC_MODE            0b01
#define AS_ABSOLUTE_MODE            0b01 
#define AS_INDIRECT_REGISTER_MODE   0b10
#define AS_INDIRECT_AUTOINCREMENT   0b11
#define AS_IMMEDIATE_MODE           0b11

#define AD_REGISTER_MODE    0
#define AD_INDEXED_MODE     1
#define AD_SYMBOLIC_MODE    1
#define AD_ABSOLUTE_MODE    1


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
private:
    uint8_t get_as_mode(uint8_t As, uint8_t source){ return 0; };
    uint8_t get_ad_mode(bool Ad, uint8_t destination){ return 0; };
    
public:
    std::ostream& operator<<(std::ostream& os){ return os; };
    virtual bool consume(uint16_t instruction) = 0;
};


class Format1Instruction : public Instruction {
private:
    uint8_t opcode;
    uint8_t source;
    bool Ad;
    bool byte_instruction;
    uint8_t As;
    uint8_t destination;
public:
    bool consume(uint16_t instruction) override {
        return (instruction >> 12 == opcode);
    } 
};


class Format2Instruction : public Instruction {
private:
    uint8_t opcode;
    bool byte_instruction;
    uint8_t As;
    uint8_t source;
public:
    bool consume(uint16_t instruction) override {
        return (instruction >> 10 == FORMAT_2_PREFIX) && (((instruction >> 7) & 0b111) == opcode);
    }
};


class Format3Instruction : public Instruction {
private:
    uint8_t condition;
    uint16_t pc_offset;
public:
    bool consume(uint16_t instruction) override {
        return (instruction >> 13 == FORMAT_3_PREFIX) && (((instruction >> 10) & 0b111) == condition);
    }
};


class InstructionFactory {
public:
    typedef std::function<Instruction*()> Builder;

    int Register(Builder const& builder) {
        instruction_types.emplace_back(builder);
        return instruction_types.size();
    }

    Instruction* Build(uint16_t instruction) const {
        for(const Builder& b: instruction_types){
            Instruction* inst = b();
            if(inst->consume(instruction)){
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
Instruction* instructionBuilder() { return new Derived(); }


static const int type1 = GetInstructionFactory().Register(instructionBuilder<Format1Instruction>);
static const int type2 = GetInstructionFactory().Register(instructionBuilder<Format2Instruction>);
static const int type3 = GetInstructionFactory().Register(instructionBuilder<Format3Instruction>);
