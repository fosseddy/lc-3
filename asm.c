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

    T_ADD,

    T_R0,
    T_R1,
    T_R2,
    T_R3,
    T_R4,
    T_R5,
    T_R6,
    T_R7,

    T_DECIMAL
};

struct token {
    enum token_kind kind;
    char *start;
    size_t len;
    size_t line;
};

struct scanner {
    char *start;
    char *curr;
    size_t line;
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
}

void make_newline_token(struct token *t, struct scanner *s)
{
    t->kind = T_NEWLINE;
    t->start = "\\n";
    t->len = 2;
    t->line = s->line;
}

int check_kwd(struct scanner *s, size_t start, size_t len, char *rest)
{
    return (size_t) (s->curr - s->start) == start + len &&
           memcmp(s->start + start, rest, len) == 0;
}

int main(void)
{
    // @LEAK(art): let OS free it for now
    char *src = read_file("./ex.asm");

    for (char *p = src; *p; p++) *p = tolower(*p);

    struct scanner s = {
        .start = src,
        .curr = src,
        .line = 1
    };

    // @TODO(art): make growable array
    struct token tokens[50] = {0};
    size_t tokens_len = 0;

    for (;;) {
        s.start = s.curr;

        if (!has_chars(&s)) break;

        char c = advance(&s);

        // @NOTE(art): kwd, regs, labels
        if (isalpha(c)) {
            while (isalnum(peek(&s))) advance(&s);
            switch (*s.start) {
            case 'a':
                if (check_kwd(&s, 1, 2, "dd")) {
                    make_token(&tokens[tokens_len++], T_ADD, &s);
                    continue;
                }
                break;

            case 'r':
                if (s.curr - s.start == 2) {
                    switch (*(s.start + 1)) {
                    case '0':
                        make_token(&tokens[tokens_len++], T_R0, &s);
                        continue;
                    case '1':
                        make_token(&tokens[tokens_len++], T_R1, &s);
                        continue;
                    case '2':
                        make_token(&tokens[tokens_len++], T_R2, &s);
                        continue;
                    case '3':
                        make_token(&tokens[tokens_len++], T_R3, &s);
                        continue;
                    case '4':
                        make_token(&tokens[tokens_len++], T_R4, &s);
                        continue;
                    case '5':
                        make_token(&tokens[tokens_len++], T_R5, &s);
                        continue;
                    case '6':
                        make_token(&tokens[tokens_len++], T_R6, &s);
                        continue;
                    case '7':
                        make_token(&tokens[tokens_len++], T_R7, &s);
                        continue;
                    }
                }
                break;
            }

            make_token(&tokens[tokens_len++], T_LABEL, &s);
            continue;
        }

        switch (c) {
        case ' ':
        case '\t':
        case '\r':
            break;

        case '\n':
            make_newline_token(&tokens[tokens_len++], &s);
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

        default: printf("Unknown char: '%c'\n", c);
        }
    }

    for (size_t i = 0; i < tokens_len; ++i) {
        printf("line: %lu ", tokens[i].line);
        printf("kind: %i ", tokens[i].kind);
        printf("len: %lu ", tokens[i].len);
        printf("lex: %.*s\n", tokens[i].len, tokens[i].start);
    }

    return 0;
}
