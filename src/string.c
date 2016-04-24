#include "include/string.h"
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
