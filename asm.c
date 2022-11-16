#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

#define ARRAY_MAKE(arr, type)                                         \
do {                                                                  \
    (arr)->size = 0;                                                  \
    (arr)->cap = 8;                                                   \
    if (((arr)->items = malloc((arr)->cap * sizeof(type))) == NULL) { \
        perror("malloc");                                             \
        exit(1);                                                      \
    }                                                                 \
} while (0)

#define ARRAY_PUT(arr, type, value)                                      \
do {                                                                     \
    if ((arr)->size == (arr)->cap) {                                     \
        (arr)->cap *= 2;                                                 \
        (arr)->items = realloc((arr)->items, (arr)->cap * sizeof(type)); \
        if ((arr)->items == NULL) {                                      \
            perror("realloc");                                           \
            exit(1);                                                     \
        }                                                                \
    }                                                                    \
    (arr)->items[(arr)->size++] = value;                                 \
} while (0)

#define ARRAY_FREE(arr) free((arr)->items)

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

    T_opcode_begin,
    T_BR,
    T_IN,

    T_ADD,
    T_AND,
    T_BRN,
    T_BRZ,
    T_BRP,
    T_JMP,
    T_JSR,
    T_LDB,
    T_LDW,
    T_LEA,
    T_NOP,
    T_NOT,
    T_RET,
    T_RTI,
    T_STB,
    T_STW,
    T_XOR,
    T_OUT,

    T_BRNZ,
    T_BRNP,
    T_BRZP,
    T_JSRR,
    T_LSHF,
    T_TRAP,
    T_HALT,
    T_GETC,
    T_PUTS,

    T_RSHFL,
    T_RSHFA,
    T_BRNZP,

    T_ORIG,
    T_FILL,
    T_BLKW,
    T_STRINGZ,
    T_END,
    T_opcode_end,

    T_DECIMAL,
    T_HEX
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
};

struct tokens_array {
    size_t size;
    size_t cap;
    struct token *items;
};

struct label {
    char *name;
    size_t len;
    size_t addr;
};

struct labels_array {
    size_t size;
    size_t cap;
    struct label *items;
};

struct parser {
    struct tokens_array tokens;
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

void make_token(struct token *t, enum token_kind kind, struct scanner *s)
{
    t->kind = kind;
    t->lexem = s->start;
    t->len = s->curr - s->start;
    t->line = s->line;

    s->add_newline = 1;
}

void make_newline_token(struct token *t, struct scanner *s)
{
    t->kind = T_NEWLINE;
    t->lexem = "\\n";
    t->len = 2;
    t->line = s->line;

    s->add_newline = 0;
}

void tokens_put(struct tokens_array *ts, struct scanner *s,
                enum token_kind kind)
{
    struct token tmp;
    if (kind == T_NEWLINE) {
        make_newline_token(&tmp, s);
    } else {
        make_token(&tmp, kind, s);
    }
    ARRAY_PUT(ts, struct token, tmp);
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
                tokens_put(ts, s, T_NEWLINE);
            }
            advance(s);
            break;

        default: return;
        }
    }
}

int is_opcode(enum token_kind kind)
{
    return kind > T_opcode_begin && kind < T_opcode_end;
}

int is_reg(enum token_kind kind)
{
    return kind > T_reg_begin && kind < T_reg_end;
}

void report_parser_error(struct token *t, char *msg)
{
    fprintf(stderr, "[line %lu] at %.*s: %s\n",
            t->line, t->len, t->lexem, msg);
}

int has_tokens(struct parser *p)
{
    return p->curr < p->tokens.size;
}

struct token peek_token(struct parser *p)
{
    return p->tokens.items[p->curr];
}

struct token advance_token(struct parser *p)
{
    return p->tokens.items[p->curr++];
}

struct token consume_token(struct parser *p, enum token_kind kind, char *err)
{
    struct token t = peek_token(p);
    if (t.kind != kind) {
        report_parser_error(&t, err);
        t.kind = T_ERR;
        return t;
    }
    return advance_token(p);
}

void sync_parser(struct parser *p)
{
    while (has_tokens(p) && advance_token(p).kind != T_NEWLINE);
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
    ARRAY_MAKE(&tokens, struct token);

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
                    if (memcmp(s.start + 1, "db", 2) == 0) {
                        kind = T_LDB;
                    } else if (memcmp(s.start + 1, "dw", 2) == 0) {
                        kind = T_LDW;
                    } else if (memcmp(s.start + 1, "ea", 2) == 0) {
                        kind = T_LEA;
                    }
                    break;
                case 'n':
                    if (memcmp(s.start + 1, "op", 2) == 0) {
                        kind = T_NOP;
                    } else if (memcmp(s.start + 1, "ot", 2) == 0) {
                        kind = T_NOT;
                    }
                    break;
                case 'r':
                    if (memcmp(s.start + 1, "et", 2) == 0) {
                        kind = T_RET;
                    } else if (memcmp(s.start + 1, "ti", 2) == 0) {
                        kind = T_RTI;
                    }
                    break;
                case 's':
                    if (memcmp(s.start + 1, "tb", 2) == 0) {
                        kind = T_STB;
                    } else if (memcmp(s.start + 1, "tw", 2) == 0) {
                        kind = T_STW;
                    }
                    break;
                case 'x':
                    if (memcmp(s.start + 1, "or", 2) == 0) kind = T_XOR;
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
                case 'l':
                    if (memcmp(s.start + 1, "shf", 3) == 0) kind = T_LSHF;
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
                case 'r':
                    if (memcmp(s.start + 1, "shfl", 4) == 0) {
                        kind = T_RSHFL;
                    } else if (memcmp(s.start + 1, "shfa", 4) == 0) {
                        kind = T_RSHFA;
                    }
                    break;
                case 'b':
                    if (memcmp(s.start + 1, "rnzp", 4) == 0) kind = T_BRNZP;
                    break;
                }
                break;
            }

            if (kind == T_IDENT) {
                if (tokens.size == 0 ||
                        tokens.items[tokens.size - 1].kind == T_NEWLINE) {
                    kind = T_LABEL;
                }
            }

            tokens_put(&tokens, &s, kind);
            continue;
        }

        switch (c) {
        case ';':
            while (!next(&s, '\n')) advance(&s);
            break;

        case ',':
            tokens_put(&tokens, &s, T_COMMA);
            break;

        case '#':
            if (next(&s, '-')) advance(&s);
            while (isdigit(peek(&s))) advance(&s);

            tokens_put(&tokens, &s, T_DECIMAL);
            break;

        case 'x':
            if (next(&s, '-')) advance(&s);
            while (isxdigit(peek(&s))) advance(&s);

            tokens_put(&tokens, &s, T_HEX);
            break;

        case '.':
            while (isalnum(peek(&s))) advance(&s);

            enum token_kind kind = T_ERR;

            switch (s.curr - s.start) {
            case 4:
                if (memcmp(s.start + 1, "end", 3) == 0) {
                    kind = T_END;
                    stop_scanning = 1;
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
            tokens_put(&tokens, &s, kind);
            break;

        default: printf("Unknown char: '%c'\n", c);
        }
    }

    // @TODO(art): -2 to ignore .orig directive, we assume it always present.
    // Make it so it does not matter if directive was used, as default address
    // for user programs is 0x3000.
    int addr_offset = -2;

    // @LEAK(art): let OS free it for now
    struct labels_array labels;
    ARRAY_MAKE(&labels, struct label);

    for (size_t i = 0; i < tokens.size; ++i) {
        struct token t = tokens.items[i];

        switch (t.kind) {
        case T_NEWLINE:
            addr_offset += 2;
            break;

        case T_LABEL:;
            struct label l = {
                .name = t.lexem,
                .len = t.len,
                .addr = (size_t) addr_offset
            };
            ARRAY_PUT(&labels, struct label, l);
            break;
        }
    }

    struct parser p = {
        .tokens = tokens,
        .curr = 0
    };

    for (;;) {
        if (!has_tokens(&p)) break;

        struct token label = {0};
        struct token opcode = {0};

        if (peek_token(&p).kind == T_LABEL) {
            label = advance_token(&p);
        }

        opcode = advance_token(&p);
        if (!is_opcode(opcode.kind)) {
            report_parser_error(&opcode, "expected opcode");
            sync_parser(&p);
            continue;
        }
    }

    //{
    //    if (t.kind < T_kwd_begin || t.kind > T_kwd_end) {
    //        fprintf(stderr, "[line %lu] expected opcode, but got %.*s\n",
    //                t.line, t.len, t.start);
    //        exit(1);
    //    }

    //    switch (t.kind) {
    //    case T_ORIG: {
    //        struct token addr = tokens[i++];
    //        fprintf(stdout, "0x%.*s\n", addr.len, addr.start);
    //    } break;

    //    case T_LEA: {
    //        struct token dest = tokens[i++];
    //        if (dest.kind < T_reg_begin || dest.kind > T_reg_end) {
    //            fprintf(stderr, "[line %lu] expected register, but got %.*s\n",
    //                    dest.line, dest.len, dest.start);
    //            exit(1);
    //        }

    //        struct token comma = tokens[i++];
    //        if (comma.kind != T_COMMA) {
    //            fprintf(stderr, "[line %lu] expected comma, but got %.*s\n",
    //                    comma.line, comma.len, comma.start);
    //            exit(1);
    //        }

    //        struct token label = tokens[i++];
    //        if (label.kind != T_LABEL) {
    //            fprintf(stderr, "[line %lu] expected label, but got %.*s\n",
    //                    label.line, label.len, label.start);
    //            exit(1);
    //        }

    //        struct symtable_item s = {0};
    //        for (size_t j = 0; j < items_len; ++j) {
    //            if (label.len == items[j].len &&
    //                    memcmp(items[j].sym, label.start, label.len) == 0) {
    //                s = items[j];
    //            }
    //        }

    //        fprintf(stdout, "\n");
    //    } break;

    //    default:
    //        fprintf(stderr, "[line %lu] unknown token %.*s\n",
    //                t.line, t.len, t.start);
    //        exit(1);
    //    }

    //    if (tokens[i].kind != T_NEWLINE) {
    //        fprintf(stderr, "[line %lu] expected new line, but got %.*s\n",
    //                t.line, t.len, t.start);
    //        exit(1);
    //    }
    //}

    return 0;
}
