#include "mbnwrite.h"
#include <fstream>
#include <iostream>

bool write_executable(std::string name, uint32_t dynamic_mem, std::vector<symtable_ent> symtable, std::vector<uint8_t> data, std::vector<uint32_t> code) {
    uint8_t magic[8] = {'m', 'e', 'm', 'b', 'r', 'a', 'n', 'e'};
    uint16_t version = 1;
    uint16_t symtable_count = symtable.size();
    if (data.size() % 16 != 0) {
        std::cerr << "err; data section size was not a multiple of 16!" << std::endl;
        return false;
    }
    uint16_t data_section_size = data.size()/16;
    uint32_t offset = 22+symtable.size()*32+data.size();
    uint32_t mem_max = dynamic_mem + data.size();

    char zeroes_block_arr[16] = {};
    
    std::ofstream fo;
    fo.open(name, std::ios::binary | std::ios::out | std::ios::trunc);

    // for the sake of not writing loss ass lines over and over
    #define fwritearr(var) fo.write(reinterpret_cast<char *>(var), sizeof(var))
    #define fwritevec(var) fo.write(reinterpret_cast<char *>(&(var[0])), var.size()*sizeof(var[0]))
    #define fwritevar(var) fo.write(reinterpret_cast<char *>(&var), sizeof(var))

    fwritearr(magic);
    fwritevar(version);
    fwritevar(mem_max);
    fwritevar(offset);
    fwritevar(symtable_count);
    fwritevec(symtable);
    fwritevar(data_section_size);
    fwritevec(data);
    fwritevec(code);
    
    std::cout << "wrote " << name << std::endl;
    return true;
}