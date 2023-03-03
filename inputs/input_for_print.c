//=============================================================================
// FILE:
//      input_for_print.c
//
// DESCRIPTION:
//      Sample input file for InsertExistFunc
//
// License: MIT
//=============================================================================
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

void print() {
    printf("branch found!");
}

int foo(int a) {
  return a * 2;
}

int bar(int a, int b) {
  return (a + foo(b) * 2);
}

int fez(int a, int b, int c) {
  return (a + bar(a, b) * 2 + c * 3);
}

int main(int argc, char *argv[]) {
  int a; 
  scanf("%d", &a);
  int ret = 0;

  ret += foo(a);
  ret += bar(a, ret);
  ret += fez(a, ret, 123);

  if (ret > 0) {
    ret += 10;
  } else {
    ret -= 10;
  }

  printf("%d", ret);
  return ret;
}
