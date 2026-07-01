#pragma once

#include <iostream>
#include <set>
#include <fstream>
#include <vector>

#include <cstdint>

#include <elf.h>
#include <inttypes.h>


struct Symbol
{
    std::string name;
    uint32_t address;
    uint32_t size;
    uint8_t info;
    uint16_t section_index;
    bool executable;
};


struct BinaryLoader {
    uint32_t entry_addr;
    uint32_t entry_offset;

    std::vector<uint8_t> data;

    bool support_symbols = false;

    bool loadBinary(const std::string &filename);

    virtual uint32_t getAddressOffset(uint32_t address, bool entrypoint = false) = 0;
    virtual bool getEntryOffset() = 0;

    virtual std::vector<Symbol> parseSymbolTable() { return {}; };
    virtual void printSymbolNames(const std::vector<Symbol> &symbols) {};
    virtual bool is_func(Symbol symbol){ return false; };
};


struct ELFFile : public BinaryLoader {
    uint32_t dymsym_header_offset;
    uint32_t sym_header_offset;

    ELFFile(){ support_symbols = true; };

    uint32_t getAddressOffset(uint32_t address, bool entrypoint) override;
    bool getEntryOffset() override;

    std::vector<Symbol> parseSymbolTable() override;
    void printSymbolNames(const std::vector<Symbol> &symbols) override;
    bool is_func(Symbol symbol) override;
};


struct DumpFile : public BinaryLoader {
    uint32_t general_offset;

    DumpFile(uint32_t offs)
        : general_offset(offs)
     {};

    uint32_t getAddressOffset(uint32_t address, bool entrypoint) override;
    bool getEntryOffset() override;
};
