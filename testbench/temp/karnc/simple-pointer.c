int simple_pointer(int *a, int *b)
{
  if(*a < *b)
    return *a;
  if(*b < *a)
    return *b;
  return *a;
}


int main()
{
  int sym_a;
  int sym_b;
  return simple_pointer(&sym_a, &sym_b);
}
