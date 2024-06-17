int non_linear_arithmetic(int x)
{
  if(x == 0)
    x = 2;

  int y = (x * x) + (x << 3) - 7;

  if (y ^ (y / x) == 1024)
    return 1;

  return 0;
}

int main()
{
  int sym_x = 0;
  return non_linear_arithmetic(sym_x);
}