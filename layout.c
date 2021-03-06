#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include "layout.h"

#define UNUSED(x) (void)(x)

void layout(Network *n, Ptree *ptr)
{
  force(ptr);
  step(ptr);
  constrain(n, ptr);
  step(ptr);
  force(ptr);
}


Ptree* ptree(struct network *net, int init)
{
  /* initialize the ptree with and its root node large enough to fit the 
   * entire network */
  Ptree *ptr = malloc(sizeof(Ptree));
  ptr->leaf_count = net->n;

  /* create a leaf node for every node in the network */
  ptr->leaves = malloc(net->n*sizeof(Plnode));
  for(uint32_t i=0; i<net->n; i++)
  {
    init_lnode(&ptr->leaves[i]);
    ptr->leaves[i].position = &net->nodes[i];
  }

  float init_radius = 2.0f * net->n;

  if(init) {
    /* initially distribute the nodes in a small circle */
    float incr = 2*M_PI/net->n;
    for(uint32_t i=0; i<net->n; i++)
    {
      ptr->leaves[i].position->x = init_radius * cos(i*incr);
      ptr->leaves[i].position->y = init_radius * sin(i*incr);
    }
  }

  Extent b = extent(net, 0, 0);
  ptr->root = new_pinode();
  ptr->root->centroid.x = 0;
  ptr->root->centroid.y = 0;
  ptr->root->diameter = b.width > b.height ? b.width : b.height;
  ptr->root->bounds.left = -b.width/2;
  ptr->root->bounds.right = b.width/2;
  ptr->root->bounds.top = b.height/2;
  ptr->root->bounds.bottom = -b.height/2;
  //printf("root diameter %f\n", ptr->root->diameter);
  printf("root diameter %f\n", diameter(ptr->root));


  /* fill in the valence of each node from the adjacency (link) list. In an
   * adjacency list, the valence of a node is the number of times its index
   * appears in the adjacency list */
  for(uint32_t i=0; i<net->l*2; i++)
  {
    uint32_t x = net->links[i];
    ptr->leaves[x].valence++;
  }

  /* calculate the weight of each node according to valence */
  for(uint32_t i=0; i<net->l*2; i++)
  {
    uint32_t x = net->links[i];
    float v = ptr->leaves[x].valence;
    if(v < 15.0f) {
      v = 15.0f;
    }
    ptr->leaves[x].base.mass = v*10;
  }

  /* insert each leaf into the tree */
  for(uint32_t i=0; i<net->n; i++)
  {
    insert(ptr->root, &ptr->leaves[i]);
  }

  
  return ptr;
}

Ptree* balance(Ptree *old)
{
  Pinode *root = new_pinode();
  Extent b = lextent(old->leaves, old->leaf_count, 0, 0);
  b.width += 500;
  b.height += 500;
  root->diameter = b.width > b.height ? b.width : b.height;
  root->bounds.left = -b.width/2;
  root->bounds.right = b.width/2;
  root->bounds.top = b.height/2;
  root->bounds.bottom = -b.height/2;
  root->centroid.x = 0;
  root->centroid.y = 0;
  //printf("root diameter %f\n", root->diameter);
  printf("root diameter %f\n", diameter(root));
  for(uint32_t i=0; i<old->leaf_count; i++) {
    insert(root, &old->leaves[i]);
  }
  old->root = root;
  return old;
}

/* calcualte the force on each node in @net using the ptree under @root */
void force(Ptree *ptr)
{
  //#pragma omp parallel
  for(uint32_t i=0; i<ptr->leaf_count; i++) {
    qforce(ptr->root, &ptr->leaves[i]);
  }
}

float diameter(Pinode *n) {
  float w = abs(n->bounds.right - n->bounds.left),
        h = abs(n->bounds.top - n->bounds.bottom);

  return sqrt(w*w + h*h);
}

void qf(Pinode *root, Plnode *p, unsigned short j)
{
  Pnode *q = root->quad[j];
  if(q == NULL)
    return;


  float d = distance(*p->position, position(q));

  if(q->type == LEAF) {
    fab(q, p, d);
    return;
  }


  //float s = ((Pinode*)q)->diameter;
  float s = diameter((Pinode*)q);


  /* far enough away to aggregate */
  if(s/d < BHC) {
    /*
    printf("(%f,%f), s=%f, d=%f, s/d=%f\n", 
        p->position->x, p->position->y, s, d, s/d);
        */
    fab(q, p, d);
    return;
  }

  qforce((Pinode*)q, p);
}

/* recursively apply the force of eqach quadrant in @root on @p, aggregating
 * according to the Barnes-Hut constant where possible */
void qforce(Pinode *root, Plnode *p) 
{
  for(unsigned short j=0; j<4; j++) {
    qf(root, p, j);
  }
}

void fab(Pnode *a, Plnode *b, float d)
{
  //float d = distance(position(a), *b->position);
  if(d == 0.0f)
    return;

  float repulse = a->mass / d;
  float theta = angle(position(a), *b->position);
  b->velocity.x += repulse * cos(theta);
  b->velocity.y += repulse * sin(theta);
}

float distance(Point2 a, Point2 b)
{
  float dx = a.x - b.x,
        dy = a.y - b.y;

  return sqrt(dx*dx + dy*dy);
}

float slope(Point2 a, Point2 b)
{
  float dx = a.x - b.x,
        dy = a.y - b.y;

  if(dx == 0)
    return 0;

  return dy/dx;
}

Point2 direction(Point2 a, Point2 b)
{
  Point2 p = {
    .x = b.x - a.x,
    .y = b.y - a.y
  };
  return p;
}

Point2 unit(Point2 a)
{
  float norm = sqrt(a.x*a.x + a.y*a.y);
  Point2 p = {
    .x = a.x / norm,
    .y = a.y / norm
  };
  return p;
}

float angle(Point2 a, Point2 b)
{
  float dx = b.x - a.x,
        dy = b.y - a.y,
        theta = atan2(dy, dx);

  if(theta < 0)
    theta += 2 * M_PI;

  return theta;
}

void update_bounds(Pinode *p, float x, float y) {
  bool recurse = false;

  if(x < p->bounds.left) {
    p->bounds.left = x;
    recurse = true;
  }

  if(x > p->bounds.right) {
    p->bounds.right = x;
    recurse = true;
  }

  if(y < p->bounds.bottom) {
    p->bounds.bottom = x;
    recurse = true;
  }

  if(y > p->bounds.top) {
    p->bounds.top = y;
    recurse = true;
  }

  if(recurse && p->base.parent) {
    update_bounds(p->base.parent, x, y);
  }

}

void step(Ptree *x)
{
  //#pragma omp parallel
  for(uint32_t i=0; i<x->leaf_count; i++)
  {
    Plnode *l = &x->leaves[i];
    l->position->x += l->velocity.x;
    l->position->y += l->velocity.y;

    //TODO consider slowing velocity instead of killing it?
    //     its more physically realisitic and may have nice
    //     effects on convergence / behavior
    l->velocity.x = 0;
    l->velocity.y = 0;

    if(l->base.parent)
      update_bounds(l->base.parent, l->position->x, l->position->y);
  }
}

void constrain(Network *n, Ptree *x)
{
  //#pragma omp parallel
  for(uint32_t i=0; i<(2*n->l)-1; i+=2)
  {
    uint32_t a = n->links[i],
             b = n->links[i+1];
    gab(&x->leaves[a], &x->leaves[b]);
  }
}

void gab(Plnode *a, Plnode *b)
{
  float   av = a->valence,
          bv = b->valence;
  Point2 *da = &a->velocity,
         *db = &b->velocity;

  float d = distance(*a->position, *b->position);
  if(abs(d) < MIN_CONSTRAIN_DISTANCE)
    return;
  
  float theta = angle(*b->position, *a->position);
  db->x += (d / 10.0f / bv) * cos(theta);
  db->y += (d / 10.0f / bv) * sin(theta);

  theta = angle(*a->position, *b->position);
  da->x += (d / 10.0f / av) * cos(theta);
  da->y += (d / 10.0f / av) * sin(theta);
}

void insert(Pinode *root, Plnode *x) 
{
  assert(root != NULL);
  assert(x != NULL);

  //printf("insert: (%f,%f)\n", x->position->x, x->position->y);

  root->base.mass += MASS_INCREMENT;
  //root->base.mass += x->base.mass;

  /* figure out which quad to insert into */
  unsigned short i = ptselect(root, x);
  Pnode *p = root->quad[i];

  /* if the selected quad is empty, we have found a new home for the leaf and
   * we are done */
  if(p == NULL) {
    x->base.parent = root;
    root->quad[i] = (Pnode*)x;

    /* update the centroid */
    Point2 c = {0,0};
    unsigned short d = 0;
    for(unsigned short i=0; i<4 ;i++)
    {
      if(root->quad[i]) {
        d++;
        switch(root->quad[i]->type) {
          case NODE: {
                       Pinode *n = (Pinode*)root->quad[i];
                       c.x += n->centroid.x;
                       c.y += n->centroid.y;
                       break;
                     }
          case LEAF: {
                       Plnode *n = (Plnode*)root->quad[i];
                       c.x += n->position->x;
                       c.y += n->position->y;
                       break;
                     }
        }
      }
    }
    //printf("centroid (%f,%f) [%d]\n", c.x, c.y, d);

    c.x /= d;
    c.y /= d;

    root->centroid = c;

    return;
  }

  switch(p->type) {

    /* if the selected quad is a leaf node, then we subdivide that leaf node by
     * replacing it with a new interior node and then inserting both the old and
     * new leaf into the interior node */
    case LEAF: {
      Pinode *new_node = new_quad(root, i);
      new_node->base.parent = root;
      root->quad[i] = (Pnode*)new_node;
      /*
      printf("split: (%f,%f)| (%f,%f) |(%f, %f)\n", 
          ((Plnode*)p)->position->x, ((Plnode*)p)->position->y,
          root->centroid.x, root->centroid.y,
          x->position->x, x->position->y
      );*/
      insert(new_node, (Plnode*)p); /* existing node */
      insert(new_node, x); /* node currently being inserted */
      break;
    }

    /* if the selected quad is an interior node, then we recurse into that node
    * and march on */
    case NODE:
      insert((Pinode*)p, x);

  }


}

unsigned short ptselect(Pinode *root, Plnode *leaf)
{
  float rx = root->centroid.x,
        ry = root->centroid.y,
        x  = leaf->position->x,
        y  = leaf->position->y;

  if(y >= ry) {
    if(x <= rx)
      return 0;
    else
      return 1;
  } else {
    if(x >= rx)
      return 2;
    else
      return 3;
  }
}

Pinode* new_quad(Pinode *root, unsigned short sector)
{
  Pinode *result = new_pinode();

  /* the diameter of any new quad is half that of its parent */
  //result->diameter = root->diameter / 2.0f;
  result->diameter = diameter(root)/2.0f;

  /* calculate the centroid position of the new pod */
  float shift = diameter(root)/ 4.0f,
        rx = root->centroid.x,
        ry = root->centroid.y;

  /* Quadrant Layout
   *   .-------.
   *   | 0 | 1 |
   *   |---+---|
   *   | 3 | 2 |
   *   '-------'
   */

  //printf("new_quad: %u\n", sector);
  switch(sector) {
    case 0:
      result->centroid.x = rx - shift;
      result->centroid.y = ry + shift;
      result->bounds.left = root->bounds.left;
      result->bounds.top = root->bounds.top;
      result->bounds.right = root->centroid.x;
      result->bounds.bottom = root->centroid.y;
      break;
    case 1:
      result->centroid.x = rx + shift;
      result->centroid.y = ry + shift;
      result->bounds.left = root->centroid.x;
      result->bounds.top = root->bounds.top;
      result->bounds.right = root->bounds.right;
      result->bounds.bottom = root->centroid.y;
      break;
    case 2:
      result->centroid.x = rx + shift;
      result->centroid.y = ry - shift;
      result->bounds.left = root->centroid.x;
      result->bounds.top = root->centroid.y;
      result->bounds.right = root->bounds.right;
      result->bounds.bottom = root->bounds.bottom;
      break;
    case 3:
      result->centroid.x = rx - shift;
      result->centroid.y = ry - shift;
      result->bounds.left = root->bounds.left;
      result->bounds.top = root->centroid.y;
      result->bounds.right = root->centroid.x;
      result->bounds.bottom = root->bounds.bottom;
  }

  return result;
}

Extent extent(Network *net, float cx, float cy)
{
  Extent e = { .width = 0, .height = 0 };

  for(uint32_t i=0; i<net->n; i++)
  {
    float x = net->nodes[i].x,
          y = net->nodes[i].y;
  
    e.width = abs(x) > e.width ? x : e.width;
    e.height = abs(y) > e.height ? y : e.height;
  }

  e.width += cx;
  e.height += cy;

  e.width *= 2;
  e.height *= 2;
  e.width  += 100;
  e.height += 100;

  return e;
}

Extent lextent(Plnode *nodes, uint32_t len, float cx, float cy)
{
  Extent e = { .width = 0, .height = 0 };

  for(uint32_t i=0; i<len; i++)
  {
    float x = nodes[i].position->x,
          y = nodes[i].position->y;
  
    e.width = abs(x) > e.width ? x : e.width;
    e.height = abs(y) > e.height ? y : e.height;
  }

  e.width += cx;
  e.height += cy;

  e.width *= 2;
  e.height *= 2;
  e.width  += 100;
  e.height += 100;

  return e;
}

void init_pnode(Pnode *n)
{
  n->mass = 0;
}

Plnode* new_plnode()
{
  Plnode *l = malloc(sizeof(Plnode));
  init_lnode(l);
  return l;
}
void init_lnode(Plnode *l)
{
  init_pnode(&l->base);
  l->base.type = LEAF;
  l->base.parent = NULL;
  l->valence = 0;
  l->position = NULL;
  l->velocity.x = 0;
  l->velocity.y = 0;
  l->data = NULL;
}


Pinode* new_pinode()
{
  Pinode *n = malloc(sizeof(Pinode));
  init_inode(n);
  return n;
}

void init_inode(Pinode *n)
{
  init_pnode(&n->base);
  n->base.type = NODE;
  n->base.parent = NULL;
  n->bounds.left = 0;
  n->bounds.right = 0;
  n->bounds.top = 0;
  n->bounds.bottom = 0;
  n->quad[0] = NULL;
  n->quad[1] = NULL;
  n->quad[2] = NULL;
  n->quad[3] = NULL;
  n->centroid.x = 0;
  n->centroid.y = 0;
  n->diameter = 0;
}



