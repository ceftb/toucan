#include <stdlib.h>
#include "link.h"
#include "layout.h"
#include "math.h"

#define INTERPOLATION 10

void tesselate_links(Network *n)
{
  for(size_t i=0; i<((n->l*2) - 1); i+=2) {
    tesselate_link(n, n->links[i], n->links[i+1]);
  }
}

void tesselate_link(Network *n, uint32_t x, uint32_t y)
{
  Point2 a = n->nodes[x],
         b = n->nodes[y];

  /* calculate the distance and slope of the line and create one tesselation 
   * point for each unit of distance along the path */
  uint32_t d = ceil(distance(a,b)/INTERPOLATION),
           t_start = n->t,
           t_end = n->t + d;
  Point2 dp = unit(direction(a,b));
  n->tlinks = realloc(n->tlinks, t_end*sizeof(Point2));

  for(uint32_t i=0; i<d; i++) {
    a.x += dp.x*INTERPOLATION;
    a.y += dp.y*INTERPOLATION;
    n->tlinks[t_start+i] = a;
  }
  
  n->t = t_end;
}
