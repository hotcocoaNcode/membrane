#include "mbnemu.h"

#include <iostream>
#include <fstream>
#include <unordered_map>

uint32_t vreg_arr[256]{};
uint32_t program_stack[256]{};
void(**symtab)();
char* prgmem = nullptr;
inst* prgcode = nullptr;
bool halt = false;
uint32_t pc;
uint8_t program_stack_top = 0;

void f_kernel_println() {
    char printstr[256]{};
    memcpy(printstr, &(prgmem[vreg_arr[0]]), prgmem[vreg_arr[1]]);
    std::cout << std::string(printstr) << std::flush;
}

void f_kernel_ret() {
    uint8_t return_code = prgmem[vreg_arr[0]];
    std::cout << "\nProgram exited with code " << static_cast<uint32_t>(return_code) << std::endl;
    halt = true;
}


void (*symtable_lookup(std::string name))(void) {
    if (name.compare("kernel_println") == 0) {
        return f_kernel_println;
    } else if (name.compare("kernel_ret") == 0) {
        return f_kernel_ret;
    }
    return nullptr;
}

void exec(std::string fname) {
    std::ifstream fi;
    fi.open(fname, std::ios::binary);
    if (!fi.is_open()) {
        std::cerr << "err; could not open provided file" << std::endl;
        return;
    }

    char magic[9]{}; // 9 because needs nullterm
    fi.read(magic, 8);
    if (strcmp(magic, "membrane") != 0) {
        std::cerr << "err; provided file is not a valid executable (no magic number)" << std::endl;
        return;
    }

    uint16_t ver = 0;
    fi.read(reinterpret_cast<char*>(&ver), sizeof(uint16_t));
    if (ver != 1) {
        std::cerr << "err; unknown version " << ver << std::endl;
        return;
    }

    uint32_t mem_max = 0;
    fi.read(reinterpret_cast<char*>(&mem_max), sizeof(uint32_t));
    prgmem = reinterpret_cast<char*>(calloc(1, mem_max));
    if (prgmem == nullptr) {
        std::cerr << "err; could not allocate program memory requirement" << std::endl;
    }

    uint32_t code_start_offset = 0;
    fi.read(reinterpret_cast<char*>(&code_start_offset), sizeof(uint32_t));

    uint16_t symtable_length = 0;
    fi.read(reinterpret_cast<char*>(&symtable_length), sizeof(uint16_t));
    symtab = reinterpret_cast<void(**)()>(malloc(sizeof(void(*)()) * symtable_length));

    for (int i = 0; i < symtable_length; i++) {
        char fn_name_raw[32];
        fi.read(fn_name_raw, 32);
        std::string fn_name(fn_name_raw);
        void(*lookup_ptr)();
        lookup_ptr = symtable_lookup(fn_name);
        if (lookup_ptr == nullptr) {
            std::cerr << "err; could not resolve " << fn_name << " from emulator symbol table" << std::endl;
            return;
        }
        symtab[i] = lookup_ptr;
    }

    uint16_t data_section_size_blocks = 0;
    fi.read(reinterpret_cast<char*>(&data_section_size_blocks), sizeof(uint16_t));
    uint32_t data_section_size = static_cast<uint32_t>(data_section_size_blocks)*16;
    if (data_section_size > mem_max) {
        std::cerr << "err; constant data larger than memory maximum. file likely corrupt" << std::endl;
        return;
    }
    fi.read(prgmem, data_section_size);


    auto header_end = static_cast<size_t>(fi.tellg());
    if (header_end != code_start_offset) {
        std::cerr << "err; code start offset does not match header size. file likely corrupt" << std::endl;
        return;
    }
    fi.seekg(0, std::ios::end);
    auto file_end = static_cast<size_t>(fi.tellg());
    auto instructions_len = file_end - header_end;
    fi.seekg(header_end);
    prgcode = reinterpret_cast<inst*>(malloc(instructions_len));
    fi.read(reinterpret_cast<char*>(prgcode), instructions_len);
    uint32_t instructions_len_opcodes = instructions_len/4;

    pc = 0;
    while (!halt) {
        if (pc > instructions_len_opcodes) {
            std::cerr << "err; program counter is larger than instruction array size! likely a bad jmp or overrun." << std::endl;
            break;
        }
        inst current = prgcode[pc++];
        uint32_t bitmasked_u24;
        switch (current.opcode) {
            case 0x00: // nop
                break;
            case 0x01: // li
                vreg_arr[current.mode_ra_imm16.ra] &= 0xFFFF0000;
                vreg_arr[current.mode_ra_imm16.ra] |= current.mode_ra_imm16.imm16;
                break;
            case 0x02: // lui
                vreg_arr[current.mode_ra_imm16.ra] &= 0x0000FFFF;
                vreg_arr[current.mode_ra_imm16.ra] |= static_cast<uint32_t>(current.mode_ra_imm16.imm16) << 16;
                break;
            case 0x03: // lwa
                vreg_arr[current.mode_ra_imm16.ra] = *reinterpret_cast<uint32_t*>(&prgmem[current.mode_ra_imm16.imm16]);
                break;
            case 0x04: // lwr
                memcpy(&vreg_arr[current.mode_ra_rb_rc.ra], 
                    &prgmem[vreg_arr[current.mode_ra_rb_rc.rb]], 
                    std::min(static_cast<unsigned long>(current.mode_ra_rb_rc.rc), sizeof(uint32_t))
                );
                break;
            case 0x05: // swa
                *reinterpret_cast<uint32_t*>(&prgmem[current.mode_ra_imm16.imm16]) = vreg_arr[current.mode_ra_imm16.ra];
                break;
            case 0x06: // lwr
                memcpy(&prgmem[vreg_arr[current.mode_ra_rb_rc.rb]], 
                    &vreg_arr[current.mode_ra_rb_rc.ra], 
                    std::min(static_cast<unsigned long>(current.mode_ra_rb_rc.rc), sizeof(uint32_t))
                );
                break;
            case 0x07: // mov
                vreg_arr[current.mode_ra_rb_rc.ra] = vreg_arr[current.mode_ra_rb_rc.rb];
                break;
            case 0x08: // add
                vreg_arr[current.mode_ra_rb_rc.ra] = vreg_arr[current.mode_ra_rb_rc.rb] + vreg_arr[current.mode_ra_rb_rc.rc];
                break;
            case 0x09: // sub
                vreg_arr[current.mode_ra_rb_rc.ra] = vreg_arr[current.mode_ra_rb_rc.rb] - vreg_arr[current.mode_ra_rb_rc.rc];
                break;
            case 0x0A: // mul
                vreg_arr[current.mode_ra_rb_rc.ra] = vreg_arr[current.mode_ra_rb_rc.rb] * vreg_arr[current.mode_ra_rb_rc.rc];
                break;
            case 0x0B: // div
                vreg_arr[current.mode_ra_rb_rc.ra] = vreg_arr[current.mode_ra_rb_rc.rb] / vreg_arr[current.mode_ra_rb_rc.rc];
                break;
            case 0x0C: // mod
                vreg_arr[current.mode_ra_rb_rc.ra] = vreg_arr[current.mode_ra_rb_rc.rb] % vreg_arr[current.mode_ra_rb_rc.rc];
                break;
            case 0x0D: // and
                vreg_arr[current.mode_ra_rb_rc.ra] = vreg_arr[current.mode_ra_rb_rc.rb] & vreg_arr[current.mode_ra_rb_rc.rc];
                break;
            case 0x0E: // or
                vreg_arr[current.mode_ra_rb_rc.ra] = vreg_arr[current.mode_ra_rb_rc.rb] | vreg_arr[current.mode_ra_rb_rc.rc];
                break;
            case 0x0F: // xor
                vreg_arr[current.mode_ra_rb_rc.ra] = vreg_arr[current.mode_ra_rb_rc.rb] ^ vreg_arr[current.mode_ra_rb_rc.rc];
                break;
            case 0x10: // not
                vreg_arr[current.mode_ra_rb_rc.ra] = ~vreg_arr[current.mode_ra_rb_rc.rb];
                break;
            case 0x11: // shl
                vreg_arr[current.mode_ra_rb_rc.ra] = vreg_arr[current.mode_ra_rb_rc.rb] << vreg_arr[current.mode_ra_rb_rc.rc];
                break;
            case 0x12: // shr
                vreg_arr[current.mode_ra_rb_rc.ra] = vreg_arr[current.mode_ra_rb_rc.rb] >> vreg_arr[current.mode_ra_rb_rc.rc];
                break;
            case 0x13: // eq
                vreg_arr[current.mode_ra_rb_rc.ra] = 
                (vreg_arr[current.mode_ra_rb_rc.rb] == vreg_arr[current.mode_ra_rb_rc.rc] ?
                1 : 0);
                break;
            case 0x14: // neq
                vreg_arr[current.mode_ra_rb_rc.ra] = 
                (vreg_arr[current.mode_ra_rb_rc.rb] != vreg_arr[current.mode_ra_rb_rc.rc] ?
                1 : 0);
                break;
            case 0x15: // ltu
                vreg_arr[current.mode_ra_rb_rc.ra] = 
                (vreg_arr[current.mode_ra_rb_rc.rb] < vreg_arr[current.mode_ra_rb_rc.rc] ?
                1 : 0);
                break;
            case 0x16: // lts
                vreg_arr[current.mode_ra_rb_rc.ra] = 
                ((*reinterpret_cast<int32_t*>(&vreg_arr[current.mode_ra_rb_rc.rb])) 
                < (*reinterpret_cast<int32_t*>(&vreg_arr[current.mode_ra_rb_rc.rc]))  ?
                1 : 0);
                break;
            case 0x17: // lteu
                vreg_arr[current.mode_ra_rb_rc.ra] = 
                (vreg_arr[current.mode_ra_rb_rc.rb] <= vreg_arr[current.mode_ra_rb_rc.rc] ?
                1 : 0);
                break;
            case 0x18: // ltes
                vreg_arr[current.mode_ra_rb_rc.ra] = 
                ((*reinterpret_cast<int32_t*>(&vreg_arr[current.mode_ra_rb_rc.rb])) 
                <= (*reinterpret_cast<int32_t*>(&vreg_arr[current.mode_ra_rb_rc.rc]))  ?
                1 : 0);
                break;
            case 0x19: // jmp
                pc += (*reinterpret_cast<int16_t*>(&current.mode_ra_imm16.imm16));
                break;
            case 0x1A: // jmpz
                if (vreg_arr[current.mode_ra_imm16.ra] == 0) {
                    pc += (*reinterpret_cast<int16_t*>(&current.mode_ra_imm16.imm16));
                }
                break;
            case 0x1B: // jmpnz
                if (vreg_arr[current.mode_ra_imm16.ra] != 0) {
                    pc += (*reinterpret_cast<int16_t*>(&current.mode_ra_imm16.imm16));
                }
                break;
            case 0x1C: // call
                if ((pc + 1) == 256) {
                    std::cerr << "err; callstack overflow" << std::endl;
                    exit(0);
                }
                bitmasked_u24 = (*reinterpret_cast<uint32_t*>(&current)) & 0x00FFFFFF;
                program_stack[program_stack_top++] = pc;
                pc = bitmasked_u24;
                break;
            case 0x1D: // kcall
                bitmasked_u24 = (*reinterpret_cast<uint32_t*>(&current)) & 0x00FFFFFF;
                symtab[bitmasked_u24]();
                break;
            case 0x1E: // ret
                if (pc == 0) {
                    std::cerr << "err; callstack underflow" << std::endl;
                    exit(0);
                }
                pc = program_stack[--program_stack_top];
                break;
            case 0x1F: // halt
                halt = true;
                break;
            default:
                std::cerr << "err; unknown opcode " << static_cast<uint32_t>(current.opcode) << " at " << pc << std::endl;
                exit(0);
        }
    }

    free(symtab);
    free(prgmem);
    free(prgcode);
}