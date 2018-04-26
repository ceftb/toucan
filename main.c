#include <stdlib.h>
#include "toucan.h"
#include "spock.h"
#include "nets.h"
#include "graphics.h"
#include "layout.h"
#include "link.h"

static void die(struct vulkanrt *r, struct network *n);

int main(void)
{
  Network net = dcomp();
  //Network net = barbell();

  Ptree *ptr = ptree(&net, 1);
  for(int i=0; i<100; i++) {
    layout(&net, ptr);
    if(i%1 == 0) {
      ptr = ptree(&net, 0);
    }
  }

  tesselate_links(&net);
  printf("tesselation size %u (%d MB)\n", net.t, (int)ceil((net.t * sizeof(Point2))/1e6));

  struct vulkanrt r = new_vulkanrt();
  if(init_vulkan(&r))
    die(&r, &net);

  if(init_glfw(&r))
    die(&r, &net);

  printf("configuring vulkan\n");
  if(configure_vulkan(&r, &net))
    die(&r, &net);
  printf("done\n");

  printf("initial draw\n");
  draw(&r);
  printf("done\n");

  while(!glfwWindowShouldClose(r.win)) {
    // the toucan is flying
    glfwPollEvents();
    //draw(&r);
  }

  free_vulkanrt(&r);
  free_network(&net);

  return 0;
}

static void die(struct vulkanrt *r, struct network *n) {
  free_vulkanrt(r);
  free_network(n);
  exit(1);
}


