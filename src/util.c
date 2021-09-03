#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

void die(const char *msg){
	fprintf(stderr, "%s", msg);
	exit(1);
}

// utf8 decoder: https://gist.github.com/tylerneylon/9773800
int decode_code_point(char **s) {
	int k = **s ? __builtin_clz(~(**s << 24)) : 0;  // Count # of leading 1 bits.
	int mask = (1 << (8 - k)) - 1;                  // All 1's with k leading 0's.
	int value = **s & mask;
	for (++(*s), --k; k > 0 && **s; --k, ++(*s)) {  // Note that k = #total bytes, or 0.
		value <<= 6;
		value += (**s & 0x3F);
	}
	return value;
}

int getActualSize(char *s){
	int l = 0;
	char *t, *next;
	for(t = s; t - s < strlen(s); t = next) {
		if(*t == '\0')
			break;
		next = t;
		decode_code_point(&next);
		l++;
	}
	return l;
}
