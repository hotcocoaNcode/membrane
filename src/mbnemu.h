#pragma once

#include <string>

#pragma pack(push, 1)
struct ra_rb_rc {
    uint8_t rc;
    uint8_t rb;
    uint8_t ra;
};

struct ra_imm16 {
    uint16_t imm16;
    uint8_t ra;
};

struct inst {
    union {
        ra_rb_rc mode_ra_rb_rc;
        ra_imm16 mode_ra_imm16;
    };
    uint8_t opcode;
};
#pragma pack(pop)

void exec(std::string fname);