#pragma once

#include <optional>
#include <array>
#include <memory>
#include <map>
#include <inttypes.h>

#include "binary_loader.hpp"


// + 1 register for IPE SP
#define NB_REGISTERS 17

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
    R15,
    IPE_SP,
};


class State {
private:
    std::array<std::string, NB_REGISTERS> regs_assoc{
        "pc", "sp", "sr", "zero", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", "ipe_sp"
    };
    std::array<std::optional<uint16_t>, NB_REGISTERS> registers;
    std::map<uint32_t, uint8_t> modified_memory;

    std::shared_ptr<BinaryLoader> binary_file;
public:
    State(uint32_t base_pc, std::shared_ptr<BinaryLoader> loader, bool unkown = false);

    std::optional<uint16_t> read_register(uint8_t reg);
    void write_register(uint8_t reg, std::optional<uint16_t> val);

    std::optional<uint16_t> read_memory(std::optional<uint32_t> addr);
    void write_memory(std::optional<uint32_t> addr, std::optional<uint16_t> val);

    std::string dumpState();
};


std::optional<uint16_t> operator+(std::optional<uint16_t> o1, std::optional<uint16_t> o2);
std::optional<uint16_t> operator-(std::optional<uint16_t> o1, std::optional<uint16_t> o2);
