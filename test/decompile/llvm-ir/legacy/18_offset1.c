
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

// ../../scripts/clang-wasi-simple.sh ./offset1.c -o offset1.wasm
// wasm2wat --enable-all ./offset1.c -o offset1.wasm
// offset生成的测试用例。

void do_printf(const int* a) {
  printf("%p\n", &a);
}

struct  __attribute__((packed)) s1 {char c1; char c2; int32_t a[2]; char c3; char c4;};

int32_t* get_s(struct s1* s, int32_t i) {
  return &s->a[i];
}

void test1() {
  int32_t a; struct s1 b; int32_t c;
  do_printf(&a);
  do_printf(&c);
  for(int i=0;i<2;i++) {
    do_printf(&b.a[i]);
  }
}

void test2() {
  int32_t a; struct s1 b; int32_t c;
  do_printf(&a);
  do_printf(&c);
  for(int i=0;i<2;i++) {
    do_printf(get_s(&b, i));
  }
}

int main() {
  test1();
  test2();
}
