#include <stdio.h>

#define MEMORY_CAP (1<<16)

typedef unsigned short u16;

enum {
    REG_R0 = 0,
    REG_R1,
    REG_R2,
    REG_R3,
    REG_R4,
    REG_R5,
    REG_R6,
    REG_R7,

    REG_PC,
    REG_NZP,

    REG_COUNT
};

enum {
    CC_POS  = 0x1,
    CC_ZERO = 0x2,
    CC_NEG  = 0x4
};

enum {
    OPCODE_BR = 0,
    OPCODE_ADD,
    OPCODE_LD,
    OPCODE_ST,
    OPCODE_RESERVED1,
    OPCODE_AND,
    OPCODE_LDR,
    OPCODE_STR,
    OPCODE_RESERVED2,
    OPCODE_NOT,
    OPCODE_LDI,
    OPCODE_STI,
    OPCODE_JMP,
    OPCODE_RESERVED3,
    OPCODE_LEA,
    OPCODE_TRAP
};

static u16 regs[REG_COUNT] = {0};
static u16 memory[MEMORY_CAP] = {0};

u16 sign_extend(u16 value, size_t bit_len)
{
    if (value >> (bit_len - 1) & 0x1) {
        return value | 0xFFFF << bit_len;
    }

    return value;
}

void setcc(u16 value)
{
    u16 nzp;
    if (value == 0) {
        nzp = CC_ZERO;
    } else if (value >> 15) {
        nzp = CC_NEG;
    } else {
        nzp = CC_POS;
    }
    regs[REG_NZP] = nzp;
}

int main(void)
{
    size_t psz = 0;
    u16 program[10] = {0};
    program[psz++] = 0;

    regs[REG_PC] = 0;

    for (size_t i = 0; regs[REG_PC] < psz; ++i) {
        u16 inst = program[regs[REG_PC]++];
        u16 opcode = inst >> 12;
        switch (opcode) {
        case OPCODE_ADD:
        case OPCODE_AND: {
            u16 dst = inst >> 9 & 0x7;
            u16 src1 = inst >> 6 & 0x7;
            u16 src2 = inst & 0x7;

            if (inst >> 5 & 0x1) {
                src2 = sign_extend(inst & 0x1F, 5);
            } else {
                src2 = regs[src2];
            }

            if (opcode == OPCODE_ADD) {
                regs[dst] = regs[src1] + src2;
            } else {
                regs[dst] = regs[src1] & src2;
            }

            setcc(regs[dst]);
        } break;

        case OPCODE_NOT: {
            u16 dst = inst >> 9 & 0x7;
            u16 src = inst >> 6 & 0x7;
            regs[dst] = ~regs[src];
            setcc(regs[dst]);
        } break;

        case OPCODE_ST: {
            u16 src = inst >> 9 & 0x7;
            u16 offset9 = sign_extend(inst & 0x1FF, 9);
            memory[regs[REG_PC] + offset9] = regs[src];
        } break;

        case OPCODE_STI: {
            u16 src = inst >> 9 & 0x7;
            u16 offset9 = sign_extend(inst & 0x1FF, 9);
            u16 idx = regs[REG_PC] + offset9;
            u16 addr = memory[idx];
            memory[addr] = regs[src];
        } break;

        case OPCODE_STR: {
            u16 src = inst >> 9 & 0x7;
            u16 base = inst >> 6 & 0x7;
            u16 offset6 = sign_extend(inst & 0x3F, 6);
            u16 idx = regs[base] + offset6;
            memory[idx] = regs[src];
        } break;

        case OPCODE_LD: {
            u16 dst = inst >> 9 & 0x7;
            u16 offset9 = sign_extend(inst & 0x1FF, 9);
            regs[dst] = memory[regs[REG_PC] + offset9];
            setcc(regs[dst]);
        } break;

        case OPCODE_LDI: {
            u16 dst = inst >> 9 & 0x7;
            u16 offset9 = sign_extend(inst & 0x1FF, 9);
            u16 idx = regs[REG_PC] + offset9;
            u16 addr = memory[idx];
            regs[dst] = memory[addr];
            setcc(regs[dst]);
        } break;

        case OPCODE_LDR: {
            u16 dst = inst >> 9 & 0x7;
            u16 base = inst >> 6 & 0x7;
            u16 offset6 = sign_extend(inst & 0x3F, 6);
            u16 idx = regs[base] + offset6;
            regs[dst] = memory[idx];
            setcc(regs[dst]);
        } break;

        case OPCODE_LEA: {
            u16 dst = inst >> 9 & 0x7;
            u16 offset9 = sign_extend(inst & 0x1FF, 9);
            u16 addr = regs[REG_PC] + offset9;
            regs[dst] = addr;
            setcc(regs[dst]);
        } break;

        case OPCODE_BR: {
            u16 nzp = inst >> 9 & 0x7;
            u16 offset9 = sign_extend(inst & 0x1FF, 9);
            if (nzp == regs[REG_NZP]) {
                regs[REG_PC] += offset9;
            }
        } break;

        case OPCODE_JMP: {
            u16 base = inst >> 6 & 0x7;
            regs[REG_PC] += regs[base];
        } break;

        case OPCODE_TRAP: {
            u16 trapvec8 = inst & 0xFF;
            switch (trapvec8) {
                case 0x21: fprintf(stderr, "not implemented %4x\n", trapvec8); break;
                case 0x23: fprintf(stderr, "not implemented %4x\n", trapvec8); break;
                case 0x25: fprintf(stderr, "not implemented %4x\n", trapvec8); break;
                default: fprintf(stderr, "unknown syscall %4x\n", trapvec8);
            }
        } break;

        default:
            fprintf(stderr, "opcode %4x not implemented\n", opcode);
        }
    }

    return 0;
}
