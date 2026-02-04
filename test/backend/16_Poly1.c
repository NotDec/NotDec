extern void isFloat(float arg_0);
extern void isInt(unsigned int arg_0);
static void *ptrFloat(float *_arg_0);
static void *ptrInt(unsigned int *_arg_0);
static unsigned int pass(unsigned int _arg_0);
static unsigned int main(void *_arg_0, void *_arg_1);
static void *ptrFloat(float *_arg_0) {
    isFloat(*_arg_0);
    return pass((unsigned int)_arg_0);
}
static void *ptrInt(unsigned int *_arg_0) {
    isInt(*_arg_0 * 5U);
    return pass((unsigned int)_arg_0);
}
static unsigned int pass(unsigned int _arg_0) {
    return _arg_0;
}
static unsigned int main(void *_arg_0, void *_arg_1) {
    float stackPtr1;
    unsigned int stackPtr2;
    return (unsigned int)ptrFloat(&stackPtr1) == _arg_0 || ptrInt(&stackPtr2) == _arg_1;
}
