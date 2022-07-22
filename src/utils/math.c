#include <math.h>

int mod(int x, int y) {
	while (x < 0) x += y;
	while (x >= y) x -= y;
	return x;
}
