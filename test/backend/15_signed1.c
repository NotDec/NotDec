


#include <stdio.h>
#include <stdint.h>
int32_t conv_signed(long i) {
  return (int32_t)i;
}

uint32_t conv_unsigned(unsigned long i) {
  return (uint32_t)i;
}

int32_t main() {
  int32_t x = conv_signed(-0x1000000001);
  // printf("%c%x\n", (x<0) ? '-' : ' ', x);
  printf("%d\n", x);
  return 0;
}
