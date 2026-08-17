/*
 * @see Overload 192, Alison Chaiken
 */

#include <stdio.h>

static void int_cleanup(int* px)
{
    printf("int_cleanup: %d\n", *px);
}

int main()
{
    {
        int x __attribute__((cleanup(int_cleanup))) = 42;
    }
    printf("main: done\n");
    return 0;
}
