struct Pair {
  int first;
  int *second;
};

int use_pair(void) {
  struct Pair *pair = (struct Pair *)1024;
  pair->first = 1;
  pair->second = (int *)2048;
  return pair->first;
}
