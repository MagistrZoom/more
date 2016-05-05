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
 * same as strstr but searches for all occasions
 */
size_t strastr(const char *haystack, const char *needle) {
    size_t r = 0;
    char *high_bracket = (char*)haystack;
    while((high_bracket = strstr(high_bracket, needle)) != NULL){
            r++;
            high_bracket += strlen(needle);
	}
	return r;
}

/*
 * calculates size of line from @from to @to with respect to tabstops
 */
struct screen_len scrlen(const char *from, size_t limit, size_t columns, int tab_l, int *flag){
	int		t_offset = 0;
	size_t 	screen_l = 0;
	size_t 	real_l = 0;
	char *to = (char*)from + limit;
	char *cur = (char*)from;

	char *newline = memmem(from, limit, NL, 1);
	if(newline != NULL && newline < to){
		if(newline < from + columns){
			/*there are \n in line*/
			newline++;
		}
		to = newline;
	}
	while(screen_l < columns && cur < to) {
		if(*cur == TAB){
			if(screen_l + tab_l - t_offset < columns)
				screen_l += tab_l - t_offset;
			else
				break;
			t_offset = 0;
		} else {
			screen_l++;
			t_offset = (t_offset + 1) & (tab_l - 1);
		}
		real_l++;
		cur++;
	}
	return (struct screen_len){
		.screen = screen_l,
		.real = real_l	
	};
}

/*
 * This function returns amount of bytes after ptr less or equal than columns
 * + newline optionally
 */
int get_outstr(char *ptr, int columns, size_t max, int *flag){
	char *newline = memmem(ptr, max, NL, 1);
	if(!newline){
		*flag = 1;
		return columns;
	} else {
		int min = 0;
		if(columns < newline - ptr){
			min = columns;
		} else {
			min = newline - ptr;
		}
		return min;
	}
}

/*
 * Find the first occurrence of the byte string s in byte string l.
 */

void *memmem(const void *l, size_t l_len, const void *s, size_t s_len) {
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

