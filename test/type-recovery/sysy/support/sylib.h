#ifndef __SYLIB_H_
#define __SYLIB_H_

// https://www.reddit.com/r/C_Programming/comments/112w2ov/why_is_it_not_an_error_to_declare_a_function/
/* Input & output functions */
int getint(void), getch(void), getarray(int a[]);
float getfloat(void);
int getfarray(float a[]);

void putint(int a), putch(int a), putarray(int n, int a[]);
void putfloat(float a);
void putfarray(int n, float a[]);

// void putf(char a[], ...);
void *memset(void *b, int c, unsigned long len);
void *memcpy(void *to, const void *from, unsigned long numBytes);

/* Timing function implementation */
// struct timeval _sysy_start,_sysy_end;
// #define starttime() _sysy_starttime(__LINE__)
// #define stoptime()  _sysy_stoptime(__LINE__)
// #define _SYSY_N 1024
// int _sysy_l1[_SYSY_N],_sysy_l2[_SYSY_N];
// int _sysy_h[_SYSY_N], _sysy_m[_SYSY_N],_sysy_s[_SYSY_N],_sysy_us[_SYSY_N];
// int _sysy_idx;
// __attribute((constructor)) void before_main();
// __attribute((destructor)) void after_main();
// void _sysy_starttime(int lineno);
// void _sysy_stoptime(int lineno);

#endif
