struct myStruct{
  int num;
  char letter;
  int flag;
};

struct myStruct struct_manipulation(struct myStruct s)
{
  int score = 0;

  if(s.letter >= 'A' && s.letter <= 'Z')
    score += (s.letter-'C');

  s.num = score;
  return s;
}


int main()
{
  struct myStruct sym_s;
  
  struct myStruct ret = struct_manipulation(sym_s);
  if(ret.num == 1)
    ret.flag = 1;
  else
    ret.flag = -1;

  return 1;
}
