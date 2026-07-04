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
    std::array<std::optional<uint16_t>, NB_REGISTERS> registers;
    std::map<uint32_t, uint8_t> modified_memory;

    std::shared_ptr<BinaryLoader> binary_file;
public:
    State(uint32_t base_pc, bool unkown = false);


};
