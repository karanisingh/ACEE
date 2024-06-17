int integer_array(int arr[])
{
  int sum = 0;
  for(int i =0;i < 10; ++i)
    sum += arr[i];

  return sum;
}

int main()
{
  int arr[10];
  klee_make_symbolic(arr, sizeof(arr), "arr");
  int result = integer_array(arr);
  if(result %2 == 0)
    return 1;
  else
    return 0;
}



