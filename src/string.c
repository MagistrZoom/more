#include "include/string.h"
#include "include/zprintf.h"

void put_header(char *filename){
	zprintf("[2J[H::::::::::::\n%s\n::::::::::::\n", filename);	
}

int perr(int err){
	char *err_ptr = strerror(err);
	dzprintf(STDERR_FILENO, "%s\n", err_ptr);
	return err;
}

/*
 * @function strastr
 * @description same as strstr but searches for all occasions
 * @param (const char*) haystack
 * @param (const char*) needle
 * @return (size_t) amount of occasions of needle in haystack
 */
size_t strastr(const char *haystack, const char *needle) {
    size_t r = 0;
    char *high_bracket = haystack;
    while((high_bracket = strstr(high_bracket, needle)) != NULL){
            r++;
            high_bracket += strlen(needle);
	}
	return r;
}

/*
 * @function scrlen
 * @description calculates size of line from @from to @to with respect to tabstops
 * @param char *from
 * @param char *to
 * @param int tabstop
 * @return size_t strlen wit respect to tabstops
 */
size_t scrlen(const char *from, size_t offset, int tab_l){
	int		t_offset = 0;
	size_t 	real_l = 0;
	char *to = from + offset;
	char *cur;
	for(cur = from; cur < to; cur++){
		if(*cur == TAB){
			real_l += tab_l - t_offset;
			t_offset = 0;
		} else {
			real_l++;
			t_offset = (t_offset+1) & 7;
		}
	}

	return real_l;
}


/*
 * Find the first occurrence of the byte string s in byte string l.
 */

void * memmem(const void *l, size_t l_len, const void *s, size_t s_len) {
	register char *cur, *last;
	const char *cl = (const char *)l;
	const char *cs = (const char *)s;

	/* we need something to compare */
	if (l_len == 0 || s_len == 0)
		return NULL;

	/* "s" must be smaller or equal to "l" */
	if (l_len < s_len)
		return NULL;

	/* special case where s_len == 1 */
	if (s_len == 1)
		return memchr(l, (int)*cs, l_len);

	/* the last position where its possible to find "s" in "l" */
	last = (char *)cl + l_len - s_len;

	for (cur = (char *)cl; cur <= last; cur++)
		if (cur[0] == cs[0] && memcmp(cur, cs, s_len) == 0)
			return cur;

	return NULL;
}

