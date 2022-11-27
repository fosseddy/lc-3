#include <stdio.h>
#include <assert.h>

#include "lc3.h"

#define MEMORY_CAP (1 << 16)

enum {
    CC_P = 0x1,
    CC_Z = 0x2,
    CC_N = 0x4
};

static u16 regs[R_COUNT];
static u16 memory[MEMORY_CAP];

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

// @TODO(art): proper object file loading
// @TODO(art): init memory, PC, etc
int main(void)
{
    FILE *f = fopen("out.obj", "rb");
    if (f == NULL) {
        perror("fopen");
        return 1;
    }

    fread(regs + R_PC, sizeof(u16), 1, f);

    u16 op;
    size_t offset = regs[R_PC];
    while (fread(&op, sizeof(op), 1, f) > 0) {
        memory[offset++] = op;
    }

    int is_halted = 0;
    while (!is_halted) {
        u16 inst = memory[regs[R_PC]];
        regs[R_PC]++;
        u16 opcode = inst >> 12;
        switch (opcode) {
        case OP_ADD:
        case OP_AND: {
            u16 dst = inst >> 9 & 0x7;
            u16 src1 = inst >> 6 & 0x7;
            u16 src2 = inst & 0x7;

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

        case OP_BR: {
            u16 nzp = inst >> 9 & 0x7;
            u16 pcoffset9 = sext(inst & 0x1FF, 9);

            if ((nzp & CC_P) && (regs[R_PSR] & CC_P) ||
                    (nzp & CC_Z) && (regs[R_PSR] & CC_Z) ||
                    (nzp & CC_N) && (regs[R_PSR] & CC_N)) {
                regs[R_PC] += pcoffset9;
            }
        } break;

        case OP_JMP: {
            u16 base = inst >> 6 & 0x7;
            regs[R_PC] = regs[base];
        } break;

        case OP_JSR: {
            regs[R_R7] = regs[R_PC];

            if (inst >> 11 & 0x1) {
                u16 pcoffset11 = sext(inst & 0x7FF, 11);
                regs[R_PC] += pcoffset11;
            } else {
                u16 base = inst >> 6 & 0x7;
                regs[R_PC] = regs[base];
            }
        } break;

        case OP_LD: {
            u16 dst = inst >> 9 & 0x7;
            u16 pcoffset9 = sext(inst & 0x1FF, 9);
            regs[dst] = memory[regs[R_PC] + pcoffset9];
            setcc(regs[dst]);
        } break;

        case OP_LDI: {
            u16 dst = inst >> 9 & 0x7;
            u16 pcoffset9 = sext(inst & 0x1FF, 9);
            u16 addr = memory[regs[R_PC] + pcoffset9];
            regs[dst] = memory[addr];
            setcc(regs[dst]);
        } break;

        case OP_LDR: {
            u16 dst = inst >> 9 & 0x7;
            u16 base = inst >> 6 & 0x7;
            u16 offset6 = sext(inst & 0x2F, 6);
            regs[dst] = memory[regs[base] + offset6];
            setcc(regs[dst]);
        } break;

        case OP_LEA: {
            u16 dst = inst >> 9 & 0x7;
            u16 pcoffset9 = sext(inst & 0x1FF, 9);
            regs[dst] = regs[R_PC] + pcoffset9;
        } break;

        case OP_NOT: {
            u16 dst = inst >> 9 & 0x7;
            u16 src = inst >> 6 & 0x7;
            regs[dst] = ~regs[src];
            setcc(regs[dst]);
        } break;

        case OP_RTI: {
            regs[R_PC] = memory[regs[R_R6]];
            regs[R_R6]++;
            regs[R_PSR] = memory[regs[R_R6]];
            regs[R_R6]++;
        } break;

        case OP_ST: {
            u16 src = inst >> 9 & 0x7;
            u16 pcoffset9 = sext(inst & 0x1FF, 9);
            memory[regs[R_PC] + pcoffset9] = regs[src];
        } break;

        case OP_STI: {
            u16 src = inst >> 9 & 0x7;
            u16 pcoffset9 = sext(inst & 0x1FF, 9);
            u16 addr = memory[regs[R_PC] + pcoffset9];
            memory[addr] = regs[src];
        } break;

        case OP_STR: {
            u16 src = inst >> 9 & 0x7;
            u16 base = inst >> 6 & 0x7;
            u16 offset6 = sext(inst & 0x2F, 6);
            memory[regs[base] + offset6] = regs[src];
        } break;

        case OP_TRAP: {
            u16 trapvec8 = inst & 0xFF;
            switch (trapvec8) {
            case 0x21: {
                char c = regs[R_R0] & 0xFF;
                putchar(c);
            } break;
            case 0x22: {
                u16 addr = regs[R_R0];
                while (memory[addr] != '\0') {
                    putchar(memory[addr++]);
                }
            } break;
            case 0x25:
                is_halted = 1;
                puts("lc3 is halted");
                break;
            }
        } break;

        default: fprintf(stderr, "opcode %4x not implemented\n", opcode);
        }
    }

    return 0;
}
