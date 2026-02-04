struct struct_120 {
    struct struct_120 *field_121 __attribute__((annotate("off:0")));
};
struct struct_123 {
    struct struct_123 *field_124 __attribute__((annotate("off:0")));
};
static void rec1(unsigned int _arg_0);
static unsigned int main(unsigned int _arg_0, unsigned int _arg_1);
static void rec1(unsigned int _arg_0) {
    rec1(*(unsigned int *)_arg_0);
    return;
}
static unsigned int main(unsigned int _arg_0, unsigned int _arg_1) {
    unsigned char stack_65[8];
    rec1((unsigned int)&stack_65);
    return 0U;
}
