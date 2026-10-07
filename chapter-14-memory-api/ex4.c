#include <stdlib.h>

int main() {
    int *p = (int *) malloc(4*sizeof(int));
    p[0] = 1;
    p[1] =2;
    p[2] = 3;
    p[3] = 4;
}
