int exponentiation(int x, int y)
{
    int result = 1;
    if(x < 0 || y < 0) 
        return result;
    
    for(int i = 0;i < y; ++i)
        result *= x;

    return result;
}

int main()
{
    int sym_x, sym_y;
    klee_make_symbolic(&sym_x, sizeof(sym_x), "sym_x");
    klee_make_symbolic(&sym_y, sizeof(sym_y), "sym_y");
    return exponentiation(sym_x, sym_y);
}

