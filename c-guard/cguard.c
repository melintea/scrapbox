/*
 * @see Overload 192, Alison Chaiken
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline void int_cleanup(int* px)
{
    printf("int_cleanup: %d\n", *px);
}

static inline void mem_cleanup(void* pp)
{
    void* p = *((void**)pp);
    printf("mem_cleanup: %p\n", p);
    free(p);
}

static inline void fopen_cleanup(void* pp)
{
    FILE* p = *((FILE**)pp);
    printf("fopen_cleanup: %p\n", p);
    if (p) { fclose(p); };
}

int main()
{
    {
        int x __attribute__((cleanup(int_cleanup))) = 42;
    }
    {
        char* p __attribute__((cleanup(mem_cleanup))) = strdup("some str");
    }
    {
        void* p __attribute__((cleanup(mem_cleanup))) = malloc(1023);
    }
    {
        FILE* pf __attribute__((cleanup(fopen_cleanup))) = fopen("/no/such/file", "r");
    }
    
    printf("main: done\n");
    return 0;
}
