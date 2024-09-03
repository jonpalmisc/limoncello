#include <stdio.h>
#include <stdlib.h>

extern void classifyNumber(int number) {
  if (number % 2 == 0) {
    puts("Number is even.");

    if (number % 12 == 0)
      puts("Number is a multiple of twelve.");
    if (number > 100)
      puts("Number is a greater than one hundred.");
    if (number > 1000)
      puts("Number is a greater than one thousand.");
  } else {
    if (number % 7 == 0)
      puts("Number is an odd multiple of seven.");
    if (number < 10)
      puts("Number is a less than ten.");
  }
}

extern void classifyNumber2(int number) {
  int i = 0;
  for (i = 0; i < 10; ++i) {
    if (number % i == 0) {
      printf("Number is a multiple of %d.\n", i);
    }

    if (i == 2) {
      printf("Number is even.");
    }

    if (i + number == 18)
      break;

    if (i == 5) {
      printf("Boom!");
    }
  }

  if (i != 10)
    printf("Bang!");
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <number>\n", argv[0]);
    return 1;
  }

  int number = atoi(argv[1]);
  classifyNumber(number);
  classifyNumber2(number);

  return 0;
}
