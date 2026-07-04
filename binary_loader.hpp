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
    std::vector<Symbol> symbols_table;

    bool support_symbols = false;

    BinaryLoader() : symbols_table() {};
    bool loadBinary(const std::string &filename);
    uint16_t read_memory(uint32_t address);

    virtual uint32_t getAddressOffset(uint32_t address, bool entrypoint = false) = 0;
    virtual bool getEntryOffset() = 0;

    virtual void parseSymbolTable() {};
    virtual void printSymbolNames(const std::vector<Symbol> &symbols) {};
    virtual bool is_func(Symbol symbol){ return false; };

    virtual bool is_inside_ipe(uint32_t addr){ return false; };
};


struct ELFFile : public BinaryLoader {
    uint32_t dymsym_header_offset;
    uint32_t sym_header_offset;

    ELFFile(){ support_symbols = true; };

    uint32_t getAddressOffset(uint32_t address, bool entrypoint) override;
    bool getEntryOffset() override;

    void parseSymbolTable() override;
    void printSymbolNames(const std::vector<Symbol> &symbols) override;
    bool is_func(Symbol symbol) override;

    // TODO: read the values inside the elf to compute the actual boundaries
    bool is_inside_ipe(uint32_t addr) override { return 0x8000 <= addr <= 0x8000 + 0x6400; };
};


struct DumpFile : public BinaryLoader {
    uint32_t general_offset;

    DumpFile(uint32_t entry = 0, uint32_t offs = 0)
        : general_offset(offs)
     {entry_addr = entry;};

    uint32_t getAddressOffset(uint32_t address, bool entrypoint) override;
    bool getEntryOffset() override;
};
