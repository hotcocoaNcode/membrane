#pragma once

#include <string>
#include <vector>

struct symtable_ent {
    uint8_t name[24];
    uint64_t reserved_padding = 0;
};

bool write_executable(std::string name, uint32_t dynamic_mem, std::vector<symtable_ent> symtable, std::vector<uint8_t> data, std::vector<uint32_t> code);