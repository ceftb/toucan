#pragma once
#include <stdint.h>

typedef struct point2 {
  float x, y;
} Point2 ;


typedef struct network {
  uint32_t  n,        /* number of nodes */
            l,        /* number of links */
            t;        /* tesselation size */
  Point2    *nodes; 
  uint32_t  *links;   /* adjacency list of links */
  Point2    *tlinks;  /* tesselated link lines */
} Network;

void free_network(struct network*);

