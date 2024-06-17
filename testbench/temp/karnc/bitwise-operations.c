int bitwise(int x, int y)
{
    int result = 0;

    if ((x ^ y) & 1) 
        result |= 1; 
    else 
        result &= ~1; 

    if ((x & y) & 2)
        result <<= 2; 
    else
        result >>= 2;

    if ((x | y) & 4) 
        result ^= 0xF; 

    return result;
}


int main()
{
    int sym_x, sym_y;
    return bitwise(sym_x, sym_y);
}