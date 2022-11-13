#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    size_t fsize;
    char *src;
    FILE *f ;

    if ((f = fopen("./ex.asm", "r")) == NULL) {
        perror("fopen");
        return 1;
    }

    if (fseek(f, 0, SEEK_END) < 0) {
        perror("fseek");
        return 1;
    }

    if ((int) (fsize = ftell(f)) < 0) {
        perror("ftell");
        return 1;
    }

    rewind(f);

    if ((src = calloc(fsize + 1, sizeof(char))) == NULL){
        perror("calloc");
        return 1;
    }

    if (fread(src, sizeof(char), fsize, f) < fsize) {
        perror("fread");
        return 1;
    }

    if (fclose(f) < 0) {
        perror("fclose");
        return 1;
    }

    return 0;
}
