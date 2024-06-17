int nested_loops(int i, int j, int k)
{
  int count = 0;
  for(int ii = 0; ii < i; ++ii)
  {
    for(int jj = 0; jj < j; ++jj)
    {
      for(int kk = 0; kk < k; ++kk)
      {
        if(ii*ii + jj*jj == kk*kk)
          count++;
      }
    }
  }

  return count;
}


int main() 
{
	int sym_i, sym_j, sym_k;
	klee_make_symbolic(&sym_i, sizeof(sym_i), "sym_i");
  klee_make_symbolic(&sym_j, sizeof(sym_j), "sym_j");
  klee_make_symbolic(&sym_k, sizeof(sym_k), "sym_k");
	return nested_loops(sym_i, sym_j, sym_k);
}
