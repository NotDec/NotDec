//   Functions   supposed   to   be   triggered   by   requests
#include <stdio.h>
void log_happy(int customer_id) { /*   ...    */
}
void log_unhappy(int customer_id) { /*   ...    */
}
void handle_request(char* input1, int input2, char* input3) {
    void (*func)(int) = NULL;
    char* happiness = malloc(16);
    char* other_allocation = malloc(16);
    memcpy(happiness, input1, input2);  //   Heap   overflow
    if (happiness[0] ==   'h')
        func = &log_happy;
    else if (happiness[0] ==   'u')
        func = &log_unhappy;
    free(happiness);     //   Unlink   exploit    overwrites   func
    func(atoi(input3));  //   3rd   input   is   passed   as   argument
}
//   Somewhere   else   in   the    binary:
void exec(const char* cmd) { /*   ...    */
}