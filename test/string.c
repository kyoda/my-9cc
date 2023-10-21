#include "test.h"

int main() {
  ASSERT(97, "abc"[0]);
  ASSERT(98, ({ char *str = "abc"; str[1]; }));
  ASSERT(99, ({ char *str = "abc"; str[2]; }));
  ASSERT(4, ({ sizeof("abc"); }));
  ASSERT(7, ({ "\a"[0]; }));
  ASSERT(8, ({ "\b"[0]; }));
  ASSERT(9, ({ "\t"[0]; }));
  ASSERT(10, ({ "\n"[0]; }));
  ASSERT(11, ({ "\v"[0]; }));
  ASSERT(12, ({ "\f"[0]; }));
  ASSERT(13, ({ "\r"[0]; }));
  ASSERT(34, ({ "\""[0]; }));

  // octal escape
  ASSERT(0, ({ "\0"[0]; }));
  ASSERT(7, ({ "\7a"[0]; }));
  ASSERT(7, ({ "\007"[0]; }));
  ASSERT(24, ({ "\030"[0]; }));
  ASSERT(192, ({ "\300"[0]; })); //gcc -> -64
  ASSERT(192, ({ "\3000"[0]; })); //gcc -> -64

  // hex escape
  ASSERT(0, ({ "\x0"[0]; }));
  ASSERT(10, ({ "\x000a"[0]; }));
  ASSERT(170, ({ "\x000aA"[0]; })); //gcc -> -86
  ASSERT(255, ({ "\x000ff"[0]; })); //gcc -> -1
  ASSERT(252, ({ "\x000ffc"[0]; })); //gcc -> -4

  //comment
  ASSERT(1, ({ //return 0;
 1; }));
  ASSERT(2, ({ /*return 1;*/ 2; }));

  printf("OK\n");
  return 0;
}