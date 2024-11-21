#include <stdio.h>
#include <stdlib.h>

#include "malloc_plus_ai.h"

int main(void) {
    int *x = mallocPlusAI("Enough memory to store up to 5 integers");
    if (!x) {
        printf("got null :(\n");
        return EXIT_FAILURE;
    }

    printf("ok :)\n");
    free(x);
    return EXIT_SUCCESS;
}
