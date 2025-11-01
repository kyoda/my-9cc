#define ASSERT(x, y) assert(x, y, #y)


void assert(int expected, int actual, char *code);
//#include <stdio.h>
extern int printf(char *fmt, ...);
extern int sprintf(char *buf, char *fmt, ...);
//#include <string.h>
extern int memcmp(char *p, char *q, int n);
extern int strcmp(char *p, char *q);