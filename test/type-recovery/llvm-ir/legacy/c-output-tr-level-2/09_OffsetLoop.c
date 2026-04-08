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
    char stack1[16];
    int p1;
    int p2;
    p1 = (int)&stack1;
    do {
        p2 = p1 + 4;
        *(int *)p2 = 0;
        p1 = p2;
    } while (p2 < _arg_0);
    return 0;
}
// ====== End of Function Definitions ======
