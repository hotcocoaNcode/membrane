#include <iostream>
#include <string>
#include "mbnwrite.h"

int main() {
    std::vector<symtable_ent> symtable = {{"kernel_println"}, {"kernel_ret"}};
    std::vector<uint8_t> data = {'h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd', '!', '\n', 0, 14, 0};
    std::vector<uint32_t> code = {0x01000000, 0x0101000E, 0x1D000000, 0x0100000F, 0x1D000001};

    write_executable("helloworld.mbn", 0, symtable, data, code);
    return 0;
}