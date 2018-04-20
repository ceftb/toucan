#include <string.h>
#include <stdlib.h>

#include "nets.h"

/* A barbell network
 *
 *  1.          .5
 *    `.      .'
 *      2----3     
 *    .'      '.
 *  0'          `4
 *
 */
struct network barbell()
{
  struct network net = {
    .n = 6,
    .l = 5,
    .nodes = malloc(net.n*sizeof(struct point2)),
    .links = malloc(net.l*2*sizeof(uint32_t))
  };

  struct point2 nodes[] = {
    {-100.0,  100.0},
    {-100.0, -100.0},
    {-50.0,   0.0},
    { 50.0,   0.0},
    { 100.0,  100.0},
    { 100.0, -100.0}
  };
  memcpy(net.nodes, nodes, net.n*sizeof(struct point2));

  uint32_t links[] = {
    0, 2,
    1, 2,
    2, 3,
    3, 4,
    3, 5
  };
  memcpy(net.links, links, net.l*2*sizeof(uint32_t));

  return net;
}
