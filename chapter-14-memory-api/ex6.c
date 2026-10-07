#include <stdlib.h>
#include <stdio.h>

int main() {
    int *p = malloc(sizeof(int)*2);
    free(p);

    printf("%d\n", p[1]);
    return 0;
}
