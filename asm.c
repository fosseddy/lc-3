#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

#include "lc3.h"

#define MEM_MAKE(mem, type)                                         \
do {                                                                \
    (mem)->size = 0;                                                \
    (mem)->cap = 8;                                                 \
    if (((mem)->buf = malloc((mem)->cap * sizeof(type))) == NULL) { \
        perror("malloc");                                           \
        exit(1);                                                    \
    }                                                               \
} while (0)

#define MEM_GROW(mem, type)                                          \
do {                                                                 \
    if ((mem)->size == (mem)->cap) {                                 \
        (mem)->cap *= 2;                                             \
        (mem)->buf = realloc((mem)->buf, (mem)->cap * sizeof(type)); \
        if ((mem)->buf == NULL) {                                    \
            perror("realloc");                                       \
            exit(1);                                                 \
        }                                                            \
    }                                                                \
} while (0)

#define MEM_FREE(mem) free((mem)->buf)

enum token_kind {
    T_ERR,
    T_COMMA,
    T_NEWLINE,

    T_LABEL,
    T_IDENT,

    T_reg_begin,
    T_R0,
    T_R1,
    T_R2,
    T_R3,
    T_R4,
    T_R5,
    T_R6,
    T_R7,
    T_reg_end,

    T_instruction_begin,
    T_ORIG,
    T_FILL,
    T_BLKW,
    T_STRINGZ,

    T_BR,
    T_LD,
    T_ST,
    T_ADD,
    T_AND,
    T_BRN,
    T_BRZ,
    T_BRP,
    T_JMP,
    T_JSR,
    T_LDI,
    T_LDR,
    T_LEA,
    T_NOT,
    T_RET,
    T_RTI,
    T_STI,
    T_STR,
    T_BRNZ,
    T_BRNP,
    T_BRZP,
    T_JSRR,
    T_TRAP,
    T_BRNZP,

    T_IN,
    T_OUT,
    T_GETC,
    T_PUTS,
    T_HALT,
    T_PUTSP,
    T_instruction_end,

    T_DECIMAL,
    T_HEX,
    T_STRING
};

struct scanner {
    char *start;
    char *curr;
    size_t line;
    int add_newline;
};

struct token {
    enum token_kind kind;
    char *lexem;
    size_t len;
    size_t line;
    short unsigned lit;
};

struct tokens_array {
    size_t size;
    size_t cap;
    struct token *buf;
};

struct label {
    char *name;
    size_t len;
    size_t offset;
};

struct labels_array {
    size_t size;
    size_t cap;
    struct label *buf;
};

struct compiler {
    struct tokens_array *tokens;
    size_t start_addr;
    size_t addr_offset;
    size_t curr;
};

char *read_file(char *path)
{
    size_t fsize;
    char *src;
    FILE *f ;

    if ((f = fopen(path, "r")) == NULL) {
        perror("fopen");
        exit(1);
    }

    if (fseek(f, 0, SEEK_END) < 0) {
        perror("fseek");
        exit(1);
    }

    if ((int) (fsize = ftell(f)) < 0) {
        perror("ftell");
        exit(1);
    }

    rewind(f);

    if ((src = calloc(fsize + 1, sizeof(char))) == NULL) {
        perror("calloc");
        exit(1);
    }

    if (fread(src, sizeof(char), fsize, f) < fsize) {
        perror("fread");
        exit(1);
    }

    if (fclose(f) < 0) {
        perror("fclose");
        exit(1);
    }

    return src;
}

int has_chars(struct scanner *s)
{
    return *s->curr != '\0';
}

char advance(struct scanner *s)
{
    return *s->curr++;
}

char peek(struct scanner *s)
{
    return *s->curr;
}

int next(struct scanner *s, char c)
{
    if (!has_chars(s)) return 0;
    return peek(s) == c;
}

int is_instruction(enum token_kind kind)
{
    return kind > T_instruction_begin && kind < T_instruction_end;
}

int is_reg(enum token_kind kind)
{
    return kind > T_reg_begin && kind < T_reg_end;
}

int is_num(enum token_kind kind)
{
    return kind == T_DECIMAL || kind == T_HEX;
}

void make_token(struct scanner *s, struct token *t, enum token_kind kind)
{
    t->kind = kind;
    t->lexem = s->start;
    t->len = s->curr - s->start;
    t->line = s->line;

    if (is_num(kind)) {
        int base = 10;
        if (kind == T_HEX) {
            base = 16;
        }
        // @NOTE(art): enough for 16bit int
        char buf[6] = {0};
        strncpy(buf, t->lexem + 1, t->len - 1);
        t->lit = strtol(buf, NULL, base);
    }

    s->add_newline = 1;
}

void make_newline_token(struct scanner *s, struct token *t)
{
    t->kind = T_NEWLINE;
    t->lexem = "\\n";
    t->len = 2;
    t->line = s->line;

    s->add_newline = 0;
}

void tokens_put(struct tokens_array *ts, enum token_kind kind,
        struct scanner *s)
{
    MEM_GROW(ts, struct token);

    struct token *t = ts->buf + ts->size;
    if (kind == T_NEWLINE) {
        make_newline_token(s, t);
    } else {
        make_token(s, t, kind);
    }
    ts->size++;
}

void skip_whitespace(struct scanner *s, struct tokens_array *ts)
{
    for (;;) {
        switch (peek(s)) {
        case ' ':
        case '\t':
        case '\r':
            advance(s);
            break;

        case '\n':
            s->line++;
            if (s->add_newline) {
                tokens_put(ts, T_NEWLINE, s);
            }
            advance(s);
            break;

        default: return;
        }
    }
}

int has_tokens(struct compiler *c)
{
    return c->curr < c->tokens->size;
}

struct token *peek_token(struct compiler *c)
{
    return c->tokens->buf + c->curr;
}

struct token *advance_token(struct compiler *c)
{
    struct token *t = peek_token(c);
    if (has_tokens(c)) c->curr++;
    return t;
}

void report_compiler_error(struct token *t, char *msg)
{
    fprintf(stderr, "[line %lu] at %.*s: %s\n",
            t->line, t->len, t->lexem, msg);
}

void sync_compiler(struct compiler *c)
{
    while (has_tokens(c) && advance_token(c)->kind != T_NEWLINE);
}

enum lc3_reg get_reg(enum token_kind kind)
{
    switch (kind) {
    case T_R0: return R_R0;
    case T_R1: return R_R1;
    case T_R2: return R_R2;
    case T_R3: return R_R3;
    case T_R4: return R_R4;
    case T_R5: return R_R5;
    case T_R6: return R_R6;
    case T_R7: return R_R7;
    }
}

enum lc3_opcode get_opcode(enum token_kind kind)
{
    switch (kind) {
    case T_BR:
    case T_BRN:
    case T_BRZ:
    case T_BRP:
    case T_BRNZ:
    case T_BRNP:
    case T_BRZP:
    case T_BRNZP: return OP_BR;

    case T_IN:
    case T_OUT:
    case T_GETC:
    case T_PUTS:
    case T_HALT:
    case T_PUTSP:
    case T_TRAP: return OP_TRAP;

    case T_LD: return OP_LD;
    case T_ST: return OP_ST;
    case T_ADD: return OP_ADD;
    case T_AND: return OP_AND;
    case T_JMP: return OP_JMP;
    case T_JSR: return OP_JSR;
    case T_LDI: return OP_LDI;
    case T_LDR: return OP_LDR;
    case T_LEA: return OP_LEA;
    case T_NOT: return OP_NOT;
    case T_RET: return OP_JMP;
    case T_RTI: return OP_RTI;
    case T_STI: return OP_STI;
    case T_STR: return OP_STR;
    case T_JSRR: return OP_JSR;
    }
}

struct label *get_label(struct labels_array *ls, struct token *t)
{
    struct label *found = NULL;
    for (size_t i = 0; i < ls->size; ++i) {
        struct label *l = ls->buf + i;
        if (l->len == t->len && memcmp(l->name, t->lexem, t->len) == 0) {
            found = l;
            break;
        }
    }
    return found;
}

struct token *consume(struct compiler *c, enum token_kind kind, char *msg)
{
    struct token *t = advance_token(c);
    if (t->kind != kind) {
        report_compiler_error(t, msg);
        sync_compiler(c);
        return NULL;
    }
    return t;
}

struct token *consume_reg(struct compiler *c)
{
    struct token *t = advance_token(c);
    if (!is_reg(t->kind)) {
        report_compiler_error(t, "expected register");
        sync_compiler(c);
        return NULL;
    }
    return t;
}

struct token *consume_num(struct compiler *c)
{
    struct token *t = advance_token(c);
    if (!is_num(t->kind)) {
        report_compiler_error(t, "expected number");
        sync_compiler(c);
        return NULL;
    }
    return t;
}

struct token *consume_reg_or_num(struct compiler *c)
{
    struct token *t = advance_token(c);
    if (!is_reg(t->kind) && !is_num(t->kind)) {
        report_compiler_error(t, "expected register or number");
        sync_compiler(c);
        return NULL;
    }
    return t;
}

struct token *consume_comma(struct compiler *c)
{
    struct token *t = advance_token(c);
    if (t->kind != T_COMMA) {
        report_compiler_error(t, "expected comma");
        sync_compiler(c);
        return NULL;
    }
    return t;
}

struct label *consume_label(struct compiler *c, struct labels_array *ls)
{
    struct token *ident = consume(c, T_IDENT, "expected label");
    if (!ident) return NULL;

    struct label *found = get_label(ls, ident);
    if (!found) {
        report_compiler_error(ident, "label does not exist");
        sync_compiler(c);
        return NULL;
    }
    return found;
}

size_t calc_offset(struct compiler *c, struct label *ident)
{
    size_t label_addr = c->start_addr + ident->offset;
    size_t curr_addr = c->start_addr + c->addr_offset + 1;
    return label_addr - curr_addr;
}

int main(void)
{
    // @TODO(art): get filepath from args
    // @LEAK(art): let OS free it for now
    char *src = read_file("./ex.asm");

    for (char *p = src; *p; p++) *p = tolower(*p);

    struct scanner s = {
        .start = src,
        .curr = src,
        .line = 1,
        .add_newline = 0
    };

    // @LEAK(art): let OS free it for now
    struct tokens_array tokens;
    MEM_MAKE(&tokens, struct token);

    int stop_scanning = 0;

    for (;;) {
        skip_whitespace(&s, &tokens);
        if (!has_chars(&s) || stop_scanning) break;

        s.start = s.curr;
        char c = advance(&s);

        // @NOTE(art): kwds, regs, labels
        if (c != 'x' && isalpha(c)) {
            while (isalnum(peek(&s))) advance(&s);

            enum token_kind kind = T_IDENT;

            switch (s.curr - s.start) {
            case 2:
                switch (*s.start) {
                case 'r':
                    switch (*(s.start + 1)) {
                    case '0': kind = T_R0; break;
                    case '1': kind = T_R1; break;
                    case '2': kind = T_R2; break;
                    case '3': kind = T_R3; break;
                    case '4': kind = T_R4; break;
                    case '5': kind = T_R5; break;
                    case '6': kind = T_R6; break;
                    case '7': kind = T_R7; break;
                    }
                    break;
                case 'b':
                    if (*(s.start + 1) == 'r') kind = T_BR;
                    break;
                case 'i':
                    if (*(s.start + 1) == 'n') kind = T_IN;
                    break;
                case 'l':
                    if (*(s.start + 1) == 'd') kind = T_LD;
                    break;
                case 's':
                    if (*(s.start + 1) == 't') kind = T_ST;
                    break;
                }
                break;

            case 3:
                switch (*s.start) {
                case 'a':
                    if (memcmp(s.start + 1, "dd", 2) == 0) {
                        kind = T_ADD;
                    } else if (memcmp(s.start + 1, "nd", 2) == 0) {
                        kind = T_AND;
                    }
                    break;
                case 'b':
                    if (memcmp(s.start + 1, "rn", 2) == 0) {
                        kind = T_BRN;
                    } else if (memcmp(s.start + 1, "rz", 2) == 0) {
                        kind = T_BRZ;
                    } else if (memcmp(s.start + 1, "rp", 2) == 0) {
                        kind = T_BRP;
                    }
                    break;
                case 'j':
                    if (memcmp(s.start + 1, "mp", 2) == 0) {
                        kind = T_JMP;
                    } else if (memcmp(s.start + 1, "sr", 2) == 0) {
                        kind = T_JSR;
                    }
                    break;
                case 'l':
                    if (memcmp(s.start + 1, "di", 2) == 0) {
                        kind = T_LDI;
                    } else if (memcmp(s.start + 1, "dr", 2) == 0) {
                        kind = T_LDR;
                    } else if (memcmp(s.start + 1, "ea", 2) == 0) {
                        kind = T_LEA;
                    }
                    break;
                case 'n':
                    if (memcmp(s.start + 1, "ot", 2) == 0) kind = T_NOT;
                    break;
                case 'r':
                    if (memcmp(s.start + 1, "et", 2) == 0) {
                        kind = T_RET;
                    } else if (memcmp(s.start + 1, "ti", 2) == 0) {
                        kind = T_RTI;
                    }
                    break;
                case 's':
                    if (memcmp(s.start + 1, "ti", 2) == 0) {
                        kind = T_STI;
                    } else if (memcmp(s.start + 1, "tr", 2) == 0) {
                        kind = T_STR;
                    }
                    break;
                case 'o':
                    if (memcmp(s.start + 1, "ut", 2) == 0) kind = T_OUT;
                    break;
                }
                break;

            case 4:
                switch (*s.start) {
                case 'b':
                    if (memcmp(s.start + 1, "rnz", 3) == 0) {
                        kind = T_BRNZ;
                    } else if (memcmp(s.start + 1, "rnp", 3) == 0) {
                        kind = T_BRNP;
                    } else if (memcmp(s.start + 1, "rzp", 3) == 0) {
                        kind = T_BRZP;
                    }
                    break;
                case 'j':
                    if (memcmp(s.start + 1, "srr", 3) == 0) kind = T_JSRR;
                    break;
                case 't':
                    if (memcmp(s.start + 1, "rap", 3) == 0) kind = T_TRAP;
                    break;
                case 'h':
                    if (memcmp(s.start + 1, "alt", 3) == 0) kind = T_HALT;
                    break;
                case 'g':
                    if (memcmp(s.start + 1, "etc", 3) == 0) kind = T_GETC;
                    break;
                case 'p':
                    if (memcmp(s.start + 1, "uts", 3) == 0) kind = T_PUTS;
                    break;
                }
                break;

            case 5:
                switch (*s.start) {
                case 'b':
                    if (memcmp(s.start + 1, "rnzp", 4) == 0) kind = T_BRNZP;
                    break;
                }
                break;
            }

            if (kind == T_IDENT) {
                if (tokens.size == 0 ||
                        tokens.buf[tokens.size - 1].kind == T_NEWLINE) {
                    kind = T_LABEL;
                }
            }

            tokens_put(&tokens, kind, &s);
            continue;
        }

        switch (c) {
        case ';':
            while (!next(&s, '\n')) advance(&s);
            break;

        case ',':
            tokens_put(&tokens, T_COMMA, &s);
            break;

        case '"':
            advance(&s);
            while (!next(&s, '"')) advance(&s);
            advance(&s);
            tokens_put(&tokens, T_STRING, &s);
            break;

        case '#':
            if (next(&s, '-')) advance(&s);
            while (isdigit(peek(&s))) advance(&s);

            tokens_put(&tokens, T_DECIMAL, &s);
            break;

        case 'x':
            if (next(&s, '-')) advance(&s);
            while (isxdigit(peek(&s))) advance(&s);

            tokens_put(&tokens, T_HEX, &s);
            break;

        case '.':
            while (isalnum(peek(&s))) advance(&s);

            enum token_kind kind = T_ERR;

            switch (s.curr - s.start) {
            case 4:
                if (memcmp(s.start + 1, "end", 3) == 0) {
                    stop_scanning = 1;
                    continue;
                }
                break;
            case 5:
                if (memcmp(s.start + 1, "orig", 4) == 0) {
                    kind = T_ORIG;
                } else if (memcmp(s.start + 1, "fill", 4) == 0) {
                    kind = T_FILL;
                } else if (memcmp(s.start + 1, "blkw", 4) == 0) {
                    kind = T_BLKW;
                }
                break;
            case 8:
                if (memcmp(s.start + 1, "stringz", 7) == 0) kind = T_STRINGZ;
                break;
            }

            // @TODO(art): handle error
            assert(kind != T_ERR);
            tokens_put(&tokens, kind, &s);
            break;

        default: printf("Unknown char: '%c'\n", c);
        }
    }

    // @TODO(art): -1 to ignore .orig directive, we assume it always present.
    // Make it so it does not matter if directive was used, as default address
    // for user programs is 0x3000.
    int addr_offset = -1;

    // @LEAK(art): let OS free it for now
    struct labels_array labels;
    MEM_MAKE(&labels, struct label);

    for (size_t i = 0; i < tokens.size; ++i) {
        struct token *t = tokens.buf + i;

        switch (t->kind) {
        case T_NEWLINE:
            addr_offset += 1;
            break;

        case T_LABEL:;
            MEM_GROW(&labels, struct label);
            labels.buf[labels.size++] = (struct label) {
                .name = t->lexem,
                .len = t->len,
                .offset = (size_t) addr_offset
            };
            break;
        }
    }

    struct compiler c = {
        .tokens = &tokens,
        .start_addr = 0x3000,
        .addr_offset = 0,
        .curr = 0
    };

    FILE *out = fopen("out.obj", "wb");
    // @TODO(art): handle error
    assert(out != NULL);

    while (has_tokens(&c)) {
        if (peek_token(&c)->kind == T_LABEL) {
            advance_token(&c);
        }

        struct token *opcode = advance_token(&c);
        if (opcode->kind == T_NEWLINE) continue;
        if (!is_instruction(opcode->kind)) {
            report_compiler_error(opcode, "unknown instruction");
            sync_compiler(&c);
            continue;
        }

        switch (opcode->kind) {
        case T_ORIG: {
            if (c.curr > 1) {
                report_compiler_error(opcode, "must be top level");
                sync_compiler(&c);
                continue;
            }
            struct token *addr = consume_num(&c);
            if (!addr) continue;
            c.start_addr = addr->lit;
            c.addr_offset -= 1;
            fwrite(&addr->lit, sizeof(u16), 1, out);
        } break;

        case T_FILL: {
            struct token *value = consume_num(&c);
            if (!value) continue;
            fwrite(&value->lit, sizeof(u16), 1, out);
        } break;

        case T_BLKW: {
            struct token *value = consume_num(&c);
            if (!value) continue;

            u16 zero = 0;
            for (size_t i = 0; i < value->lit; ++i) {
                fwrite(&zero, sizeof(zero), 1, out);
            }
        } break;

        case T_STRINGZ: {
            struct token *str = consume(&c, T_STRING, "expected string");
            if (!str) continue;

            for (size_t i = 1; i < str->len - 1; ++i) {
                u16 c = str->lexem[i];
                fwrite(&c, sizeof(c), 1, out);
            }
            u16 null = '\0';
            fwrite(&null, sizeof(null), 1, out);
        } break;

        case T_ADD:
        case T_AND: {
            struct token *dst, *src1, *src2;

            if (!(dst = consume_reg(&c))) continue;
            if (!consume_comma(&c)) continue;

            if (!(src1 = consume_reg(&c))) continue;
            if (!consume_comma(&c)) continue;

            if (!(src2 = consume_reg_or_num(&c))) continue;

            u16 op = get_opcode(opcode->kind) << 12;
            op |= get_reg(dst->kind) << 9;
            op |= get_reg(src1->kind) << 6;

            if (is_reg(src2->kind)) {
                op |= get_reg(src2->kind);
            } else {
                op |= 0x1 << 5;
                op |= src2->lit & 0x1F;
            }

            fwrite(&op, sizeof(op), 1, out);
        } break;

        case T_BRNZP:
        case T_BRNZ:
        case T_BRNP:
        case T_BRZP:
        case T_BRN:
        case T_BRZ:
        case T_BRP:
        case T_BR: {
            struct label *ident = consume_label(&c, &labels);
            if (!ident) continue;

            unsigned nzp;
            switch (opcode->kind) {
            case T_BRNZP: nzp = 0x7; break;
            case T_BRNZ: nzp = 0x6; break;
            case T_BRNP: nzp = 0x5; break;
            case T_BRZP: nzp = 0x3; break;
            case T_BRN: nzp = 0x4; break;
            case T_BRZ: nzp = 0x2; break;
            case T_BRP: nzp = 0x1; break;
            case T_BR: nzp = 0x7; break;
            }

            u16 op = get_opcode(opcode->kind) << 12;
            op |= nzp << 9;
            op |= calc_offset(&c, ident) & 0x1FF;
            fwrite(&op, sizeof(op), 1, out);
        } break;

        case T_JMP: {
            struct token *base = consume_reg(&c);
            if (!base) continue;

            u16 op = get_opcode(opcode->kind) << 12;
            op |= get_reg(base->kind) << 6;
            fwrite(&op, sizeof(op), 1, out);
        } break;

        case T_RET: {
            u16 op = get_opcode(opcode->kind) << 12;
            op |= 0x7 << 6;
            fwrite(&op, sizeof(op), 1, out);
        } break;

        case T_JSR: {
            struct label *ident = consume_label(&c, &labels);
            if (!ident) continue;

            u16 op = get_opcode(opcode->kind) << 12;
            op |= 1 << 11;
            op |= calc_offset(&c, ident) & 0x7FF;
            fwrite(&op, sizeof(op), 1, out);
        } break;

        case T_JSRR: {
            struct token *base = consume_reg(&c);
            if (!base) continue;

            u16 op = get_opcode(opcode->kind) << 12;
            op |= get_reg(base->kind) << 6;
            fwrite(&op, sizeof(op), 1, out);
        } break;

        case T_LD:
        case T_ST:
        case T_LDI:
        case T_STI: {
            struct token *reg;
            struct label *ident;

            if (!(reg = consume_reg(&c))) continue;
            if (!consume_comma(&c)) continue;

            if (!(ident = consume_label(&c, &labels))) continue;

            u16 op = get_opcode(opcode->kind) << 12;
            op |= get_reg(reg->kind) << 9;
            op |= calc_offset(&c, ident) & 0x1FF;
            fwrite(&op, sizeof(op), 1, out);
        } break;

        case T_LDR:
        case T_STR: {
            struct token *reg, *base, *offset;

            if (!(reg = consume_reg(&c))) continue;
            if (!consume_comma(&c)) continue;

            if (!(base = consume_reg(&c))) continue;
            if (!consume_comma(&c)) continue;

            if (!(offset = consume_num(&c))) continue;

            u16 op = get_opcode(opcode->kind) << 12;
            op |= get_reg(reg->kind) << 9;
            op |= get_reg(base->kind) << 6;
            op |= offset->lit & 0x3F;
            fwrite(&op, sizeof(op), 1, out);
        } break;

        case T_LEA: {
            struct token *dst;
            struct label *ident;

            if (!(dst = consume_reg(&c))) continue;
            if (!consume_comma(&c)) continue;

            if (!(ident = consume_label(&c, &labels))) continue;

            u16 op = get_opcode(opcode->kind) << 12;
            op |= get_reg(dst->kind) << 9;
            op |= calc_offset(&c, ident) & 0x1FF;
            fwrite(&op, sizeof(op), 1, out);
        } break;

        case T_NOT: {
            struct token *dst, *src;

            if (!(dst = consume_reg(&c))) continue;
            if (!consume_comma(&c)) continue;

            if (!(src = consume_reg(&c))) continue;
            if (!consume_comma(&c)) continue;

            u16 op = get_opcode(opcode->kind) << 12;
            op |= get_reg(dst->kind) << 9;
            op |= get_reg(src->kind) << 6;
            op |= 0x3F;
            fwrite(&op, sizeof(op), 1, out);
        } break;

        case T_RTI: {
            u16 op = get_opcode(opcode->kind) << 12;
            fwrite(&op, sizeof(op), 1, out);
        } break;

        case T_TRAP: {
            struct token *trapvec = consume_num(&c);
            if (!trapvec) continue;

            u16 op = get_opcode(opcode->kind) << 12;
            op |= trapvec->lit & 0xFF;
            fwrite(&op, sizeof(op), 1, out);
        } break;

        case T_IN: {
            u16 op = get_opcode(opcode->kind) << 12;
            op |= 0x23;
            fwrite(&op, sizeof(op), 1, out);
        } break;

        case T_OUT: {
            u16 op = get_opcode(opcode->kind) << 12;
            op |= 0x21;
            fwrite(&op, sizeof(op), 1, out);
        } break;

        case T_GETC: {
            u16 op = get_opcode(opcode->kind) << 12;
            op |= 0x20;
            fwrite(&op, sizeof(op), 1, out);
        } break;

        case T_PUTS: {
            u16 op = get_opcode(opcode->kind) << 12;
            op |= 0x22;
            fwrite(&op, sizeof(op), 1, out);
        } break;

        case T_HALT: {
            u16 op = get_opcode(opcode->kind) << 12;
            op |= 0x25;
            fwrite(&op, sizeof(op), 1, out);
        } break;

        case T_PUTSP: {
            u16 op = get_opcode(opcode->kind) << 12;
            op |= 0x24;
            fwrite(&op, sizeof(op), 1, out);
        } break;
        }

        if (!consume(&c, T_NEWLINE, "expected new line after instruction"))
            continue;

        c.addr_offset += 1;
    }

    fflush(out);

    return 0;
}
