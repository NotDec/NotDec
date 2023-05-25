//   Write    "constant"   string   into   "constant"   file
#include <stdio.h>
int main() {
    FILE* f = fopen("file.txt", "a");
    fprintf(f, "Append   constant   text.");
    fclose(f);
    //   Somewhere   else   in   the   binary:
    char buf[32];
    scanf("%[^\n]", buf);  // Stack-based buffer overflow
}