#include "include/zprintf.h" 
#include "include/string.h" 
#include "include/zassert.h"

void reverse(char s[]){
    int i, j;
    char c;
    for (i = 0, j = strlen(s)-1; i < j; i++, j--){
    	c = s[i];
    	s[i] = s[j];
   		s[j] = c;
    }
}

void itoa(int n, char s[]) {
	int i, sign;

	if ((sign = n) < 0)
		n = -n;       
	i = 0;
	do {       
		s[i++] = n % 10 + '0';  
	} while ((n /= 10) > 0);   
	if (sign < 0)
		s[i++] = '-';
	s[i] = '\0';
	reverse(s);
}
#define STR_BUF (1024)
#define FORMAT_BUF (64)
static void _zprintf(int fd, char *template, va_list argp){
	int i = -1, j = 0, r;
	size_t written;
	char outbuffer[STR_BUF] = { 0 };
	char formatbuffer[FORMAT_BUF] = { 0 };
	size_t template_length = strlen(template);
	char t;
	char *s;

A:
	i++;
	if(i >= template_length)
		goto Z;
	if(template[i] != '%')
		goto F;
	written = write(fd, outbuffer, j);
	zassert(written < 0)

	j = 0;
	i++;
	switch(template[i]){
		case 'c': goto C; break;
		case 's': goto E; break;
		case 'd': goto D; break;
		case '%': goto G; break;
	}
	goto G;
C:
	t = va_arg(argp, int);
_C:	written = write(fd, &t, 1);
	zassert(written < 0)
	goto A;
D:
	r = va_arg(argp, int);
	itoa(r, formatbuffer);
	written = write(fd, formatbuffer, strlen(formatbuffer));
	zassert(written < 0)
	goto A;
E:
	s = va_arg(argp, char*);
	written = write(fd, s, strlen(s));
	zassert(written < 0)
	goto A;
F:
	if(j >= STR_BUF){
		written = write(fd, outbuffer, j);
		zassert(written < 0)
		*outbuffer = 0;
	}
		
	outbuffer[j++] = template[i];
	goto A;
G: 
	t = '%';
	goto _C;

Z:
	written = write(fd, outbuffer, j);
	zassert(written < 0)

	va_end(argp);
}

void dzprintf(int fd, char *template, ...){
	va_list a;
	va_start(a, template);
	_zprintf(fd, template, a);
}

void zprintf(char *template, ...){
	va_list a;
	va_start(a, template);
	_zprintf(STDOUT_FILENO, template, a);
}

void zputb(char *ptr, int length){
	size_t written = write(STDOUT_FILENO, ptr, length);	
	zassert(written < 0)
}
