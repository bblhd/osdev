#include <string.h>

void *memcpy(void *dest, const void *src, size_t n) {
    uint8_t *d = dest;
    const uint8_t *s = src;
	if (d < s) {
	    while(n--) *d++ = *s++;
	} else {
	    while(n--) d[n] = s[n];
	}
    return dest;
}

void *memcpyr(void *dest, const void *src, size_t n) {
    uint8_t *d = dest;
    const uint8_t *s = src;

    while(n--) d[n] = s[n];

    return dest;
}

size_t strlen(const char* str) {
	size_t len = 0;
	while (str[len]) len++;
	return len;
}

void ultos(unsigned long num, int base, char *out, int max) {
	int i = 0;
	if (num == 0) {
		out[i++] = '0';
		out[i++] = '\0';
		return;
	}
	int j = i;
	while (num > 0 && i < max-1) {
		if (num % base >= 10) {
			out[i++] = 'a' + (num % base) - 10;
		} else {
			out[i++] = '0' + (num % base);
		}
		num /= base;
	}
	out[i--] = '\0';
	char temp;
	while (j < i) {
		temp = out[j];
		out[j++] = out[i];
		out[i--] = temp;
	}
}

void ltos(long num, int base, char *out, int max) {
	if (num < 0) {
		out[0] = '-';
		ultos(-num, base, out+1, max-1);
	} else {
		ultos(num, base, out, max);
	}
}

int streq(char *a, char *b) {
	int i = 0;
	while (a[i] != '\0' && a[i] == b[i]) i++;
	return a[i] == '\0' && b[i] == '\0';
}

int streqn(char *a, char *b, int n) {
	int i = 0;
	while (a[i] != '\0' && a[i] == b[i] && i < n-1) i++;
	return (a[i] == '\0' && b[i] == '\0') || i == n-1;
}

