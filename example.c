#include "malloc_plus_ai.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
  int *x = mallocPlusAI("Enough memory to store up to 5 integers");
  if (!x) {
    printf("got null :(\n");
    return EXIT_FAILURE;
  }

  // possibly UB
  x[3] = 55;
  int y = x[3];
  printf("wrote and read at index 3: %d\n", y);
  free(x);

  return EXIT_SUCCESS;
}
