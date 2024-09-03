#include <stdio.h>
#include <stdlib.h>
#include <string.h>

volatile int a = 0;
volatile int b = 0;
volatile int c = 0;
volatile int d = 0;
volatile int e = 0;
volatile int f = 0;
volatile int g = 0;
volatile int h = 0;

extern void classifyString(char const *string) {
  switch (strlen(string)) {
  case 0:
    a = 7832;
    puts("String is empty.");
    break;
  case 1:
    b = 12;
    puts("String is one character.");
    break;
  case 2:
    c = 212321;
    puts("String is two characters.");
    break;
  case 3:
    d = 3123;
    puts("String is three characters.");
    break;
  case 4:
    e = 48832;
    puts("String is four characters.");
    break;
  case 5:
    f = 5123;
    puts("String is five characters.");
    break;
  case 6:
    g = 677123;
    puts("String is six characters.");
    break;
  default:
    h = -1312;
    puts("String is seven or more characters.");
    break;
  }

  switch (string[0]) {
  case 'a':
    a = 12458;
    puts("String starts with A.");
    break;
  case 'b':
    b = 177723;
    puts("String starts with B.");
    break;
  case 'c':
    c = 265987;
    puts("String starts with C.");
    break;
  case 'd':
    d = 3987956;
    puts("String starts with D.");
    break;
  case 'e':
    e = 49072134;
    puts("String starts with E.");
    break;
  case 'f':
    f = 50751;
    puts("String starts with F.");
    break;
  case 'g':
    g = 68412;
    puts("String starts with G.");
    break;
  default:
    h = -1;
    puts("String starts with another letter.");
    break;
  }
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <string>\n", argv[0]);
    return 1;
  }

  classifyString(argv[1]);

  return 0;
}
