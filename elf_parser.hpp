#pragma once

#include <iostream>
#include <set>
#include <fstream>
#include <vector>

#include <cstdint>

#include <elf.h>
#include <inttypes.h>


struct ELFFile
{
    std::vector<uint8_t> data;
    uint32_t entry_offset;
    uint32_t dymsym_header_offset;
    uint32_t sym_header_offset;
};

struct Symbol
{
    std::string name;
    uint32_t address;
    uint32_t size;
    uint8_t info;
    uint16_t section_index;
    bool executable;
};

bool loadELF(const std::string &filename, ELFFile &elfFile);
bool getEntryOffset(ELFFile &elfFile);
void printSymbolNames(const std::vector<Symbol> &symbols);
std::vector<Symbol> parseSymbolTable(ELFFile &elfFile);
