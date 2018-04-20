#pragma once
#include <stdint.h>

struct point2 {
  float x, y;
};

struct network {
  uint32_t n,  /* number of nodes */
           l;  /* number of links */
  struct point2 *nodes;
  uint32_t *links;
};


void free_network(struct network*);

