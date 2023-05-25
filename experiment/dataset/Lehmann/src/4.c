#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<wchar.h>

void exec(const char * cmd) {
}

void print_hex(int i) {
    printf("%x\n", i);
}

void print_dec(int i) {
    printf("%d\n", i);
}

int main() {

    // forces exec to be in the function table
    printf("&exec: %p\n", &exec);

    void (*fptr)(int);

    char * op = malloc(16);
    char * arg = malloc(16);

    FILE * f = fopen("exploit", "rb");
    fgets (op, 40, f);

    if (op[0] == 'x') {
        fptr = &print_hex;
    } else if (op[0] == 'd') {
        fptr = &print_dec;
    } else {
        exit(1);
    }

    for (size_t* i = (size_t*) (op-40); i < (size_t*) (op+100); i++)
        printf("%p %zx\n", i, *i);

    // force fptr to be in linear memory because we take its address
    printf("fptr: %p @ %p \n", fptr, &fptr);

    free(op);

    for (size_t* i = (size_t*) (op-40); i < (size_t*) (op+100); i++)
        printf("%p %zx\n", i, *i);

    // force fptr to be in linear memory because we take its address
    printf("fptr: %p @ %p \n", fptr, &fptr);

    gets(arg);

    fptr(atoi(arg));
}
