int basic_switch(char c)
{
  int result = -1;
  switch(c){
    case 'k':
      result = 0;
      break;
    case 'a':
      result = 1;
      break;
    case 'r':
      result = 2;
      break;
    case 'n':
      result = 4;
      break;
    default:
      break;
  }

  return result;
}

int main()
{
  char sym_c;
  klee_make_symbolic(&sym_c, sizeof(sym_c), "sym_c");
  return basic_switch(sym_c);
}
