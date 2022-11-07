#include <stdio.h>

#define MEMORY_CAP (1 << 16)

typedef unsigned short word;
typedef unsigned char byte;

enum {
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
    CC_P = 0x1,
    CC_Z = 0x2,
    CC_N = 0x4
};

enum {
    OP_BR = 0,
    OP_ADD,
    OP_LD,
    OP_ST,
    OP_RESERVED1,
    OP_AND,
    OP_LDR,
    OP_STR,
    OP_RESERVED2,
    OP_NOT,
    OP_LDI,
    OP_STI,
    OP_JMP,
    OP_RESERVED3,
    OP_LEA,
    OP_TRAP
};

static word regs[R_COUNT] = {0};
static byte memory[MEMORY_CAP] = {0};

word sext(word value, size_t bit_len)
{
    if (value >> (bit_len - 1) & 0x1) {
        return 0xFFFF << bit_len | value;
    }

    return value;
}

void setcc(word value)
{
    word nzp;
    if (value == 0) {
        nzp = CC_Z;
    } else if (value >> 15) {
        nzp = CC_N;
    } else {
        nzp = CC_P;
    }

    regs[R_PSR] = regs[R_PSR] & 0x8000 | nzp;
}

void debug_regs()
{
    for (size_t i = 0; i < 8; ++i) {
        printf("R%lu: %5hu |", i, regs[i]);
    }
    printf("NZP: %hx |", regs[R_PSR] & 0x7);
    printf("MODE: %hu |", regs[R_PSR] >> 15);
    printf("PC: 0x%04x |", regs[R_PC]);
    printf("\n");
}

word mem_readw(word idx)
{
    return memory[idx + 1] << 8 | memory[idx];
}

void mem_writew(word w, word addr)
{
    memory[addr] = w & 0xFF;
    memory[addr + 1] = w >> 8;
}

int main(void)
{
    regs[R_R1] = 3;
    regs[R_R2] = 9;

    regs[R_PC] = 0;

    size_t psz = 0;
    word program[10] = {0};

    program[psz++] = OP_ADD << 12 | R_R0 << 9 | R_R1 << 6 | R_R2;
    program[psz++] = OP_ADD << 12 | R_R0 << 9 | R_R1 << 6 | 0x1 << 5 | 4 & 0x1F;
    program[psz++] = 69;

    for (size_t i = 0, j = 0; i < psz; ++i, j += 2) {
        memory[j] = program[i] & 0xFF;
        memory[j + 1] = program[i] >> 8;
    }

    debug_regs();

    word inst;
    while ((inst = mem_readw(regs[R_PC])) != 69) {
        regs[R_PC] += 2;
        word opcode = inst >> 12;
        switch (opcode) {
        case OP_ADD:
        case OP_AND: {
            word dst = inst >> 9 & 0x7;
            word src1 = inst >> 6 & 0x7;
            word src2 = inst & 0x7;

            if (inst >> 5 & 0x1) {
                src2 = sext(inst & 0x1F, 5);
            } else {
                src2 = regs[src2];
            }

            if (opcode == OP_ADD) {
                regs[dst] = regs[src1] + src2;
            } else {
                regs[dst] = regs[src1] & src2;
            }

            setcc(regs[dst]);
        } break;

        //case OP_NOT: {
        //    u16 dst = inst >> 9 & 0x7;
        //    u16 src = inst >> 6 & 0x7;
        //    regs[dst] = ~regs[src];
        //    setcc(regs[dst]);
        //} break;

        //case OP_ST: {
        //    u16 src = inst >> 9 & 0x7;
        //    u16 offset9 = sext(inst & 0x1FF, 9);
        //    memory[regs[REG_PC] + offset9] = regs[src];
        //} break;

        //case OP_STI: {
        //    u16 src = inst >> 9 & 0x7;
        //    u16 offset9 = sext(inst & 0x1FF, 9);
        //    u16 idx = regs[REG_PC] + offset9;
        //    u16 addr = memory[idx];
        //    memory[addr] = regs[src];
        //} break;

        //case OP_STR: {
        //    u16 src = inst >> 9 & 0x7;
        //    u16 base = inst >> 6 & 0x7;
        //    u16 offset6 = sext(inst & 0x3F, 6);
        //    u16 idx = regs[base] + offset6;
        //    memory[idx] = regs[src];
        //} break;

        //case OP_LD: {
        //    u16 dst = inst >> 9 & 0x7;
        //    u16 offset9 = sext(inst & 0x1FF, 9);
        //    regs[dst] = memory[regs[REG_PC] + offset9];
        //    setcc(regs[dst]);
        //} break;

        //case OP_LDI: {
        //    u16 dst = inst >> 9 & 0x7;
        //    u16 offset9 = sext(inst & 0x1FF, 9);
        //    u16 idx = regs[REG_PC] + offset9;
        //    u16 addr = memory[idx];
        //    regs[dst] = memory[addr];
        //    setcc(regs[dst]);
        //} break;

        //case OP_LDR: {
        //    u16 dst = inst >> 9 & 0x7;
        //    u16 base = inst >> 6 & 0x7;
        //    u16 offset6 = sext(inst & 0x3F, 6);
        //    u16 idx = regs[base] + offset6;
        //    regs[dst] = memory[idx];
        //    setcc(regs[dst]);
        //} break;

        //case OP_LEA: {
        //    u16 dst = inst >> 9 & 0x7;
        //    u16 offset9 = sext(inst & 0x1FF, 9);
        //    u16 addr = regs[REG_PC] + offset9;
        //    regs[dst] = addr;
        //    setcc(regs[dst]);
        //} break;

        //case OP_BR: {
        //    u16 nzp = inst >> 9 & 0x7;
        //    u16 offset9 = sext(inst & 0x1FF, 9);
        //    if (nzp == regs[REG_NZP]) {
        //        regs[REG_PC] += offset9;
        //    }
        //} break;

        //case OP_JMP: {
        //    u16 base = inst >> 6 & 0x7;
        //    regs[REG_PC] += regs[base];
        //} break;

        //case OP_TRAP: {
        //    u16 trapvec8 = inst & 0xFF;
        //    switch (trapvec8) {
        //        case 0x21: fprintf(stderr, "not implemented %4x\n", trapvec8); break;
        //        case 0x23: fprintf(stderr, "not implemented %4x\n", trapvec8); break;
        //        case 0x25: fprintf(stderr, "not implemented %4x\n", trapvec8); break;
        //        default: fprintf(stderr, "unknown syscall %4x\n", trapvec8);
        //    }
        //} break;

        //default:
        //    fprintf(stderr, "opcode %4x not implemented\n", opcode);
        }

        debug_regs();
    }

    return 0;
}
