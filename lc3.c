#include <stdio.h>
#include <assert.h>

#define MEMORY_CAP (1 << 16)

typedef unsigned short u16;
typedef unsigned char u8;

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
    OP_LDB,
    OP_STB,
    OP_RESERVED1,
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
    OP_TRAP
};

static u16 regs[R_COUNT] = {0};
static u8 memory[MEMORY_CAP] = {0};

u16 sext(u16 value, size_t bit_len)
{
    if (value >> (bit_len - 1) & 0x1) {
        return 0xFFFF << bit_len | value;
    }

    return value;
}

void setcc(u16 value)
{
    u16 nzp;
    if (value == 0) {
        nzp = CC_Z;
    } else if (value >> 15) {
        nzp = CC_N;
    } else {
        nzp = CC_P;
    }

    regs[R_PSR] = (regs[R_PSR] & 0x8000) | nzp;
}

u16 mem_readw(u16 idx)
{
    return memory[idx + 1] << 8 | memory[idx];
}

void mem_writew(u16 w, u16 addr)
{
    memory[addr] = w & 0xFF;
    memory[addr + 1] = w >> 8;
}

int main(void)
{
    regs[R_R1] = 32769;

    size_t psz = 0;
    u16 program[10] = {0};

    program[psz++] = OP_SHF << 12 | R_R0 << 9 | R_R1 << 6 | 0x3 << 4 | 1 & 0xF;
    program[psz++] = 69;

    for (size_t i = 0, j = 0; i < psz; ++i, j += 2) {
        memory[j] = program[i] & 0xFF;
        memory[j + 1] = program[i] >> 8;
    }

    u16 inst;
    while ((inst = mem_readw(regs[R_PC])) != 69) {
        regs[R_PC] += 2;
        u16 opcode = inst >> 12;
        switch (opcode) {
        case OP_ADD:
        case OP_AND:
        case OP_XOR: {
            u16 dst = inst >> 9 & 0x7;
            u16 src1 = inst >> 6 & 0x7;
            u16 src2 = inst & 0x7;

            if (inst >> 5 & 0x1) {
                src2 = sext(inst & 0x1F, 5);
            } else {
                src2 = regs[src2];
            }

            switch (opcode) {
            case OP_ADD: regs[dst] = regs[src1] + src2; break;
            case OP_AND: regs[dst] = regs[src1] & src2; break;
            case OP_XOR: regs[dst] = regs[src1] ^ src2; break;
            default: assert(0 && "unreachable\n");
            }

            setcc(regs[dst]);
        } break;

        case OP_SHF: {
            u16 dst = inst >> 9 & 0x7;
            u16 src = inst >> 6 & 0x7;
            u16 amount4 = inst & 0xF;

            if (inst >> 4 & 0x1) {
                if (inst >> 5 & 0x1) {
                    u16 sign_bit = 0;
                    if (inst >> 15 & 0x1) {
                        sign_bit = 0xFFFF << (16 - amount4);
                    }
                    regs[dst] = sign_bit | regs[src] >> amount4;
                } else {
                    regs[dst] = regs[src] >> amount4;
                }
            } else {
                regs[dst] = regs[src] << amount4;
            }

            setcc(regs[dst]);
        } break;

        case OP_STB: {
            u16 src = inst >> 9 & 0x7;
            u16 base = inst >> 6 & 0x7;
            u16 boffset6 = sext(inst & 0x3F, 9);
            u16 idx = regs[base] + boffset6;
            memory[idx] = regs[src] & 0xFF;
        } break;

        case OP_LDB: {
            u16 dst = inst >> 9 & 0x7;
            u16 base = inst >> 6 & 0x7;
            u16 boffset6 = sext(inst & 0x3F, 6);
            u16 idx = regs[base] + boffset6;
            regs[dst] = sext(memory[idx], 8);
            setcc(regs[dst]);
        } break;

        case OP_STW: {
            u16 src = inst >> 9 & 0x7;
            u16 base = inst >> 6 & 0x7;
            u16 offset6 = sext(inst & 0x3F, 6) << 1;
            u16 idx = regs[base] + offset6;
            mem_writew(regs[src], idx);
            // @TODO(art): check regs[base] alignment
        } break;

        case OP_LDW: {
            u16 dst = inst >> 9 & 0x7;
            u16 base = inst >> 6 & 0x7;
            u16 offset6 = sext(inst & 0x3F, 6) << 1;
            u16 idx = regs[base] + offset6;
            regs[dst] = mem_readw(idx);
            setcc(regs[dst]);
            // @TODO(art): check regs[base] alignment
        } break;

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

        case OP_JMP: {
            u16 base = inst >> 6 & 0x7;
            regs[R_PC] = regs[base];
            // @TODO(art): check regs[base] alignment
        } break;

        //case OP_TRAP: {
        //    u16 trapvec8 = inst & 0xFF;
        //    switch (trapvec8) {
        //        case 0x21: fprintf(stderr, "not implemented %4x\n", trapvec8); break;
        //        case 0x23: fprintf(stderr, "not implemented %4x\n", trapvec8); break;
        //        case 0x25: fprintf(stderr, "not implemented %4x\n", trapvec8); break;
        //        default: fprintf(stderr, "unknown syscall %4x\n", trapvec8);
        //    }
        //} break;

        default: fprintf(stderr, "opcode %4x not implemented\n", opcode);
        }
    }

    return 0;
}
