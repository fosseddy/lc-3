#ifndef LC3_H
#define LC3_H

typedef unsigned short u16;

enum lc3_reg {
    R_R0 = 0,
    R_R1,
    R_R2,
    R_R3,
    R_R4,
    R_R5,
    R_R6,
    R_R7,

    R_PC,
    R_PSR,

    R_COUNT
};

enum lc3_opcode {
    OP_BR = 0,
    OP_ADD,
    OP_LD,
    OP_ST,
    OP_JSR,
    OP_AND,
    OP_LDR,
    OP_STR,
    OP_RTI,
    OP_NOT,
    OP_LDI,
    OP_STI,
    OP_JMP,
    OP_RESERVED,
    OP_LEA,
    OP_TRAP // @TODO(art): not implemented
};

#endif
