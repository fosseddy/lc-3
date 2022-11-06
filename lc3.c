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

enum opcode {
    OPCODE_ADD = 1,
    OPCODE_AND = 5,
    OPCODE_NOT = 9
};

static u16 regs[REG_COUNT] = {0};
static u16 memory[MEMORY_CAP] = {0};

void setcc(u16 value)
{
    u16 nzp = 0;
    if (value == 0) {
        nzp = nzp | 0x2;
    } else if (value >> 15 & 0x1) { // negative
        nzp = nzp | 0x4;
    } else {
        nzp = nzp | 0x1;
    }
    regs[REG_NZP] = nzp;
}

u16 and(u16 dst, u16 src1, u16 src2)
{
    return OPCODE_AND << 12 | dst << 9 | src1 << 6 | 0x0 << 3 | src2;
}

u16 and_imm(u16 dst, u16 src1, u16 imm5)
{
    return OPCODE_AND << 12 | dst << 9 | src1 << 6 | 0x1 << 5 | imm5 & 0x1F;
}

u16 add(u16 dst, u16 src1, u16 src2)
{
    return OPCODE_ADD << 12 | dst << 9 | src1 << 6 | 0x0 << 3 | src2;
}

u16 add_imm(u16 dst, u16 src1, u16 imm5)
{
    return OPCODE_ADD << 12 | dst << 9 | src1 << 6 | 0x1 << 5 | imm5 & 0x1F;
}

u16 not(u16 dst, u16 src)
{
    return OPCODE_NOT << 12 | dst << 9 | src << 6 | 0x3F;
}

void debug_regs()
{
    for (size_t i = 0; i < REG_COUNT; ++i) {
        printf("%5u ", regs[i]);
    }
    printf("\n");
}

void debug_binary(u16 b, char *label)
{
    printf("%s: %016b\n", label, b);
}

int main(void)
{
    u16 program[10] = {0};
    size_t psz = 0;

    program[psz++] = and_imm(REG_R1, REG_R0, 0);
    program[psz++] = add_imm(REG_R1, REG_R1, 7);
    program[psz++] = not(REG_R2, REG_R1);

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
                src2 = inst & 0x1F;
                if (src2 >> 4 & 0x1) {
                    src2 = src2 | ~0x1F;
                }
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

        default:
            fprintf(stderr, "opcode %4x not implemented\n", opcode);
        }

        printf("\n");
        debug_regs();
    }

    return 0;
}
