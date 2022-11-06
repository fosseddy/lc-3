#include <stdio.h>

#define MEMORY_CAP (1<<16)

typedef unsigned short u16;

enum reg {
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

enum cc_flag {
    CC_POS  = 0x1,
    CC_ZERO = 0x2,
    CC_NEG  = 0x4
};

enum opcode {
    OPCODE_ADD = 1,
    OPCODE_LD = 2,
    OPCODE_ST = 3,
    OPCODE_AND = 5,
    OPCODE_LDR = 6,
    OPCODE_STR = 7,
    OPCODE_NOT = 9,
    OPCODE_LDI = 10,
    OPCODE_STI = 11,
    OPCODE_LEA = 14
};

static u16 regs[REG_COUNT] = {0};
static u16 memory[MEMORY_CAP] = {0};

u16 and(enum reg dst, enum reg src1, enum reg src2)
{
    return OPCODE_AND << 12 | dst << 9 | src1 << 6 | 0x0 << 3 | src2;
}

u16 and_imm(enum reg dst, enum reg src1, u16 imm5)
{
    return OPCODE_AND << 12 | dst << 9 | src1 << 6 | 0x1 << 5 | imm5 & 0x1F;
}

u16 add(enum reg dst, enum reg src1, enum reg src2)
{
    return OPCODE_ADD << 12 | dst << 9 | src1 << 6 | 0x0 << 3 | src2;
}

u16 add_imm(enum reg dst, enum reg src1, u16 imm5)
{
    return OPCODE_ADD << 12 | dst << 9 | src1 << 6 | 0x1 << 5 | imm5 & 0x1F;
}

u16 not(enum reg dst, enum reg src)
{
    return OPCODE_NOT << 12 | dst << 9 | src << 6 | 0x3F;
}

u16 load(enum reg dst, u16 offset9)
{
    return OPCODE_LD << 12 | dst << 9 | offset9 & 0x1FF;
}

u16 store(enum reg src, u16 offset9)
{
    return OPCODE_ST << 12 | src << 9 | offset9 & 0x1FF;
}

u16 loadi(enum reg dst, u16 offset9)
{
    return OPCODE_LDI << 12 | dst << 9 | offset9 & 0x1FF;
}

u16 storei(enum reg src, u16 offset9)
{
    return OPCODE_STI << 12 | src << 9 | offset9 & 0x1FF;
}

u16 loadr(enum reg dst, enum reg base, u16 offset6)
{
    return OPCODE_LDR << 12 | dst << 9 | base << 6 | offset6 & 0x3F;
}

u16 storer(enum reg src, enum reg base, u16 offset6)
{
    return OPCODE_STR << 12 | src << 9 | base << 6 | offset6 & 0x3F;
}

u16 lea(enum reg dst, u16 offset9)
{
    return OPCODE_LEA << 12 | dst << 9 | offset9 & 0x1FF;
}

void debug_regs()
{
    for (size_t i = 0; i < REG_COUNT; ++i) {
        printf("%5hu ", regs[i]);
    }
    printf("\n");
}

void debug_binary(u16 b, char *label)
{
    printf("%s: %016b\n", label, b);
}

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

    debug_regs();
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

        default:
            fprintf(stderr, "opcode %4x not implemented\n", opcode);
        }

        printf("\n");
        debug_regs();
    }

    printf("\nmemory:\n");
    for (size_t i = 0; i < 10; ++i) {
        printf("%5u ", memory[i]);
    }
    printf("\n");

    return 0;
}
