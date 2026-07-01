#include <iostream>
#include <set>
#include <fstream>
#include <vector>

#include <cstdint>

#include <elf.h>
#include <inttypes.h>

#include "elf_parser.hpp"


bool loadELF(const std::string &filename, ELFFile &elfFile)
{
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file)
    {
        std::cerr << "Failed to open ELF file: " << filename << "\n";
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    elfFile.data.resize(size);

    if (!file.read(reinterpret_cast<char *>(elfFile.data.data()), size))
    {
        std::cerr << "Failed to read ELF file." << std::endl;
        return false;
    }

    return true;
}

uint32_t getAddressOffset(ELFFile &elfFile, uint32_t address, bool entrypoint){
    if (elfFile.data.size() < sizeof(Elf32_Ehdr))
    {
        std::cerr << "Invalid ELF file." << std::endl;
        return false;
    }

    Elf32_Ehdr *header = reinterpret_cast<Elf32_Ehdr *>(elfFile.data.data());
    Elf32_Phdr *program_headers = reinterpret_cast<Elf32_Phdr *>(elfFile.data.data() + header->e_phoff);

    if(entrypoint){
        address = header->e_entry;
        elfFile.entry_addr = address;
    }
        

    for (int i = 0; i < header->e_phnum; i++)
    {
        Elf32_Phdr &ph = program_headers[i];
        if (ph.p_type == PT_LOAD && ph.p_vaddr <= address && address < ph.p_vaddr + ph.p_memsz)
            return address - ph.p_vaddr + ph.p_offset;
    }

    std::cerr << "Offset not found." << std::endl;
    return -1;
}


bool getEntryOffset(ELFFile &elfFile)
{
    uint32_t result = getAddressOffset(elfFile, 0, true);
    if(result == -1)
        return false;

    elfFile.entry_offset = result;
    return true;
}

std::vector<Symbol> parseSymbolTable(ELFFile &elfFile)
{
    std::vector<Symbol> symbols;

    if (elfFile.data.size() < sizeof(Elf32_Ehdr))
    {
        std::cerr << "Invalid ELF file." << std::endl;
        return symbols;
    }

    Elf32_Ehdr *header = reinterpret_cast<Elf32_Ehdr *>(elfFile.data.data());
    Elf32_Shdr *section_headers = reinterpret_cast<Elf32_Shdr *>(elfFile.data.data() + header->e_shoff);

    for (int i = 0; i < header->e_shnum; ++i)
    {
        Elf32_Shdr &sh = section_headers[i];
        if (sh.sh_type != SHT_SYMTAB && sh.sh_type != SHT_DYNSYM)
            continue;

        const Elf32_Sym *symtab = reinterpret_cast<const Elf32_Sym *>(elfFile.data.data() + sh.sh_offset);
        size_t symbol_count = sh.sh_size / sizeof(Elf32_Sym);

        const Elf32_Shdr &strtab_section = section_headers[sh.sh_link];
        const char *strtab = reinterpret_cast<const char *>(elfFile.data.data() + strtab_section.sh_offset);

        for (size_t j = 0; j < symbol_count; ++j)
        {
            const Elf32_Sym &sym = symtab[j];
            if (sym.st_name == 0)
                continue;

            Symbol s;
            Elf32_Shdr section;
            s.name = std::string(strtab + sym.st_name);
            s.address = sym.st_value;
            s.size = sym.st_size;
            s.info = sym.st_info;
            s.section_index = sym.st_shndx;
            if (s.section_index <= header->e_shnum)
            {
                section = section_headers[s.section_index];
                if ((section.sh_flags & SHF_EXECINSTR))
                {
                    s.executable = 1;
                }
                else
                {
                    s.executable = 0;
                }
            }
            symbols.push_back(s);
        }
    }
    return symbols;
}

void printSymbolNames(const std::vector<Symbol> &symbols)
{
    std::cout << "\nSymbols:\n";
    std::cout << "-----------------------------\n";
    for (const auto &sym : symbols)
    {
        std::cout << sym.name << " @ 0x" << std::hex << sym.address << std::dec << "\n";
    }
    std::cout << "-----------------------------\n";
}

bool is_func(Symbol symbol){
    return ELF32_ST_TYPE(symbol.info) == STT_FUNC;
}
