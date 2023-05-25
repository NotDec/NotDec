int main() {
    char parent_frame[8] = "BBBBBBBB";  // Also overwritten
    vulnerable(readline());
    // Dangerous if parent_frame is passed, e.g., to exec
}
void vulnerable(char* input) {
    char same_frame[8] = "AAAAAAAA";  // Can be overwritten
    char buffer[8];
    strcpy(buffer, input);  // Buffer overflow on the stack
}
