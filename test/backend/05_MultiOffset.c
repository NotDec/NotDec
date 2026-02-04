typedef unsigned int undef32;
struct struct_71 {
    undef32 *field_72 __attribute__((annotate("off:4")));
};
struct struct_73 {
    undef32 field_74 __attribute__((annotate("off:4")));
};
struct struct_75 {
    undef32 field_76 __attribute__((annotate("off:0")));
    undef32 *field_77 __attribute__((annotate("off:4")));
};
struct struct_79 {
    undef32 field_80 __attribute__((annotate("off:4")));
};
static undef32 main(unsigned int _arg_0, unsigned int _arg_1);
unsigned char __notdec_mem0[16777216] = {};
static undef32 main(unsigned int _arg_0, unsigned int _arg_1) {
    struct struct_75 stack1;
    struct struct_71 *stack_1p = (struct struct_71 *)&stack1;
    *(undef32 *)&stack1 = 0U;
    *(undef32 *)&stack_1p->field_72 = 1U;
    return stack_1p;
}
