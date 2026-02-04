struct struct_264 {
    unsigned int field_265[0] __attribute__((annotate("off:-48")));
    unsigned int field_266 __attribute__((annotate("off:-8")));
    unsigned int field_267 __attribute__((annotate("off:-4")));
};
typedef unsigned int undef32;
extern void printLine(unsigned int arg_0);
extern void printIntLine(unsigned int arg_0);
static void goodG2B();
extern void llvm_memset_p0i8_i64(unsigned char *arg_0, unsigned char arg_1, unsigned long long arg_2, int arg_3);
static unsigned int __stack_pointer = 66720U;
static void goodG2B() {
    struct struct_264 stack26;
    unsigned int storemerge;
    llvm_memset_p0i8_i64((unsigned char *)(undef32 *)&stack26.field_265, 0Ui8, 40ULL, (int)false);
    unsigned int loadResult = stack26.field_267;
    if ((int)loadResult > (unsigned int)-1) {
        stack26.field_265[loadResult << 2U] = 1U;
        storemerge = 0U;
        while (1)
            {
                unsigned int storemerge_reload = storemerge;
                stack26.field_266 = storemerge_reload;
                if ((int)storemerge_reload <= 9U)
                    break;
                printIntLine(stack26.field_265[storemerge_reload << 2U]);
                storemerge = stack26.field_266 + 1U;
            }
    } else
        printLine(1060U);
    return;
}
