#pragma once
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>

#define TAB '\t'
int perr(int err); 
#define zassert(eq) if(eq){ _exit(perr(errno)); }

void put_header(char *filename);
size_t strastr(const char *haystack, const char *needle);
size_t scrlen(const char *from, size_t to, int tab_l);

