#pragma once
#include "common.h"

__attribute__((noreturn)) void exit(void);
int getchar(void);
void putchar(char ch);
void listfiles(void); // Prints all filenames in MYFS to stdout
int readfile(const char *filename, char *buf, int max_len); // Returns bytes read, or -1 if not found
int writefile(const char *filename, const char *buf, int len); // Creates or overwrites filename