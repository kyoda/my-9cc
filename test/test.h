#define ASSERT(x, y) assert(x, y, #y)

void assert(int expected, int actual, char *code);
void printf();
int memcmp(char *p, char *q, int n);
int strcmp(char *p, char *q);