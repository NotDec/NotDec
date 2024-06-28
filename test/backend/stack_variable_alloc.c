
#include<stdio.h>

// ../../scripts/clang-wasi-simple.sh ./stack_variable_alloc.c -o stack_variable_alloc.wasm
// wasm2wat --enable-all ./stack_variable_alloc.wasm -o ./stack_variable_alloc.wat
// 这个测试用例展示了，单是扫描函数内的常量内存访问不够，如果仅仅是取地址不访问也是有可能的。
// 更进一步，如果把结构体地址取走，在其他函数访问内部信息，也是有可能的。
// 栈空间关于直接分配的变量的分割可以直接实现。但即使是栈上变量，想要继续深入分析类型也需要指针分析。

void do_printf(const int* a, const int* b, const int* c) {
  printf("%p\n%p\n%p\n", &a, &b, &c);
}

void test1() {
  int a; int b; int c;
  do_printf(&a, &b, &c);
}

struct tmp {
    int a;
    int b;
    int c;
  };

void do_printf2(const struct tmp* a) {
  printf("%p\n%p\n%p\n", &a->a, &a->b, &a->c);
}

void test2() {
  struct tmp a;
  do_printf2(&a);
}

int main() {
  test1();
  test2();
}
