#include <math.h>

int mod(int x, int y) {
	return x % y + (x < 0 ? y : 0);
}

int max(int x, int y) {
	return x > y ? x : y;
}

int min(int x, int y) {
	return x < y ? x : y;
}
