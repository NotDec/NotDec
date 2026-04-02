#include <stdbool.h>

// ====== Type Declarations ======
// ====== End of Type Declarations ======

// ====== Function Declarations ======
static int main(int _arg_0, char **_arg_1);
// ====== End of Function Declarations ======

// ====== Type Definitions ======
// ====== End of Type Definitions ======

// ====== Global Declarations ======
// ====== End of Global Declarations ======

// ====== Global Definitions ======
// ====== End of Global Definitions ======

// ====== Function Definitions ======
static int main(int _arg_0, char **_arg_1) {
    char stack[8];
    return *(int *)&stack[-8] == _arg_0 || (char **)*(int *)&stack[-4] == _arg_1;
}
// ====== End of Function Definitions ======
