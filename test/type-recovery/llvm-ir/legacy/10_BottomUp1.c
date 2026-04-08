#include <stdbool.h>

// ====== Type Declarations ======
// ====== End of Type Declarations ======

// ====== Function Declarations ======
extern void isFloat(float v_412);
static void ptrFloat(float *_arg_0);
static int main(int _arg_0, char **_arg_1);
// ====== End of Function Declarations ======

// ====== Type Definitions ======
// ====== End of Type Definitions ======

// ====== Global Definitions ======
// ====== End of Global Definitions ======

// ====== Function Definitions ======
static void ptrFloat(float *_arg_0) {
    isFloat(*_arg_0);
    return;
}
static int main(int _arg_0, char **_arg_1) {
    float stackPtr;
    ptrFloat(&stackPtr);
    return 0;
}
// ====== End of Function Definitions ======

