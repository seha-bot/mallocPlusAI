#include "malloc_plus_ai.h"
#include <stdlib.h>

int main() {
  int *x = (int *)mallocPlusAI("Enough memory to store up to 5 integers");
  x[3] = 55;
  int y = x[3];
  free(x);

  return y;
}
