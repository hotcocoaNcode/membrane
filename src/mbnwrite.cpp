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

void parseData(std::vector<std::string> parsable, std::vector<uint8_t>& data, std::unordered_map<std::string, uint32_t>& assembly_symbol_map) {
    if (streq(parsable[1], "u8")) {
        uint8_t u8 = std::stoull(parsable[2]);
        assembly_symbol_map.emplace(parsable[0], data.size());
        data.push_back(u8);
    } else if (streq(parsable[1], "u16")) {
        uint16_t u16 = std::stoull(parsable[2]);
        assembly_symbol_map.emplace(parsable[0], data.size());
        for (int i = 0; i < sizeof(u16); i++) {
            data.push_back(reinterpret_cast<uint8_t*>(&u16)[i]);
        }
    } else if (streq(parsable[1], "u32")) {
        uint32_t u32 = std::stoull(parsable[2]);
        assembly_symbol_map.emplace(parsable[0], data.size());
        for (int i = 0; i < sizeof(u32); i++) {
            data.push_back(reinterpret_cast<uint8_t*>(&u32)[i]);
        }
    } else if (streq(parsable[1], "i8")) {
        int8_t i8 = std::stoi(parsable[2]);
        assembly_symbol_map.emplace(parsable[0], data.size());
        data.push_back(i8);
    } else if (streq(parsable[1], "i16")) {
        int16_t i16 = std::stoi(parsable[2]);
        assembly_symbol_map.emplace(parsable[0], data.size());
        for (int i = 0; i < sizeof(i16); i++) {
            data.push_back(reinterpret_cast<uint8_t*>(&i16)[i]);
        }
    } else if (streq(parsable[1], "i32")) {
        int32_t i32 = std::stoi(parsable[2]);
        assembly_symbol_map.emplace(parsable[0], data.size());
        for (int i = 0; i < sizeof(i32); i++) {
            data.push_back(reinterpret_cast<uint8_t*>(&i32)[i]);
        }
    } else if (streq(parsable[1], "ascii")) {
        std::string ascii_escape_parsed = "";
        assembly_symbol_map.emplace(parsable[0], data.size());
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

void parseSymtable(std::vector<std::string> parsable, std::vector<symtable_ent>& symtable, std::unordered_map<std::string, uint32_t>& assembly_symbol_map) {
    symtable_ent entry{};
    memcpy(&(entry.name), parsable[0].c_str(), std::min(parsable[0].length(), static_cast<size_t>(24)));
    assembly_symbol_map.emplace(parsable[0], symtable.size());
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

void emit_unsigned_imm24(int icode, std::vector<std::string> parsable, std::vector<uint32_t>& code, const std::unordered_map<std::string, uint32_t>& assembly_symbol_map) {
    uint32_t inst = (icode) << 24;  
    inst |= (data_index_map(parsable[1], assembly_symbol_map) & 0x00FFFFFF); 
    code.push_back(inst);
}

void emit_ra_unsigned_imm16(int icode, std::vector<std::string> parsable, std::unordered_map<std::string, uint32_t> assembly_symbol_map, std::vector<uint32_t>& code) {
    uint32_t inst = (icode) << 24; 
    inst |= (std::stoul(parsable[1]) & 0xFF) << 16; 
    inst |= (data_index_map(parsable[2], assembly_symbol_map) & 0xFFFF); 
    code.push_back(inst);
}

void emit_ra_signed_imm16(int icode, std::vector<std::string> parsable, std::unordered_map<std::string, uint32_t> assembly_symbol_map, std::vector<uint32_t>& code) {
    uint32_t inst = (icode) << 24; 
    inst |= (std::stoul(parsable[1]) & 0xFF) << 16; 
    // avoiding a weird implicit compiler cast
    int16_t a = static_cast<int16_t>(std::stoi(parsable[2]));
    uint16_t* b = reinterpret_cast<uint16_t*>(&a);
    inst |= *b;
    code.push_back(inst);
}

void emit_ra_rb_rc(int icode, std::vector<std::string> parsable, std::vector<uint32_t>& code) {
    uint32_t inst = (icode) << 24; 
    inst |= (std::stoul(parsable[1]) & 0xFF) << 16; 
    inst |= (std::stoul(parsable[2]) & 0xFF) << 8; 
    inst |= (std::stoul(parsable[3]) & 0xFF); 
    code.push_back(inst);
}

void emit_ra_rb(int icode, std::vector<std::string> parsable, std::vector<uint32_t>& code) {
    uint32_t inst = (icode) << 24; 
    inst |= (std::stoul(parsable[1]) & 0xFF) << 16; 
    inst |= (std::stoul(parsable[2]) & 0xFF) << 8; 
    code.push_back(inst);
}

void emit_ra_rb_cnst(int icode, std::vector<std::string> parsable, std::vector<uint32_t>& code, uint8_t cnst) {
    uint32_t inst = (icode) << 24; 
    inst |= (std::stoul(parsable[1]) & 0xFF) << 16; 
    inst |= (std::stoul(parsable[2]) & 0xFF) << 8; 
    inst |= (cnst);
    code.push_back(inst);
}

void emit_unsigned_imm16(int icode, std::vector<std::string> parsable, std::vector<uint32_t>& code) {
    uint32_t inst = (icode) << 24; 
    inst |= (std::stoul(parsable[1]) & 0xFFFF); 
    code.push_back(inst);
}

void emit_signed_imm16(int icode, std::vector<std::string> parsable, std::vector<uint32_t>& code) {
    uint32_t inst = (icode) << 24; 
    // once again avoiding the weird implicit cast
    int16_t a = static_cast<int16_t>(std::stoi(parsable[1]));
    uint16_t* b = reinterpret_cast<uint16_t*>(&a);
    inst |= *b; 
    code.push_back(inst);
}

void parseISA(std::vector<std::string> parsable, std::vector<uint32_t>& code, std::unordered_map<std::string, uint32_t>& assembly_symbol_map) {
    if (streq(parsable[0], "mark")) {
        assembly_symbol_map.emplace(parsable[1], code.size());
    } else if (streq(parsable[0], "nop")) {
        code.push_back(0);
    } else if (streq(parsable[0], "ret")) {
        code.push_back(0x1E << 24);
    } else if (streq(parsable[0], "halt")) {
        code.push_back(0x1F << 24);
    } else if (streq(parsable[0], "li")) {
        emit_ra_unsigned_imm16(0x01, parsable, assembly_symbol_map, code);
    } else if (streq(parsable[0], "lui")) {
        emit_ra_unsigned_imm16(0x02, parsable, assembly_symbol_map, code);
    } else if (streq(parsable[0], "lwa")) {
        emit_ra_unsigned_imm16(0x03, parsable, assembly_symbol_map, code);
    } else if (streq(parsable[0], "lwr")) {
        if (parsable.size() > 3) {
            // technically this is ra_rb_imm8 but this works enough. no register aliasing
            emit_ra_rb_rc(0x04, parsable, code);
        } else {
            emit_ra_rb_cnst(0x04, parsable, code, 4);
        }
    } else if (streq(parsable[0], "swa")) {
        emit_ra_unsigned_imm16(0x05, parsable, assembly_symbol_map, code);
    } else if (streq(parsable[0], "swr")) {
        if (parsable.size() > 3) {
            // ra_rb_imm8 again
            emit_ra_rb_rc(0x06, parsable, code);
        } else {
            emit_ra_rb_cnst(0x06, parsable, code, 4);
        }
    } else if (streq(parsable[0], "mov")) {
        emit_ra_rb(0x07, parsable, code);
    } else if (streq(parsable[0], "add")) {
        emit_ra_rb_rc(0x08, parsable, code);
    } else if (streq(parsable[0], "sub")) {
        emit_ra_rb_rc(0x09, parsable, code);
    } else if (streq(parsable[0], "mul")) {
        emit_ra_rb_rc(0x0A, parsable, code);
    } else if (streq(parsable[0], "div")) {
        emit_ra_rb_rc(0x0B, parsable, code);
    } else if (streq(parsable[0], "mod")) {
        emit_ra_rb_rc(0x0C, parsable, code);
    } else if (streq(parsable[0], "and")) {
        emit_ra_rb_rc(0x0D, parsable, code);
    } else if (streq(parsable[0], "or")) {
        emit_ra_rb_rc(0x0E, parsable, code);
    } else if (streq(parsable[0], "xor")) {
        emit_ra_rb_rc(0x0F, parsable, code);
    } else if (streq(parsable[0], "not")) {
        emit_ra_rb(0x10, parsable, code);
    } else if (streq(parsable[0], "shl")) {
        emit_ra_rb_rc(0x11, parsable, code);
    } else if (streq(parsable[0], "shr")) {
        emit_ra_rb_rc(0x12, parsable, code);
    } else if (streq(parsable[0], "eq")) {
        emit_ra_rb_rc(0x13, parsable, code);
    } else if (streq(parsable[0], "neq")) {
        emit_ra_rb_rc(0x14, parsable, code);
    } else if (streq(parsable[0], "ltu")) {
        emit_ra_rb_rc(0x15, parsable, code);
    } else if (streq(parsable[0], "lts")) {
        emit_ra_rb_rc(0x16, parsable, code);
    } else if (streq(parsable[0], "lteu")) {
        emit_ra_rb_rc(0x17, parsable, code);
    } else if (streq(parsable[0], "ltes")) {
        emit_ra_rb_rc(0x18, parsable, code);
    } else if (streq(parsable[0], "jmp")) {
        emit_signed_imm16(0x19, parsable, code);
    } else if (streq(parsable[0], "jmpz")) {
        emit_ra_signed_imm16(0x1A, parsable, assembly_symbol_map, code);
    } else if (streq(parsable[0], "jmpnz")) {
        emit_ra_signed_imm16(0x1B, parsable, assembly_symbol_map, code);
    } else if (streq(parsable[0], "call")) {
        emit_unsigned_imm24(0x1C, parsable, code, assembly_symbol_map);
    } else if (streq(parsable[0], "kcall")) {
        emit_unsigned_imm24(0x1D, parsable, code, assembly_symbol_map);
    } else {
        std::cout << "warn; unknown instruction " << parsable[0] << "! ommitting." << std::endl;
    }
}

bool assemble_file(const std::string& name, const uint32_t& dynamic_mem) {
    if (!(name[name.length()-1] == 's' && name[name.length()-2] == 'b' && name[name.length()-3] == 'm')) {
        std::cerr << "err; file " << name << " is not a .mbs (membrane source)!" << std::endl;
        return false;
    }
    std::ifstream fi(name);
    if (!(fi.is_open())) {
        std::cerr << "err; couldn't open " << name << std::endl;
    }

    std::vector<symtable_ent> symtable{};
    std::vector<uint8_t> data{};
    std::vector<uint32_t> code{};
    std::unordered_map<std::string, uint32_t> assembly_symbol_map{};

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
                parseData(parsable, data, assembly_symbol_map);
                break;
            case SYMTABLE:
                std::cout << " (stmode)" << std::endl;
                parseSymtable(parsable, symtable, assembly_symbol_map);
                break;
            case CODE:
                std::cout << " (cmode)" << std::endl;
                parseISA(parsable, code, assembly_symbol_map);
                break;
            default:
                continue;
        }
    }
    auto data_rollover = 16 - (data.size() % 16);
    for (int i = 0; i < data_rollover; i++) {
        data.push_back(0);
    }
    std::string executable_name = name;
    executable_name.resize(name.length()-4);
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