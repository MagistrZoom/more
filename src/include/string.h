#pragma once
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>

#define TAB '\t'
#define NL "\n"
#define NLC '\n'
int perr(int err); 

void put_header(char *filename);
size_t strastr(const char *haystack, const char *needle);
size_t scrlen(const char *from, size_t limit, size_t columns, int tab_l);
void *memmem(const void *l, size_t l_len, const void *s, size_t s_len);
