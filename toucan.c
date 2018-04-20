#include <stdlib.h>
#include "toucan.h"

void free_network(struct network *n)
{
  free(n->nodes);
  n->nodes = NULL;
  free(n->links);
  n->links = NULL;
}
