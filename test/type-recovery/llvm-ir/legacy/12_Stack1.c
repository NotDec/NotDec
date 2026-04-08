struct struct_245 {
    unsigned int field_246 __attribute__((annotate("off:-8")));
    float field_247 __attribute__((annotate("off:-4")));
};
static unsigned int main(unsigned int _arg_0, unsigned int _arg_1);
static unsigned int main(unsigned int _arg_0, unsigned int _arg_1) {
    struct struct_245 stack;
    return (unsigned int)stack.field_246 == _arg_0 || (unsigned int)stack.field_247 == _arg_1;
}
