#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

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

    T_ADD,

    T_R0,

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
    size_t idx = 0;
    struct token tokens[50] = {0};

    for (;;) {
        s.start = s.curr;

        if (!has_chars(&s)) break;

        char c = advance(&s);

        // @NOTE(art): kwd, regs, labels
        if (isalpha(c)) {
            while (isalnum(peek(&s))) advance(&s);
            switch (*s.start) {
            case 'a':
                if (s.curr - s.start == 3 &&
                        memcmp(s.start + 1, "dd", 2) == 0) {
                    printf("keyword: add\n");
                }
                break;

            default: printf("label: %.*s\n", s.curr - s.start, s.start);
            }

            continue;
        }

        switch (c) {
        case ' ':
        case '\t':
        case '\r':
            break;

        case '\n':
            tokens[idx++] = (struct token) {
                .kind = T_NEWLINE,
                .len = 2,
                .start = "\\n",
                .line = s.line
            };
            s.line += 1;
            break;

        case ';':
            while (!next(&s, '\n') && has_chars(&s)) advance(&s);
            break;

        default: printf("Unknown char: '%c'\n", c);
        }
    }

    for (size_t i = 0; i < idx; ++i) {
        printf("line: %lu ", tokens[i].line);
        printf("kind: %i ", tokens[i].kind);
        printf("len: %lu ", tokens[i].len);
        printf("lex: %.*s\n", tokens[i].len, tokens[i].start);
    }

    return 0;
}
