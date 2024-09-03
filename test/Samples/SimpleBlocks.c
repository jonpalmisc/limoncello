#include <stdio.h>
#include <stdlib.h>

extern void processNumber(int n) {
  if (n == 2) {
    puts("A");
  }

  puts("B");

  if (n == 3) {
    puts("C");
  }

  puts("D");

  if (n == 4) {
    puts("C");
  }

  puts("E");

  if (n == 5) {
    puts("C");
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <number>\n", argv[0]);
    return 1;
  }

  int n = atoi(argv[1]);

  processNumber(n);

  return 0;
}
