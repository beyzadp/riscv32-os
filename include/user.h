#pragma once
#include "common.h"

__attribute__((noreturn)) void exit(void);
int getchar(void);
void putchar(char ch);
void listfiles(void);
int readfile(const char *filename, char *buf, int max_len);