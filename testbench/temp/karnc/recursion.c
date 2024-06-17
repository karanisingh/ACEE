int factorial(int n)
{
    if(n == 1)
        return 1;
    else    
        return n*factorial(n-1);
}

int main()
{
    int sym_n = 5;
    return factorial(sym_n);
}
