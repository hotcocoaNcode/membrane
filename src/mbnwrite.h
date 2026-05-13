#pragma once

#include <string>
#include <vector>

struct symtable_ent {
    uint8_t name[24];
    uint64_t reserved_padding = 0;
};

bool assemble_file(const std::string& name, const uint32_t& dynamic_mem);
bool write_executable(const std::string& name, const uint32_t& dynamic_mem, const std::vector<symtable_ent>& symtable, const std::vector<uint8_t>& data, const std::vector<uint32_t>& code);