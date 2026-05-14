#include "mbnwrite.h"
#include <fstream>
#include <iostream>
#include <cctype>
#include <algorithm>

#define streq(a, b) (a.compare(b) == 0)

std::vector<std::string> space_sep(std::string line) {
    std::vector<std::string> out{};
    out.push_back("");
    bool in_continuous_string = false;
    bool strcapture = false;
    bool escape = false;
    for (const auto& c : line) {
        if (!strcapture && c == ';') return out; // we made it to comment
        else if (!strcapture && std::isspace(c)) {
            if (in_continuous_string) {
                out.push_back("");
            }
            in_continuous_string = false;
        } else if (c == '"' && !escape) {
            strcapture = !strcapture;
        } else {
            if (c == '\\') escape = true;
            else if (escape) escape = false;
            in_continuous_string = true;
            out.back() = out.back() + c;
        }
    }
    return out;
}

enum assembler_section {
    NONE, DATA, SYMTABLE, CODE
};

void parseData(std::vector<std::string> parsable, std::vector<uint8_t>& data, std::unordered_map<std::string, uint32_t>& data_indices) {
    if (streq(parsable[1], "u8")) {
        uint8_t u8 = std::stoull(parsable[2]);
        data_indices.emplace(parsable[0], data.size());
        data.push_back(u8);
    } else if (streq(parsable[1], "u16")) {
        uint16_t u16 = std::stoull(parsable[2]);
        data_indices.emplace(parsable[0], data.size());
        for (int i = 0; i < sizeof(u16); i++) {
            data.push_back(reinterpret_cast<uint8_t*>(&u16)[i]);
        }
    } else if (streq(parsable[1], "u32")) {
        uint32_t u32 = std::stoull(parsable[2]);
        data_indices.emplace(parsable[0], data.size());
        for (int i = 0; i < sizeof(u32); i++) {
            data.push_back(reinterpret_cast<uint8_t*>(&u32)[i]);
        }
    } else if (streq(parsable[1], "i8")) {
        int8_t i8 = std::stoi(parsable[2]);
        data_indices.emplace(parsable[0], data.size());
        data.push_back(i8);
    } else if (streq(parsable[1], "i16")) {
        int16_t i16 = std::stoi(parsable[2]);
        data_indices.emplace(parsable[0], data.size());
        for (int i = 0; i < sizeof(i16); i++) {
            data.push_back(reinterpret_cast<uint8_t*>(&i16)[i]);
        }
    } else if (streq(parsable[1], "i32")) {
        int32_t i32 = std::stoi(parsable[2]);
        data_indices.emplace(parsable[0], data.size());
        for (int i = 0; i < sizeof(i32); i++) {
            data.push_back(reinterpret_cast<uint8_t*>(&i32)[i]);
        }
    } else if (streq(parsable[1], "ascii")) {
        std::string ascii_escape_parsed = "";
        data_indices.emplace(parsable[0], data.size());
        for (int i = 0; i < parsable[2].length(); i++) {
            if (parsable[2][i] == '\\') {
                char hex[2]; // for \xFF codes, have to init outside of switch
                switch (parsable[2][++i]) {
                    case 'a':
                        ascii_escape_parsed += '\a';
                        break;
                    case 'b':
                        ascii_escape_parsed += '\b';
                        break;
                    case 't':
                        ascii_escape_parsed += '\t';
                        break;
                    case 'n':
                        ascii_escape_parsed += '\n';
                        break;
                    case 'v':
                        ascii_escape_parsed += '\v';
                        break;
                    case 'f':
                        ascii_escape_parsed += '\f';
                        break;
                    case 'r':
                        ascii_escape_parsed += '\r';
                        break;
                    case 'e':
                        ascii_escape_parsed += '\e';
                        break;
                    case '\\':
                        ascii_escape_parsed += '\\';
                        break;
                    case 'x':
                        hex[0] = parsable[2][++i];
                        hex[1] = parsable[2][++i];
                        ascii_escape_parsed += static_cast<char>(std::stoul(std::string(hex), nullptr, 16));
                        break;
                    default:
                        break;
                }
            } else {
                ascii_escape_parsed += parsable[2][i];
            }
        }
        for (const auto& c : ascii_escape_parsed) {
            data.push_back(c);
        }
    }
}

void parseSymtable(std::vector<std::string> parsable, std::vector<symtable_ent>& symtable) {
    symtable_ent entry{};
    memcpy(&(entry.name), parsable[0].c_str(), std::min(parsable[0].length(), static_cast<size_t>(24)));
    symtable.push_back(entry);
}

uint32_t data_index_map(std::string in, const std::unordered_map<std::string, uint32_t>& map) {
    if (map.find(in) != map.end()) {
        return map.at(in);
    } else if (in[0] == '0' && in[1] == 'x') {
        in.erase(0, 2);
        return std::stoul(in, nullptr, 16);
    } else if (in[0] == '0' && in[1] == 'b') {
        in.erase(0, 2);
        return std::stoul(in, nullptr, 2);
    } else {
        return std::stoul(in);
    }
}

void inst_unsigned_imm24(int icode, std::vector<std::string> parsable, std::vector<uint32_t>& code) {
    uint32_t inst = (icode) << 24; 
    inst |= (std::stoul(parsable[1]) % 0xFFFFFF); 
    code.push_back(inst);
}

void inst_ra_unsigned_imm16(int icode, std::vector<std::string> parsable, std::unordered_map<std::string, uint32_t> data_indices, std::vector<uint32_t>& code) {
    uint32_t inst = (icode) << 24; 
    inst |= (std::stoul(parsable[1]) % 0xFF) << 16; 
    inst |= (data_index_map(parsable[2], data_indices) % 0xFFFF); 
    code.push_back(inst);
}

void inst_ra_signed_imm16(int icode, std::vector<std::string> parsable, std::unordered_map<std::string, uint32_t> data_indices, std::vector<uint32_t>& code) {
    uint32_t inst = (icode) << 24; 
    inst |= (std::stoul(parsable[1]) % 0xFF) << 16; 
    inst |= static_cast<signed short>(std::stoi(parsable[2])); 
    code.push_back(inst);
}

void inst_ra_rb_rc(int icode, std::vector<std::string> parsable, std::vector<uint32_t>& code) {
    uint32_t inst = (icode) << 24; 
    inst |= (std::stoul(parsable[1]) % 0xFF) << 16; 
    inst |= (std::stoul(parsable[2]) % 0xFF) << 8; 
    inst |= (std::stoul(parsable[3]) % 0xFF); 
    code.push_back(inst);
}

void inst_ra_rb(int icode, std::vector<std::string> parsable, std::vector<uint32_t>& code) {
    uint32_t inst = (icode) << 24; 
    inst |= (std::stoul(parsable[1]) % 0xFF) << 16; 
    inst |= (std::stoul(parsable[2]) % 0xFF) << 8; 
    code.push_back(inst);
}

void inst_ra_rb_cnst(int icode, std::vector<std::string> parsable, std::vector<uint32_t>& code, uint8_t cnst) {
    uint32_t inst = (icode) << 24; 
    inst |= (std::stoul(parsable[1]) % 0xFF) << 16; 
    inst |= (std::stoul(parsable[2]) % 0xFF) << 8; 
    inst |= (cnst);
    code.push_back(inst);
}

void inst_unsigned_imm16(int icode, std::vector<std::string> parsable, std::vector<uint32_t>& code) {
    uint32_t inst = (icode) << 24; 
    inst |= (std::stoul(parsable[1]) % 0xFFFF); 
    code.push_back(inst);
}

void inst_signed_imm16(int icode, std::vector<std::string> parsable, std::vector<uint32_t>& code) {
    uint32_t inst = (icode) << 24; 
    inst |= static_cast<signed short>(std::stoi(parsable[1])); 
    code.push_back(inst);
}

void parseISA(std::vector<std::string> parsable, std::vector<uint32_t>& code, std::unordered_map<std::string, uint32_t>& data_indices) {
    if (streq(parsable[0], "mark")) {
        data_indices.emplace(parsable[1], code.size());
    } else if (streq(parsable[0], "nop")) {
        code.push_back(0);
    } else if (streq(parsable[0], "ret")) {
        code.push_back(0x1E << 24);
    } else if (streq(parsable[0], "halt")) {
        code.push_back(0x1F << 24);
    } else if (streq(parsable[0], "li")) {
        inst_ra_unsigned_imm16(0x01, parsable, data_indices, code);
    } else if (streq(parsable[0], "lui")) {
        inst_ra_unsigned_imm16(0x02, parsable, data_indices, code);
    } else if (streq(parsable[0], "lwa")) {
        inst_ra_unsigned_imm16(0x03, parsable, data_indices, code);
    } else if (streq(parsable[0], "lwr")) {
        if (parsable.size() > 3) {
            // technically this is ra_rb_imm8 but this works enough. no register aliasing
            inst_ra_rb_rc(0x04, parsable, code);
        } else {
            inst_ra_rb_cnst(0x04, parsable, code, 4);
        }
    } else if (streq(parsable[0], "swa")) {
        inst_ra_unsigned_imm16(0x05, parsable, data_indices, code);
    } else if (streq(parsable[0], "swr")) {
        if (parsable.size() > 3) {
            // ra_rb_imm8 again
            inst_ra_rb_rc(0x06, parsable, code);
        } else {
            inst_ra_rb_cnst(0x06, parsable, code, 4);
        }
    } else if (streq(parsable[0], "mov")) {
        inst_ra_rb(0x07, parsable, code);
    } else if (streq(parsable[0], "add")) {
        inst_ra_rb_rc(0x08, parsable, code);
    } else if (streq(parsable[0], "sub")) {
        inst_ra_rb_rc(0x09, parsable, code);
    } else if (streq(parsable[0], "mul")) {
        inst_ra_rb_rc(0x0A, parsable, code);
    } else if (streq(parsable[0], "div")) {
        inst_ra_rb_rc(0x0B, parsable, code);
    } else if (streq(parsable[0], "mod")) {
        inst_ra_rb_rc(0x0C, parsable, code);
    } else if (streq(parsable[0], "and")) {
        inst_ra_rb_rc(0x0D, parsable, code);
    } else if (streq(parsable[0], "or")) {
        inst_ra_rb_rc(0x0E, parsable, code);
    } else if (streq(parsable[0], "xor")) {
        inst_ra_rb_rc(0x0F, parsable, code);
    } else if (streq(parsable[0], "not")) {
        inst_ra_rb(0x10, parsable, code);
    } else if (streq(parsable[0], "shl")) {
        inst_ra_rb_rc(0x11, parsable, code);
    } else if (streq(parsable[0], "shr")) {
        inst_ra_rb_rc(0x12, parsable, code);
    } else if (streq(parsable[0], "eq")) {
        inst_ra_rb_rc(0x13, parsable, code);
    } else if (streq(parsable[0], "neq")) {
        inst_ra_rb_rc(0x14, parsable, code);
    } else if (streq(parsable[0], "ltu")) {
        inst_ra_rb_rc(0x15, parsable, code);
    } else if (streq(parsable[0], "lts")) {
        inst_ra_rb_rc(0x16, parsable, code);
    } else if (streq(parsable[0], "lteu")) {
        inst_ra_rb_rc(0x17, parsable, code);
    } else if (streq(parsable[0], "ltes")) {
        inst_ra_rb_rc(0x18, parsable, code);
    } else if (streq(parsable[0], "jmp")) {
        inst_signed_imm16(0x19, parsable, code);
    } else if (streq(parsable[0], "jmpz")) {
        inst_ra_signed_imm16(0x1A, parsable, data_indices, code);
    } else if (streq(parsable[0], "jmpnz")) {
        inst_ra_signed_imm16(0x1B, parsable, data_indices, code);
    } else if (streq(parsable[0], "call")) {
        inst_unsigned_imm24(0x1C, parsable, code);
    } else if (streq(parsable[0], "kcall")) {
        inst_unsigned_imm24(0x1D, parsable, code);
    } else {
        std::cout << "warn; unknown instruction " << parsable[0] << "! ommitting." << std::endl;
    }
}

bool assemble_file(const std::string& name, const uint32_t& dynamic_mem) {
    if (!(name[name.length()-1] == 's' && name[name.length()-2] == 'm' && name[name.length()-3] == '.')) {
        std::cerr << "err; file " << name << " is not a .ms (membrane source)!" << std::endl;
        return false;
    }
    std::ifstream fi(name);
    if (!(fi.is_open())) {
        std::cerr << "err; couldn't open " << name << std::endl;
    }

    std::vector<symtable_ent> symtable{};
    std::vector<uint8_t> data{};
    std::vector<uint32_t> code{};
    std::unordered_map<std::string, uint32_t> data_indices{};

    std::string line;

    assembler_section mode = NONE;
    while (std::getline(fi, line)) {
        std::vector<std::string> parsable = space_sep(line);
        std::cout << line;
        if (parsable[0].compare("end") == 0) {
            mode = NONE;
            std::cout << " (end_mode smem@" << data.size() << ")" << std::endl;
            continue;
        }
        switch (mode) {
            case NONE:
                if (parsable[0].compare("begin") == 0) {
                    std::cout << " (begin_mode)" << std::endl;
                    if (parsable[1].compare("data") == 0) mode = DATA;
                    else if (parsable[1].compare("symtable") == 0) mode = SYMTABLE;
                    else if (parsable[1].compare("exec") == 0) mode = CODE;
                }
                break;
            case DATA:
                std::cout << " (dmode)" << std::endl;
                parseData(parsable, data, data_indices);
                break;
            case SYMTABLE:
                std::cout << " (stmode)" << std::endl;
                parseSymtable(parsable, symtable);
                break;
            case CODE:
                std::cout << " (cmode)" << std::endl;
                parseISA(parsable, code, data_indices);
                break;
            default:
                continue;
        }
    }
    auto data_rollover = data.size() % 16;
    for (int i = 0; i < data_rollover; i++) {
        data.push_back(0);
    }
    std::string executable_name = name;
    executable_name.resize(name.length()-3);
    executable_name += ".mbn";
    return write_executable(executable_name, dynamic_mem, symtable, data, code);
}

bool write_executable(const std::string& name, const uint32_t& dynamic_mem, const std::vector<symtable_ent>& symtable, const std::vector<uint8_t>& data, const std::vector<uint32_t>& code) {
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
    #define fwritevec(var) fo.write(reinterpret_cast<const char *>(&(var[0])), var.size()*sizeof(var[0]))
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