#include <iostream>
#include <fstream>

struct symtable_ent {
    uint8_t name[24];
    uint64_t reserved_padding = 0;
};

int main() {
    uint8_t magic[8] = {'m', 'e', 'm', 'b', 'r', 'a', 'n', 'e'};
    uint16_t version = 1;
    uint32_t mem_max = 0x10;
    uint32_t offset = 102; //hope i counted right
    uint16_t symtable_count = 2;
    symtable_ent entries[2];
    entries[0] = {"kernel_println"};
    entries[1] = {"kernel_ret"};
    uint16_t data_section_size = 1;
    uint8_t data[16] = {'h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd', '!', '\n', 0, 14, 0};
    uint32_t code[5] = {0x01000000, 0x0101000E, 0x1D000000, 0x0100000F, 0x1D000001};
    
    std::ofstream fo;
    fo.open("helloworld.mbn", std::ios::binary | std::ios::out | std::ios::trunc);
    #define fwritearr(var) fo.write(reinterpret_cast<char *>(var), sizeof(var))
    #define fwriteobj(var) fo.write(reinterpret_cast<char *>(&var), sizeof(var))
    fwritearr(magic);
    fwriteobj(version);
    fwriteobj(mem_max);
    fwriteobj(offset);
    fwriteobj(symtable_count);
    fwritearr(entries);
    fwriteobj(data_section_size);
    fwritearr(data);
    fwritearr(code);
    std::cout << "hhahahfndf" << std::endl;
    return 0;
}