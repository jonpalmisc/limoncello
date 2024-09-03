#include <stdio.h>

void caesar(char *data) {
  while (*data != 0) {
    if (*data >= 'A' && *data <= 'X')
      *data += 2;
    if (*data >= 'a' && *data <= 'x')
      *data += 1;

    ++data;
  }
}

char greeting[] = "Hello, world!";

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  puts(greeting);
  caesar(greeting);
  puts(greeting);

  return 0;
}
