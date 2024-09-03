#include <stdio.h>
#include <stdlib.h>

volatile int n = 0;

extern void doArithmetic(int m) {
  n = m;

  int o = n;
  o ^= 2;
  o += 17;
  n = o;

  int p = n;
  p -= 2;
  p &= 0x5555;
  n = p;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <number>\n", argv[0]);
    return 1;
  }

  int m = atoi(argv[1]);
  doArithmetic(m);

  printf("%d\n", n);

  return 0;
}
