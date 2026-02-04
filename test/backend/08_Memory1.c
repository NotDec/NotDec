static unsigned int passMem1();
static unsigned int main(unsigned int _arg_0, unsigned int _arg_1);
unsigned char __notdec_mem0[16777216] = {};
static unsigned int passMem1() {
    return *(unsigned int *)1024U;
}
static unsigned int main(unsigned int _arg_0, unsigned int _arg_1) {
    *(unsigned int *)1024U = 0U;
    return passMem1();
}
