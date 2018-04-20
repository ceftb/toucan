#include <stdlib.h>
#include "toucan.h"
#include "spock.h"
#include "nets.h"
#include "graphics.h"

static void die(struct vulkanrt *r, struct network *n);

int main(void)
{
  struct vulkanrt r = new_vulkanrt();
  struct network net = barbell();

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


