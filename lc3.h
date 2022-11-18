#ifndef LC3_H
#define LC3_H

enum reg {
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

enum {
    OP_BR = 0,
    OP_ADD,
    OP_LDB,
    OP_STB,
    OP_JSR, // @TODO(art): not implemented
    OP_AND,
    OP_LDW,
    OP_STW,
    OP_RTI, // @TODO(art): not implemented
    OP_XOR,
    OP_RESERVED3,
    OP_RESERVED4,
    OP_JMP,
    OP_SHF,
    OP_LEA,
    OP_TRAP // @TODO(art): not implemented
};

#endif
