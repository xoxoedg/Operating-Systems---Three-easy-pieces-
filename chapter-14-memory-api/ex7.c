#include <stdlib.h>

int main() {
    int *p = malloc(3*sizeof(int));
    free(p+1);
    return 0;
}
