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
    char stack1[8];
    *(int *)&stack1 = 0;
    *(int *)&stack1[4] = 1;
    return (int)&stack1;
}
// ====== End of Function Definitions ======
