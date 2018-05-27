#include <stdlib.h>
#include <sys/time.h>
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

  struct timeval start, finish;
  gettimeofday(&start, NULL);
  printf("building tree\n");
  Ptree *ptr = ptree(&net, 1);
  printf("DONE\n");
  for(int i=0; i<50; i++) {
    layout(&net, ptr);
    //if(i%1 == 0) {
      //ptr = ptree(&net, 0);
      balance(ptr);
    //}
  }
  gettimeofday(&finish, NULL);
  size_t d = 
    (finish.tv_sec*1e6 + finish.tv_usec) - 
    (start.tv_sec*1e6 + start.tv_usec);
  printf("time: %f\n", d/1e3);

  //return 0;

  tesselate_links(&net);
  printf("tesselation size %u (%d MB)\n", 
      net.t, (int)ceil((net.t * sizeof(Point2))/1e6));

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


