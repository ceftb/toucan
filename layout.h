#pragma once
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * toucan ptree
 * ============-
 *   This file implements a PR-type quadtree that is used for network
 *   visualization. The primary motivation for using this data structure is
 *   that it allows us to compute a force directed layout in l*log(n) time
 *   using the Barnes-Hut algorithm, as opposed to the typical n^2 approach
 *
 *   The quadtree is composed of two node types - interior (Pnode) and leaf
 *   (Pleaf). Leaf nodes contain data and interior nodes are purely for data
 *   oranization e.g. this is a really a *trie*, but most literature refers to 
 *   it as a tree. A good reference on this data structure is 'Foundations of
 *   Multidimensional and Metric Data Structures' by Hanan Samet - see section
 *   1.4.2.2.
 *
 *	Copyright ceftb 2018 - All Rights Reserved
 *	License: Apache 2.0
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#include "toucan.h"

/* the Barnes-Hut constant */
#define BHC 0.5

/* the ammount to increment the mass of an interior node by when a new leaf
 * is added to it */
#define MASS_INCREMENT 1

/* any closer than this distance and constaint forces cease to function, e.g.
 * the resting length of the spring */
#define MIN_CONSTRAIN_DISTANCE 10

/* the radius of the initial layout distribution circle, before the layout
 * algorithm begins, the nodes are laid out evenly over the edge of a circle
 * with this radius */
#define INITIAL_RADIUS 100

typedef enum PNTYPE { NODE, LEAF } PnType;

typedef struct extent {
  float width,
        height;
} Extent;

/* Ptree base node */
typedef struct pnode {
  PnType type;
  float  mass;
} Pnode ;

/* Ptree interior node */
typedef struct pinode {
  Pnode   base;
  Pnode   *quad[4];
  Point2  centroid;
  float   diameter;
} Pinode;

/* Ptree leaf node */
typedef struct plnode {
  Pnode    base;
  Point2   *position;
  Point2   velocity;
  uint32_t valence;
  void     *data;
} Plnode;

typedef struct ptree {
  Pinode *root;
  Plnode *leaves;
  uint32_t leaf_count;
} Ptree;

void init_pnode();

Pinode* new_pinode();
void init_inode();

Plnode* new_plnode();
void init_lnode(Plnode *);

Ptree* ptree(Network*, int init);
void layout(Network*, Ptree*);

void force(Ptree*);
void qforce(Pinode*, Plnode*);
void fab(Pnode*, Plnode*);
void gab(Plnode*, Plnode*);
float distance(Point2, Point2);
float angle(Point2, Point2);
void step(Ptree*);
void constrain(Network*, Ptree*);
void insert(Pinode*, Plnode*);
Extent extent(Network*, float, float);
unsigned short ptselect(Pinode*, Plnode*);
Pinode* new_quad(Pinode*, unsigned short);

static inline Point2* positionp(Pnode *x)
{
  switch(x->type) {
    case LEAF:
      return ((Plnode*)x)->position;
    case NODE:
    default:
      return &((Pinode*)x)->centroid;
  }
}

static inline Point2 position(Pnode *x)
{
  switch(x->type) {
    case LEAF:
      return *(((Plnode*)x)->position);
    case NODE:
    default:
      return ((Pinode*)x)->centroid;
  }
}

