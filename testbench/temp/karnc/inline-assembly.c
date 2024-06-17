#include <stdio.h>

unsigned int inline_assembly(unsigned int input) 
{
    unsigned int result;
    __asm__(
        "mov %1, %%eax\n\t" 
        "add %%eax, %%eax\n\t"
        "mov %%eax, %0\n\t" 
        : "=r" (result) 
        : "r" (input) 
        : "%eax" 
    );

    if(result > 4096)
        return 1;
    else
        return 0;
}

int main() 
{
    unsigned int sym_s; 
    klee_make_symbolic(&sym_s, sizeof(sym_s), "sym_s");
    klee_assume(sym_s > 0);
    klee_assume(sym_s < 4096);

    return inline_assembly(sym_s);
}
