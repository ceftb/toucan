#include <stdlib.h>
#include "toucan.h"
#include "spock.h"
#include "nets.h"
#include "graphics.h"
#include "layout.h"

static void die(struct vulkanrt *r, struct network *n);

int main(void)
{
  struct network net = barbell_nolayout();

  Ptree *ptr = ptree(&net, 1);
  for(int i=0; i<50; i++) {
    layout(&net, ptr);
    if(i%5 == 0) {
      ptr = ptree(&net, 0);
    }
  }

  struct vulkanrt r = new_vulkanrt();
  if(init_vulkan(&r))
    die(&r, &net);

  if(init_glfw(&r))
    die(&r, &net);

  if(configure_vulkan(&r, &net))
    die(&r, &net);

  while(!glfwWindowShouldClose(r.win)) {
    // the toucan is flying
    glfwPollEvents();
    draw(&r);
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


