#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

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

enum token_kind {
    T_COMMA,
    T_NEWLINE,

    T_LABEL,

    T_BR,
    T_IN,
    T_R0,
    T_R1,
    T_R2,
    T_R3,
    T_R4,
    T_R5,
    T_R6,
    T_R7,

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

    T_DECIMAL,
    T_HEX
};

struct token {
    enum token_kind kind;
    char *start;
    size_t len;
    size_t line;
    int addr_offset;
};

struct scanner {
    char *start;
    char *curr;
    size_t line;
    int add_newline;
    int addr_offset;
};

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
    t->start = s->start;
    t->len = s->curr - s->start;
    t->line = s->line;
    t->addr_offset = s->addr_offset;

    s->add_newline = 1;
}

void make_newline_token(struct token *t, struct scanner *s)
{
    t->kind = T_NEWLINE;
    t->start = "\\n";
    t->len = 2;
    t->line = s->line;
    t->addr_offset = s->addr_offset;
}

int main(void)
{
    // @LEAK(art): let OS free it for now
    char *src = read_file("./ex.asm");

    for (char *p = src; *p; p++) *p = tolower(*p);

    struct scanner s = {
        .start = src,
        .curr = src,
        .line = 1,
        .add_newline = 0,
        // @NOTE(art): ignore .orig directive
        .addr_offset = -2
    };

    // @TODO(art): make growable array
    struct token tokens[50] = {0};
    size_t tokens_len = 0;
    int stop_scanning = 0;

    for (;;) {
        s.start = s.curr;

        if (!has_chars(&s) || stop_scanning) break;

        char c = advance(&s);

        // @NOTE(art): kwd, regs, labels
        if (c != 'x' && isalpha(c)) {
            while (isalnum(peek(&s))) advance(&s);

            enum token_kind kind = T_LABEL;

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

            make_token(&tokens[tokens_len++], kind, &s);
            continue;
        }

        switch (c) {
        case ' ':
        case '\t':
        case '\r':
            break;

        case '\n':
            if (s.add_newline) {
                make_newline_token(&tokens[tokens_len++], &s);
                s.add_newline = 0;
                s.addr_offset += 2;
            }
            s.line += 1;
            break;

        case ';':
            while (!next(&s, '\n') && has_chars(&s)) advance(&s);
            break;

        case ',':
            make_token(&tokens[tokens_len++], T_COMMA, &s);
            break;

        case '#':
            s.start++;

            if (next(&s, '-')) advance(&s);
            while (isdigit(peek(&s))) advance(&s);

            // @TODO(art): handle error
            assert(s.curr - s.start > 0);
            make_token(&tokens[tokens_len++], T_DECIMAL, &s);
            break;

        case 'x':
            s.start++;

            if (next(&s, '-')) advance(&s);
            while (isxdigit(peek(&s))) advance(&s);

            // @TODO(art): handle error
            assert(s.curr - s.start > 0);
            make_token(&tokens[tokens_len++], T_HEX, &s);
            break;

        case '.':
            while (isalnum(peek(&s))) advance(&s);

            enum token_kind kind = -1;

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
            assert((int) kind != -1);
            make_token(&tokens[tokens_len++], kind, &s);
            break;

        default: printf("Unknown char: '%c'\n", c);
        }
    }

    for (size_t i = 0; i < tokens_len; ++i) {
        printf("line: %lu ", tokens[i].line);
        printf("kind: %i ", tokens[i].kind);
        printf("len: %lu ", tokens[i].len);
        printf("lex: %.*s ", tokens[i].len, tokens[i].start);
        printf("offset: %i\n", tokens[i].addr_offset);
    }

    return 0;
}
