#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <dlfcn.h>
#include <string.h>
#include <sys/stat.h>

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

//#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

/* The number of vulkan extensions required */
#define VK_EXTNS 4
const char* vkx[VK_EXTNS] = {
  "VK_KHR_surface",
  "VK_KHR_xcb_surface",
  "VK_KHR_xlib_surface",
  "VK_KHR_wayland_surface",
};

VkApplicationInfo application_info = {
  .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
  .pNext = NULL,
  .pApplicationName = "toucan",
  .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
  .pEngineName = NULL,
  .engineVersion = 0,
  .apiVersion = VK_MAKE_VERSION(1, 0, 0)
};

VkInstanceCreateInfo instance_create_info = {
  .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
  .pNext = NULL,
  .flags = 0,
  .pApplicationInfo = &application_info,
  .enabledLayerCount = 0,
  .ppEnabledLayerNames = NULL, 
  .enabledExtensionCount = 4,
  .ppEnabledExtensionNames = vkx
};

struct point2 {
  float x, y;
};

struct network {
  uint32_t n,  /* number of nodes */
           l;  /* number of links */
  struct point2 *nodes;
  uint32_t *links;
};

struct network_resources {
  VkBuffer node_buffer,
           link_buffer;

  VkDeviceMemory node_mem,
                 link_mem;
};

struct image_resources {
  VkImage handle;
  VkImageView view;
  VkSampler sampler;
  VkDeviceMemory memory;
};

struct resources {
  /* vulkan */
  void *vk;
  VkInstance vki;
  VkPhysicalDevice vkpd;
  VkDevice vkd;

  uint32_t graphicsq_family_index, 
           presentq_family_index,
           graphicsq_index,
           presentq_index;

  VkQueue graphicsq,
          presentq;

  PFN_vkGetInstanceProcAddr vkl;
  PFN_vkGetDeviceProcAddr vkdl;
  VkSurfaceKHR surface;
  VkCommandPool vkp;
  //uint32_t vkb_size; tied to number of images (nimg)
  VkCommandBuffer *vkb;
  VkShaderModule vkvert, vkfrag;
  size_t vert_sipr_sz, frag_sipr_sz;
  uint32_t *vert_sipr, *frag_sipr;

  uint32_t nvib;
  VkVertexInputBindingDescription *vibs;
  uint32_t nvattr;
  VkVertexInputAttributeDescription *vattrs;

  uint32_t nviewports;
  VkViewport *viewports;
  uint32_t nscissors;
  VkRect2D *scissors;

  VkSwapchainKHR swapchain, old_swapchain;
  VkFormat format;
  uint32_t nimg;
  struct image_resources *images;
  VkFramebuffer *framebuffers;
  VkExtent2D extent;

  VkRenderPass render_pass;

  VkPipeline node_pipeline, link_pipeline;
  VkPipelineLayout node_pipeline_layout, link_pipeline_layout;

  VkSemaphore image_ready,
              rendering_finished;

  VkExtent2D surface_area;
  VkClearValue clear;

  /* glfw */
  GLFWwindow *win;

  /* data */
  struct network data;
  struct network_resources bufs;
  uint32_t memory_type_index;

};



static int init_glfw(struct resources*);
static void glfw_error_callback(int error, const char* description);
static int init_vulkan(struct resources*);
static int alloc_resources(struct resources*);
static int free_resources(struct resources*);
static struct network init_data();
static int net_bufs(struct resources*, struct network);
static int bind_bufs(struct resources*);
static int create_command_pool(struct resources*);
static int load_shaders(struct resources*);

static int init_graphics_pipeline(struct resources*, VkPrimitiveTopology pt,
    VkPipelineLayout *layout, VkPipeline *pipeline);

static int create_swapchain(struct resources*);
static int create_render_pass(struct resources*);
static int create_framebuffers(struct resources*);
static int create_semaphores(struct resources*);
static int record_command_buffers(struct resources*);
static int vk_get_queue_info(struct resources*);
static void draw(struct resources*);

static void die(struct resources *r) {
  free_resources(r);
  exit(1);
}

int main(void)
{
  // TODO just bzero the fucking thing and set what remains
  struct resources r = {
    /* vulkan */
    .vk = NULL,
    .vki = NULL,
    .vkpd = NULL,
    .vkd = NULL,
    .vkl = NULL,
    .vkp = NULL,
    .vkb = NULL,
    .vkvert = NULL,
    .vkfrag = NULL,
    .vert_sipr = NULL,
    .frag_sipr = NULL,
    .surface = NULL,
    .vibs = NULL,
    .vattrs = NULL,
    .viewports = NULL,
    .scissors = NULL,
    .images = NULL,
    .framebuffers = NULL,
    .graphicsq_family_index = 0,
    .presentq_family_index = 0,
    .graphicsq_index = 0,
    .presentq_index = 0,
    .vert_sipr_sz = 0,
    .frag_sipr_sz = 0,
    .nvib = 0,
    .nvattr = 0,
    .nviewports = 1,
    .nscissors = 1,
    .nimg = 0,
    .graphicsq = VK_NULL_HANDLE,
    .presentq = VK_NULL_HANDLE,
    .swapchain = VK_NULL_HANDLE,
    .old_swapchain = VK_NULL_HANDLE,
    .render_pass = VK_NULL_HANDLE,
    .node_pipeline = VK_NULL_HANDLE,
    .node_pipeline_layout = VK_NULL_HANDLE,
    .link_pipeline = VK_NULL_HANDLE,
    .link_pipeline_layout = VK_NULL_HANDLE,
    .image_ready = VK_NULL_HANDLE,
    .rendering_finished = VK_NULL_HANDLE,

    /* glfw */
    .win = 0,

    .surface_area = {
      .width = 1000,
      .height = 700
    },

    .bufs = {
      .node_buffer = NULL,
      .link_buffer = NULL,
      .node_mem = VK_NULL_HANDLE,
      .link_mem = VK_NULL_HANDLE
    },

    .extent = {
      .width = 0,
      .height = 0,
    },

    .clear = {
      .color.float32 = {0.01f, 0.01f, 0.01f}
    }
  };

  if(alloc_resources(&r))
    die(&r);

  r.data = init_data();

  if(vk_get_queue_info(&r))
    die(&r);

  if(net_bufs(&r, r.data)) 
    die(&r);

  if(bind_bufs(&r))
    die(&r);

  if(load_shaders(&r))
    die(&r);

  if(create_swapchain(&r))
    die(&r);

  if(create_render_pass(&r))
    die(&r);

  if(create_framebuffers(&r))
    die(&r);

  if(init_graphics_pipeline(&r, VK_PRIMITIVE_TOPOLOGY_POINT_LIST, 
        &r.node_pipeline_layout, &r.node_pipeline))
    die(&r);

  if(init_graphics_pipeline(&r, VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
        &r.link_pipeline_layout, &r.link_pipeline))
    die(&r);

  if(create_semaphores(&r))
    die(&r);

  if(create_command_pool(&r))
    die(&r);

  if(record_command_buffers(&r))
    die(&r);

  while(!glfwWindowShouldClose(r.win)) {
    // the toucan is flying
    glfwPollEvents();
    draw(&r);
  }

  free_resources(&r);
}

static int alloc_resources(struct resources *r)
{
  int err = init_vulkan(r);
  if(err) {
    return err;
  }
  err = init_glfw(r);
  if(err) {
    return err;
  }
  return err;
}

#define VKFN(loader, name, instance) \
  PFN_##name name = (PFN_##name) loader(instance, #name); \
  if(!name) { \
    fprintf(stderr, "failed to load " #name "\n"); \
  }

static int free_resources(struct resources *r)
{
  int ret = 0;
  int err;

  /* destroy the data */
  printf("destroying data\n");
  VKFN(r->vkdl, vkFreeMemory, r->vkd);
  if(vkFreeMemory) {
    vkFreeMemory(r->vkd, r->bufs.node_mem, NULL);
    vkFreeMemory(r->vkd, r->bufs.link_mem, NULL);
  }

  VKFN(r->vkdl, vkDestroyBuffer, r->vkd);
  if(vkDestroyBuffer) {
    vkDestroyBuffer(r->vkd, r->bufs.node_buffer, NULL);
    vkDestroyBuffer(r->vkd, r->bufs.link_buffer, NULL);
  }

  free(r->data.nodes);
  r->data.nodes = NULL;
  free(r->data.links);
  r->data.links = NULL;

  /* destroy the drawing surface */
  printf("destroying surface\n");
  VKFN(r->vkl, vkDestroySurfaceKHR, r->vki);
  if(vkDestroySurfaceKHR)
    vkDestroySurfaceKHR(r->vki, r->surface, NULL);

  /* destroy the glfw window */
  printf("destroying window\n");
  glfwDestroyWindow(r->win);

  printf("shutting down glfw\n");
  glfwTerminate();

  /* destroy swapchain */
  printf("destroying swapchain\n");
  VKFN(r->vkdl, vkDestroySwapchainKHR, r->vkd);
  if(vkDestroySwapchainKHR) {
    vkDestroySwapchainKHR(r->vkd, r->swapchain, NULL);
    if(r->old_swapchain != VK_NULL_HANDLE)
      vkDestroySwapchainKHR(r->vkd, r->old_swapchain, NULL);
  }

  /* destroy views images */
  printf("destroying image views\n");
  VKFN(r->vkdl, vkDestroyImageView, r->vkd);
  if(vkDestroyImageView) {
    for(uint32_t i=0; i<r->nimg; i++) {
      vkDestroyImageView(r->vkd, r->images[i].view, NULL);
    }
  }
  // TODO destroy 
  //   - images
  //   - samples
  //   - memory
  free(r->images);

  /* destroy framebuffers */
  printf("destroying framebuffers\n");
  VKFN(r->vkdl, vkDestroyFramebuffer, r->vkd);
  if(vkDestroyFramebuffer) {
    for(uint32_t i=0; i<r->nimg; i++) {
      vkDestroyFramebuffer(r->vkd, r->framebuffers[i], NULL);
    }
  }
  free(r->framebuffers);

  /* destroy render pass */
  printf("destroying render pass\n");
  VKFN(r->vkdl, vkDestroyRenderPass, r->vkd);
  if(vkDestroyRenderPass)
    vkDestroyRenderPass(r->vkd, r->render_pass, NULL);

  /* destroy pipeline */
  printf("destroying pipeline\n");
  VKFN(r->vkdl, vkDestroyPipeline, r->vkd);
  if(vkDestroyPipeline) {
    vkDestroyPipeline(r->vkd, r->node_pipeline, NULL);
    vkDestroyPipeline(r->vkd, r->link_pipeline, NULL);
  }
  VKFN(r->vkdl, vkDestroyPipelineLayout, r->vkd);
  if(vkDestroyPipelineLayout) {
    vkDestroyPipelineLayout(r->vkd, r->node_pipeline_layout, NULL);
    vkDestroyPipelineLayout(r->vkd, r->link_pipeline_layout, NULL);
  }

  /* destroy semaphores */
  printf("destroying semaphores\n");
  VKFN(r->vkdl, vkDestroySemaphore, r->vkd);
  if(vkDestroySemaphore) {
    vkDestroySemaphore(r->vkd, r->image_ready, NULL);
    vkDestroySemaphore(r->vkd, r->rendering_finished, NULL);
  }

  /* destroy the command buffer */
  printf("destroying command buffer\n");
  VKFN(r->vkdl, vkFreeCommandBuffers, r->vkd);
  if(vkFreeCommandBuffers)
    vkFreeCommandBuffers(r->vkd, r->vkp, r->nimg, r->vkb);
  free(r->vkb);
  r->vkb = NULL;

  /* destroy the command pool */
  printf("destroying command pool\n");
  VKFN(r->vkdl, vkDestroyCommandPool, r->vkd);
  if(vkDestroyCommandPool)
    vkDestroyCommandPool(r->vkd, r->vkp, NULL);

  /* destroy shader resources */
  printf("destroying shaders\n");
  VKFN(r->vkdl, vkDestroyShaderModule, r->vkd);
  if(vkDestroyShaderModule) {
    vkDestroyShaderModule(r->vkd, r->vkvert, NULL);
    vkDestroyShaderModule(r->vkd, r->vkfrag, NULL);
  }
  free(r->vert_sipr);
  r->vert_sipr = NULL;
  free(r->frag_sipr);
  r->frag_sipr = NULL;

  /* destroy the vulkan device */
  printf("destroying vulkan device\n");
  VKFN(r->vkl, vkDestroyDevice, r->vki);
  if(vkDestroyDevice)
    vkDestroyDevice(r->vkd, NULL);


  /* destroy the vulkan instance */
  printf("destroying the vulkan instance\n");
  VKFN(r->vkl, vkDestroyInstance, r->vki);
  if(vkDestroyInstance)
    vkDestroyInstance(r->vki, NULL);


  /* close the vulkan library */
  printf("closing vulkan library\n");
  if(r->vk) {
    err = dlclose(r->vk);
    ret |= err;
    if(err) {
      fprintf(stderr, "failed to close vulkan\n");
    }
  }

  return ret;
}

static int vk_create_instance(struct resources *r)
{
  VKFN(r->vkl, vkEnumerateInstanceExtensionProperties, NULL);
  if(!vkEnumerateInstanceExtensionProperties)
    return 1;

  uint32_t n;
  if(vkEnumerateInstanceExtensionProperties(NULL, &n, NULL) != VK_SUCCESS) {
    fprintf(stderr, "failed to query vulkan extensions\n");
    return 1;
  }

  VkExtensionProperties *vxt = malloc(n*sizeof(VkExtensionProperties));
  if(vkEnumerateInstanceExtensionProperties(NULL, &n, vxt) != VK_SUCCESS) {
    fprintf(stderr, "failed to load vulkan extensions\n");
    free(vxt);
    return 1;
  }

  printf("extensions:\n");
  for(uint32_t i=0; i<n; i++)
  {
    printf("  %s\n", vxt[i].extensionName);
  }
  free(vxt);

  VKFN(r->vkl, vkCreateInstance, NULL);
  if(!vkCreateInstance)
    return 1;

  if(vkCreateInstance(&instance_create_info, NULL, &r->vki) != VK_SUCCESS) {
    fprintf(stderr, "failed to create vulkan instance\n");
    return 1;
  }

  return 0;
}

static int vk_select_device(struct resources *r)
{
  VKFN(r->vkl, vkEnumeratePhysicalDevices, r->vki);
  if(!vkEnumeratePhysicalDevices)
    return 1;

  uint32_t ndev;
  if(vkEnumeratePhysicalDevices(r->vki, &ndev, NULL)) {
    fprintf(stderr, "failed to query physical vulkan devices\n");
    return 1;
  }

  VkPhysicalDevice *devices = malloc(ndev*sizeof(VkPhysicalDevice));
  if(vkEnumeratePhysicalDevices(r->vki, &ndev, devices)) {
    fprintf(stderr, "failed to load physical vulkan devices\n");
    free(devices);
    return 1;
  }

  VKFN(r->vkl, vkEnumerateDeviceExtensionProperties, r->vki);
  if(!vkEnumerateDeviceExtensionProperties) {
    free(devices);
    return 1;
  }

  VKFN(r->vkl, vkGetPhysicalDeviceProperties, r->vki);
  if(!vkGetPhysicalDeviceProperties) {
    free(devices);
    return 1;
  }

  int ret = 0;
  for(uint32_t i=0; i<ndev; i++) {
    printf("device-%u:\n", i);
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(devices[i], &props);
    printf("  name: %s (%x)\n", props.deviceName, props.deviceID);
    uint32_t n_ext;
    if(vkEnumerateDeviceExtensionProperties(devices[i], NULL, &n_ext, NULL) != VK_SUCCESS)
    {
      fprintf(stderr, "failed to query properties for vulkan device %u\n", i);
      ret = 1;
    }
    else {
      VkExtensionProperties *xps = malloc(n_ext*sizeof(VkExtensionProperties));
      if(vkEnumerateDeviceExtensionProperties(devices[i], NULL, &n_ext, xps) != VK_SUCCESS)
      {
        fprintf(stderr, "failed to load properties for vulkan device %u\n", i);
        ret = 1;
      }
      else {
        for(uint32_t j=0; j<n_ext; j++) {
          printf("  %s\n", xps[j].extensionName);
        }
      }
      free(xps);
    }
  }

  if(ndev == 0) {
    ret = 1;
    fprintf(stderr, "no vulkan devices found\n");
  } else {
    /* XXX just taking the first device for now */
    printf("selected device-0\n");
    r->vkpd = devices[0];
  }

  free(devices);

  VKFN(r->vkl, vkGetPhysicalDeviceFeatures, r->vki);
  if(!vkGetPhysicalDeviceFeatures)
    return 1;
  VkPhysicalDeviceFeatures pdf;
  vkGetPhysicalDeviceFeatures(r->vkpd, &pdf);
  if(pdf.largePoints != VK_TRUE)
    printf("warning: large points not supported!\n");
  if(pdf.wideLines != VK_TRUE)
    printf("warning: wide lines not supported!\n");

  return ret;
}

static int vk_get_queue_info(struct resources *r)
{
  VKFN(r->vkl, vkGetPhysicalDeviceQueueFamilyProperties, r->vki);
  if(!vkGetPhysicalDeviceQueueFamilyProperties)
    return 1;

  VKFN(r->vkl, vkGetPhysicalDeviceSurfaceSupportKHR, r->vki);
  if(!vkGetPhysicalDeviceSurfaceSupportKHR)
    return 1;

  /* get the physical device queue family properties */
  uint32_t n;
  vkGetPhysicalDeviceQueueFamilyProperties(r->vkpd, &n, NULL);
  VkQueueFamilyProperties *qps = malloc(n*sizeof(VkQueueFamilyProperties));
  vkGetPhysicalDeviceQueueFamilyProperties(r->vkpd, &n, qps);

  for(uint32_t i=0; i<n; i++){

    VkBool32 present_support;
    vkGetPhysicalDeviceSurfaceSupportKHR(r->vkpd, i, r->surface, 
        &present_support);

    printf("queue-%u:\n", i);
    printf("  type:");
    if(qps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      printf(" graphics");
    }
    if(qps[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
      printf(" compute");
    }
    if(qps[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
      printf(" transfer");
    }
    if(qps[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) {
      printf(" sparse");
    }
    printf("\n");
    printf("  count: %u\n", qps[i].queueCount);

    /* XXX only support combined graphics/present queues for now */
    if(qps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT &&
        present_support == VK_TRUE) {
      /* mark the queue family as selected */
      r->graphicsq_family_index = i;
      r->presentq_family_index = i;
      /* grab the first queue in the family */
      r->graphicsq_index = 0;
      r->presentq_index = 0;
    }
    printf("using queue-family %d index %d\n",
        r->graphicsq_family_index,
        r->graphicsq_index);
  }

  free(qps);

  printf("selected queue-%u\n", r->graphicsq_family_index);

  return 0;
}

static int vk_create_device(struct resources *r) {

  /* set the device properties
   * notes:
   *  - only using one queue with a priority of 1 for now
   *  - not explicitly enabling features
   *  - the only extensions that are enabled are
   *    + swapchain
   */

  float qp[1] = { 1.0 };

  VkDeviceQueueCreateInfo q_infos[1] = {
    {
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .queueFamilyIndex = r->graphicsq_family_index,
      .queueCount = 1,
      .pQueuePriorities = qp
    }
  };

  const char* exts[1] = {
    "VK_KHR_swapchain"
  };

  VkDeviceCreateInfo vci = {
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .queueCreateInfoCount = 1,
    .pQueueCreateInfos = q_infos,
    .enabledLayerCount = 0,
    .ppEnabledLayerNames = NULL,
    .enabledExtensionCount = 1,
    .ppEnabledExtensionNames = exts,
    .pEnabledFeatures = NULL
  };

  /* create the device */

  VKFN(r->vkl, vkCreateDevice, r->vki);
  if(!vkCreateDevice)
    return 1;

  int res = vkCreateDevice(r->vkpd, &vci, NULL, &r->vkd);
  if(res != VK_SUCCESS) {
    fprintf(stderr, "failed to create vulkan device %d\n", res);
    return 1;
  }

  /* get a handle for the logical device queue */

  VKFN(r->vkdl, vkGetDeviceQueue, r->vkd);
  if(!vkGetDeviceQueue)
    return 1;
  vkGetDeviceQueue(r->vkd, r->graphicsq_family_index, r->graphicsq_index, 
      &r->graphicsq);

  /* XXX considering graphicsq and presentq to be same for now */
  r->presentq = r->graphicsq;

  return 0;

}

static int init_vulkan(struct resources *r)
{
  r->vk = dlopen("libvulkan.so.1", RTLD_NOW);
  if(!r->vk){

    fprintf(stderr, "failed to load vulkan\n");
    return 1;
  }

  r->vkl = (PFN_vkGetInstanceProcAddr)dlsym(r->vk, "vkGetInstanceProcAddr");
  if(!r->vkl){
    fprintf(stderr, "failed to load vulkan instance loader\n");
    return 1;
  }

  r->vkdl = (PFN_vkGetDeviceProcAddr)dlsym(r->vk, "vkGetDeviceProcAddr");
  if(!r->vkdl) {
    fprintf(stderr, "failed to load vulkan device loader\n");
    return 1;
  }

  int err = vk_create_instance(r);
  if(err) {
    return 1;
  }

  err = vk_select_device(r);
  if(err) {
    return 1;
  }


  err = vk_create_device(r);
  if(err) {
    return 1;
  }

  return err;
}

/* A set of coordintes that describes the following network
 *
 *  1.          .5
 *    `.      .'
 *      2----3     
 *    .'      '.
 *  0'          `4
 *
 */
static struct network init_data()
{
  struct network net = {
    .n = 6,
    .l = 5,
    .nodes = malloc(net.n*sizeof(struct point2)),
    .links = malloc(net.l*2*sizeof(uint32_t))
  };

  struct point2 nodes[] = {
    {-0.8,  0.8},
    {-0.8, -0.8},
    {-0.4,  0.0},
    { 0.4,  0.0},
    { 0.8,  0.8},
    { 0.8, -0.8}
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

static int net_bufs(struct resources *r, struct network net)
{
  VkBufferCreateInfo nbi = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .size = net.n*sizeof(struct point2),
    .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .queueFamilyIndexCount = 0,
    .pQueueFamilyIndices = NULL
  };

  VkBufferCreateInfo lbi = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .size = net.l*2*sizeof(uint32_t),
    .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .queueFamilyIndexCount = 0,
    .pQueueFamilyIndices = NULL
  };

  VKFN(r->vkdl, vkCreateBuffer, r->vkd);
  if(!vkCreateBuffer)
    return 1;

  int res = vkCreateBuffer(r->vkd, &nbi, NULL, &r->bufs.node_buffer);
  if(res != VK_SUCCESS) {
    fprintf(stderr, "creating node buffer failed (%d)", res);
    return 1;
  }

  res = vkCreateBuffer(r->vkd, &lbi, NULL, &r->bufs.link_buffer);
  if(res != VK_SUCCESS) {
    fprintf(stderr, "creating link buffer failed (%d)", res);
    return 1;
  }

  return 0;
}

static int bind_bufs(struct resources *r)
{
  /* query device memory type information */
  VKFN(r->vkl, vkGetPhysicalDeviceMemoryProperties, r->vki);
  if(!vkGetPhysicalDeviceMemoryProperties)
    return 1;

  VkPhysicalDeviceMemoryProperties props;
  vkGetPhysicalDeviceMemoryProperties(r->vkpd, &props);

  int chosen_mem_index = -1;
  for(uint32_t i=0; i<props.memoryTypeCount; i++) {
    VkMemoryType *t = &props.memoryTypes[i];
    printf("mem-%d:\n", i);
    printf("  heap: %u\n", t->heapIndex);
    printf("  flags:");
    if(t->propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
      printf(" local");
    if(t->propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
      printf(" host-visible");
    if(t->propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
      printf(" host-coherent");
    if(t->propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
      printf(" host-cached");
    if(t->propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
      printf(" lazily-allocated");
    printf("\n");

    /* choose host visible memory */
    if(t->propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
      chosen_mem_index = i;
    }
  }
  for(uint32_t i=0; i<props.memoryHeapCount; i++) {
    VkMemoryHeap *h = &props.memoryHeaps[i];
    printf("heap-%d:\n", i);
    printf("  heap-flags:");
    if(h->flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
      printf(" device-local");
    printf("\n");
    printf("  heap-size: %lu\n", h->size);
  }

  if(chosen_mem_index == -1) {
    fprintf(stderr, "device has no host visible local memory - cannot continue\n");
    return 1;
  } else {
    r->memory_type_index = (uint32_t)chosen_mem_index;
  }

  /* create memory requirements */
  VKFN(r->vkdl, vkGetBufferMemoryRequirements, r->vkd);
  if(!vkGetBufferMemoryRequirements)
    return 1;

  VkMemoryRequirements nmem_req,
                       lmem_req;
  vkGetBufferMemoryRequirements(r->vkd, r->bufs.node_buffer, &nmem_req);
  vkGetBufferMemoryRequirements(r->vkd, r->bufs.link_buffer, &lmem_req);

  printf("node memory %lu\n", nmem_req.size);

  /* allocate the memory */
  VKFN(r->vkdl, vkAllocateMemory, r->vkd);
  if(!vkAllocateMemory)
    return 1;
  
  VkMemoryAllocateInfo nmem_alloc = {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .pNext = NULL,
    .allocationSize = nmem_req.size,
    .memoryTypeIndex = r->memory_type_index
  };
  int res = vkAllocateMemory(r->vkd, &nmem_alloc, NULL, &r->bufs.node_mem);
  if(res !=  VK_SUCCESS) {
    fprintf(stderr, "failed to allocate node buffer memory (%d)\n", res);
    return 1;
  }
  if(r->bufs.node_mem == VK_NULL_HANDLE) {
    fprintf(stderr, "node allocation resulted in null handle\n");
    return 1;
  }

  VkMemoryAllocateInfo lmem_alloc = {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .pNext = NULL,
    .allocationSize = lmem_req.size,
    .memoryTypeIndex = r->memory_type_index
  };
  res = vkAllocateMemory(r->vkd, &lmem_alloc, NULL, &r->bufs.link_mem);
  if(res !=  VK_SUCCESS) {
    fprintf(stderr, "failed to allocate link buffer memory (%d)\n", res);
    return 1;
  }
  if(r->bufs.link_mem == VK_NULL_HANDLE) {
    fprintf(stderr, "link allocation resulted in null handle\n");
    return 1;
  }

  /* bind the memory */
  VKFN(r->vkdl, vkBindBufferMemory, r->vkd);
  if(!vkBindBufferMemory)
    return 1;

  res = vkBindBufferMemory(r->vkd, r->bufs.node_buffer, r->bufs.node_mem, 0);
  if(res != VK_SUCCESS) {
    fprintf(stderr, "binding node memory failed (%d)\n", res);
    return 1;
  }

  res = vkBindBufferMemory(r->vkd, r->bufs.link_buffer, r->bufs.link_mem, 0);
  if(res != VK_SUCCESS) {
    fprintf(stderr, "binding link memory failed (%d)\n", res);
    return 1;
  }

  VKFN(r->vkdl, vkMapMemory, r->vkd);
  if(!vkMapMemory)
    return 1;

  /* map memory and copy vertex data */
  void *vm, *lm;
  res = vkMapMemory(r->vkd, r->bufs.node_mem, 0, nmem_req.size, 0, &vm);
  if(res != VK_SUCCESS) {
    fprintf(stderr, "failed to map node memory for copying (%d)\n", res);
    return 1;
  }
  memcpy(vm, r->data.nodes, nmem_req.size);

  res = vkMapMemory(r->vkd, r->bufs.link_mem, 0, lmem_req.size, 0, &lm);
  if(res != VK_SUCCESS) {
    fprintf(stderr, "failed to map link memory for copying (%d)\n", res);
    return 1;
  }
  memcpy(lm, r->data.links, lmem_req.size);

  /* unmap memory */
  VKFN(r->vkdl, vkFlushMappedMemoryRanges, r->vkd);
  if(!vkFlushMappedMemoryRanges)
    return 1;
  VKFN(r->vkdl, vkUnmapMemory, r->vkd);
  if(!vkUnmapMemory)
    return 1;

  VkMappedMemoryRange mmr[] = {
    {
      .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
      .pNext = NULL,
      .memory = r->bufs.node_mem,
      .offset = 0,
      .size = VK_WHOLE_SIZE
    },
    {
      .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
      .pNext = NULL,
      .memory = r->bufs.link_mem,
      .offset = 0,
      .size = VK_WHOLE_SIZE
    },
  };
  vkFlushMappedMemoryRanges(r->vkd, 2, mmr);
  vkUnmapMemory(r->vkd, r->bufs.node_mem);
  vkUnmapMemory(r->vkd, r->bufs.link_mem);


  return 0;
}

static int create_command_pool(struct resources *r)
{
  /* create the pool */
  VkCommandPoolCreateInfo vkpi = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .pNext = NULL,
    .flags = 
      VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT |
      VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
    .queueFamilyIndex = r->graphicsq_family_index
  };

  VKFN(r->vkdl, vkCreateCommandPool, r->vkd);
  if(!vkCreateCommandPool)
    return 1;

  int res = vkCreateCommandPool(r->vkd, &vkpi, NULL, &r->vkp);
  if(res != VK_SUCCESS) {
    fprintf(stderr, "failed to create command pool (%d)\n", res);
    return 1;
  }

  /* allocate a command buffer per image */
  VkCommandBufferAllocateInfo vkbi = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .pNext = NULL,
    .commandPool = r->vkp,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = r->nimg
  };

  VKFN(r->vkdl, vkAllocateCommandBuffers, r->vkd);
  if(!vkAllocateCommandBuffers)
    return 1;

  r->vkb = malloc(r->nimg*sizeof(VkCommandBuffer));
  res = vkAllocateCommandBuffers(r->vkd, &vkbi, r->vkb);
  if(res != VK_SUCCESS) {
    fprintf(stderr, "failed to allocate command buffer (%d)\n", res);
    return 1;
  }

  return 0;
}

static int shader_src(const char* sourcefile, uint32_t **destbuf, size_t *size)
{
  struct stat st;
  if(stat(sourcefile, &st)) {
    fprintf(stderr, "failed to stat vertex shader sipr src\n");
    return 1;
  }
  *destbuf = malloc(st.st_size);
  FILE *f = fopen(sourcefile, "rb");
  size_t n = fread(*destbuf, sizeof(char), st.st_size, f);
  if(n != (size_t)st.st_size) {
    fprintf(stderr, "failed to read vertex shader %lu != %lu", n, st.st_size);
    return 1;
  }
  *size = (size_t)st.st_size;
  return 0;
}

static int load_shaders(struct resources *r)
{

  if(shader_src("vertex.vert.sipr", &r->vert_sipr, &r->vert_sipr_sz))
    return 1;
  printf("loaded vertex.vert.sipr (%lu)\n", r->vert_sipr_sz);

  if(shader_src("fragment.frag.sipr", &r->frag_sipr, &r->frag_sipr_sz))
    return 1;
  printf("loaded fragment.frag.sipr (%lu)\n", r->frag_sipr_sz);

  VkShaderModuleCreateInfo vksi_vert = {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .codeSize = r->vert_sipr_sz,
    .pCode = r->vert_sipr
  };

  VkShaderModuleCreateInfo vksi_frag = {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .codeSize = r->frag_sipr_sz,
    .pCode = r->frag_sipr
  };

  VKFN(r->vkdl, vkCreateShaderModule, r->vkd);
  if(!vkCreateShaderModule)
    return 1;

  int res = vkCreateShaderModule(r->vkd, &vksi_vert, NULL, &r->vkvert);
  if(res != VK_SUCCESS) {
    fprintf(stderr, "failed to create vertex shader module (%d)\n", res);
    return 1;
  }

  vkCreateShaderModule(r->vkd, &vksi_frag, NULL, &r->vkfrag);
  if(res != VK_SUCCESS) {
    fprintf(stderr, "failed to create fragment shader module (%d)\n", res);
    return 1;
  }

  return 0;
}

static int init_graphics_pipeline(struct resources *r, VkPrimitiveTopology pt,
    VkPipelineLayout *layout, VkPipeline *pipeline)
{
  /* create the pipeline parameters */
  VkPipelineShaderStageCreateInfo shaders[] = {
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .stage = VK_SHADER_STAGE_VERTEX_BIT,
      .module = r->vkvert,
      .pName = "main",
      .pSpecializationInfo = NULL,
    },
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
      .module = r->vkfrag,
      .pName = "main",
      .pSpecializationInfo = NULL,
    }
  };

  VkVertexInputBindingDescription vkib = {
    .binding = 0,
    .stride = 2*sizeof(float),
    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
  };

  VkVertexInputAttributeDescription vkad = {
    .location = 0,
    .binding = vkib.binding,
    .format = VK_FORMAT_R32G32_SFLOAT,
    .offset = 0
  };

  VkPipelineVertexInputStateCreateInfo vci = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .vertexBindingDescriptionCount = 1,
    .pVertexBindingDescriptions = &vkib,
    .vertexAttributeDescriptionCount = 1,
    .pVertexAttributeDescriptions = &vkad
  };

  VkPipelineInputAssemblyStateCreateInfo vkpac = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .topology = pt,
    .primitiveRestartEnable = VK_FALSE,
  };

  VkViewport vp = {
    .x = 0.0f,
    .y = 0.0f,
    .width = r->surface_area.width,
    .height = r->surface_area.height,
    .minDepth = 0.0f,
    .maxDepth = 1.0f
  };
  r->nviewports = 1;
  r->viewports = malloc(sizeof(vp));
  *r->viewports = vp;

  VkRect2D scissor = {
    .offset = {
      .x = 0.0f,
      .y = 0.0f,
    },
    .extent = r->surface_area
  };
  r->nscissors = 1;
  r->scissors = malloc(sizeof(scissor));
  *r->scissors = scissor;

  VkPipelineViewportStateCreateInfo vkvcr = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .viewportCount = r->nviewports,
    .pViewports = r->viewports,
    .scissorCount = r->nscissors,
    .pScissors = r->scissors
  };

  VkPipelineRasterizationStateCreateInfo vkri = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .depthClampEnable = VK_FALSE,
    .rasterizerDiscardEnable = VK_FALSE,
    .polygonMode = VK_POLYGON_MODE_FILL,
    .cullMode = VK_CULL_MODE_BACK_BIT,
    .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
    .depthBiasEnable = VK_FALSE,
    .depthBiasConstantFactor = 0.0,
    .depthBiasClamp = 0.0,
    .depthBiasSlopeFactor = 0.0,
    .lineWidth = 4.0
  };

  VkPipelineMultisampleStateCreateInfo msci = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    .sampleShadingEnable = VK_FALSE,
    .minSampleShading = 1.0f,
    .pSampleMask = NULL,
    .alphaToCoverageEnable = VK_FALSE,
    .alphaToOneEnable = VK_FALSE
  };

  VkPipelineColorBlendAttachmentState cbas = {
    .blendEnable = VK_FALSE,
    .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
    .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
    .colorBlendOp = VK_BLEND_OP_ADD,
    .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
    .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
    .alphaBlendOp = VK_BLEND_OP_ADD,
    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                      VK_COLOR_COMPONENT_G_BIT |
                      VK_COLOR_COMPONENT_B_BIT |
                      VK_COLOR_COMPONENT_A_BIT
  };

  VkPipelineColorBlendStateCreateInfo bsci = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .logicOpEnable = VK_FALSE,
    .logicOp = VK_LOGIC_OP_COPY,
    .attachmentCount = 1,
    .pAttachments = &cbas,
    .blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f }
  };

  /* create the pipeline */

  VkPipelineLayoutCreateInfo plci = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .setLayoutCount = 0,
    .pSetLayouts = NULL,
    .pushConstantRangeCount = 0,
    .pPushConstantRanges = NULL
  };
  VKFN(r->vkdl, vkCreatePipelineLayout, r->vkd);
  if(!vkCreatePipelineLayout)
    return 1;
  int res = vkCreatePipelineLayout(r->vkd, &plci, NULL, layout);
  if(res != VK_SUCCESS) {
    fprintf(stderr, "failed to create pipeline layout (%d)\n", res);
    return 1;
  }

  VkGraphicsPipelineCreateInfo pci = {
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .stageCount = 2, //vertex & fragment
    .pStages = shaders,
    .pVertexInputState = &vci,
    .pInputAssemblyState = &vkpac,
    .pTessellationState = NULL,
    .pViewportState = &vkvcr,
    .pRasterizationState = &vkri,
    .pMultisampleState = &msci,
    .pDepthStencilState = NULL,
    .pColorBlendState = &bsci,
    .pDynamicState = NULL,
    .layout = *layout,
    .renderPass = r->render_pass,
    .subpass = 0,
    .basePipelineHandle = VK_NULL_HANDLE,
    .basePipelineIndex = -1
  };

  VKFN(r->vkdl, vkCreateGraphicsPipelines, r->vkd);
  if(!vkCreateGraphicsPipelines)
    return 1;
  
  res = vkCreateGraphicsPipelines(
      r->vkd, 
      VK_NULL_HANDLE,
      1,              /* # of piplines to create */
      &pci,
      NULL,
      pipeline
  );
  if(res != VK_SUCCESS) {
    fprintf(stderr, "failed to create graphics pipeline\n");
    return 1;
  }

  return 0;
}

static uint32_t decide_num_img(VkSurfaceCapabilitiesKHR *sc)
{
  uint32_t ret;
  /* maxImageCount = 0 indicates no limit on # of images according to vulkan 
   * spec */
  if(sc->maxImageCount == 0 || sc->maxImageCount >= 2)
    ret = 2;
  else
    ret = sc->maxImageCount;

  printf("swapchain: # images %u\n", ret);
  return ret;
}

static VkSurfaceFormatKHR decide_format(VkSurfaceFormatKHR *formats, uint32_t count)
{
  printf("supported image formats:\n");
  for(uint32_t i=0; i<count; i++)
  {
    printf("%d ", formats[i].format);
    if(formats[i].format == VK_FORMAT_B8G8R8A8_SRGB) {
      printf("\n");
      return formats[i];
    }
  }
  printf("\n");

  fprintf(stderr, 
      "warning: VK_FORMAT_R8G8B8A8 image format not found "
      "defualting to first supported format\n"
  );
  return formats[0];
}

static VkPresentModeKHR decide_present_mode(VkPresentModeKHR *modes, uint32_t count)
{
  VkBool32 immediate = VK_FALSE;

  for(uint32_t i=0; i<count; i++)
  {
    if(modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
      return modes[i];
    else if (modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
      immediate = VK_TRUE;
  }

  if(immediate)
    return VK_PRESENT_MODE_IMMEDIATE_KHR;
  
  return  VK_PRESENT_MODE_FIFO_KHR;
}

static int create_swapchain(struct resources *r)
{
  /* query surface capabilities */
  VKFN(r->vkl, vkGetPhysicalDeviceSurfaceCapabilitiesKHR, r->vki);
  if(!vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
    return 1;

  VkSurfaceCapabilitiesKHR sc;
  int res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(r->vkpd, r->surface, &sc);
  if(res != VK_SUCCESS) {
    fprintf(stderr, "failed to get surface capabilities (%d)\n", res);
    return 1;
  }

  /* query surface formats */
  VKFN(r->vkl, vkGetPhysicalDeviceSurfaceFormatsKHR, r->vki);
  if(!vkGetPhysicalDeviceSurfaceFormatsKHR)
    return 1;

  uint32_t nformat;
  res = vkGetPhysicalDeviceSurfaceFormatsKHR(
      r->vkpd, r->surface, &nformat, NULL);
  if(res != VK_SUCCESS) {
    fprintf(stderr, "failed to count surface formats (%d)\n", res);
    return 1;
  }
  if(nformat == 0) {
    fprintf(stderr, "no supported image formats found\n");
    return 1;
  }
  VkSurfaceFormatKHR *formats = malloc(nformat*sizeof(VkSurfaceFormatKHR));
  res = vkGetPhysicalDeviceSurfaceFormatsKHR(
      r->vkpd, r->surface, &nformat, formats);
  if(res != VK_SUCCESS) {
    fprintf(stderr, "failed to fetch surface formats (%d)\n", res);
    free(formats);
    return 1;
  }

  /* query present modes */
  VKFN(r->vkl, vkGetPhysicalDeviceSurfacePresentModesKHR, r->vki);
  if(!vkGetPhysicalDeviceSurfacePresentModesKHR) {
    free(formats);
    return 1;
  }

  uint32_t nmode;
  res = vkGetPhysicalDeviceSurfacePresentModesKHR(
      r->vkpd, r->surface, &nmode, NULL);
  if(res != VK_SUCCESS) {
    fprintf(stderr, "failed to count surface presentation modes (%d)\n", res);
    free(formats);
    return 1;
  }
  VkPresentModeKHR *modes = malloc(nmode*sizeof(VkPresentModeKHR));
  res = vkGetPhysicalDeviceSurfacePresentModesKHR(
      r->vkpd, r->surface, &nmode, modes);
  if(res != VK_SUCCESS) {
    fprintf(stderr, "failed to count surface presentation modes (%d)\n", res);
    free(formats);
    free(modes);
    return 1;
  }

  /* prepare the swapchain creation parameters */

  VkSurfaceFormatKHR format = decide_format(formats, nformat);
  VkSwapchainCreateInfoKHR sci = {
    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .pNext = NULL,
    .flags = 0,
    .surface = r->surface,
    .minImageCount =  decide_num_img(&sc),
    .imageFormat = format.format,
    .imageColorSpace = format.colorSpace,
    .imageExtent = sc.currentExtent,
    .imageArrayLayers = 1,
    .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .queueFamilyIndexCount = 0,
    .pQueueFamilyIndices = NULL,
    .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR ,
    .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .presentMode = decide_present_mode(modes, nmode),
    .clipped = VK_TRUE,
    .oldSwapchain = r->old_swapchain
  };
  r->extent = sc.currentExtent;
  r->format = format.format;

  free(modes);
  free(formats);

  /* create the new swapchain */
  VKFN(r->vkdl, vkCreateSwapchainKHR, r->vkd);
  if(!vkCreateSwapchainKHR)
    return 1;
  res = vkCreateSwapchainKHR(r->vkd, &sci, NULL, &r->swapchain);
  if(res != VK_SUCCESS) {
    fprintf(stderr, "failed to create swapchain (%d)\n", res);
    return 1;
  }

  /* destroy the old swapchain */
  VKFN(r->vkdl, vkDestroySwapchainKHR, r->vkd);
  if(!vkDestroySwapchainKHR)
    return 1;
  if(r->old_swapchain != VK_NULL_HANDLE)
    vkDestroySwapchainKHR(r->vkd, r->old_swapchain, NULL);

  /* initialize the images */
  VKFN(r->vkdl, vkGetSwapchainImagesKHR, r->vkd);
  if(!vkGetSwapchainImagesKHR)
    return 1;
  uint32_t nimg;
  res = vkGetSwapchainImagesKHR(r->vkd, r->swapchain, &nimg, NULL);
  if(res != VK_SUCCESS) {
    fprintf(stderr, "failed to count swapchain images (%d)\n", res);
    return 1;
  }
  VkImage *images = malloc(nimg*sizeof(VkImage));
  r->nimg = nimg;
  r->images = malloc(nimg*sizeof(struct image_resources));
  res = vkGetSwapchainImagesKHR(r->vkd, r->swapchain, &nimg, images);
  if(res != VK_SUCCESS) {
    fprintf(stderr, "failed to fetch swapchain images (%d)\n", res);
    free(images);
    return 1;
  }
  for(uint32_t i=0; i<nimg; i++) {
    struct image_resources *img = &r->images[i];
    img->handle = images[i];
    img->view = VK_NULL_HANDLE;
    img->sampler = VK_NULL_HANDLE;
    img->memory = VK_NULL_HANDLE;
  }
  free(images);

  /* create image views */
  VKFN(r->vkdl, vkCreateImageView, r->vkd);
  if(!vkCreateImageView)
    return 1;
  for(uint32_t i=0; i<nimg; i++) {
    VkImageViewCreateInfo vki = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .image = r->images[i].handle,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = format.format,
      .components = {
        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .a = VK_COMPONENT_SWIZZLE_IDENTITY
      },
      .subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1
      }
    };
    res = vkCreateImageView(r->vkd, &vki, NULL, &r->images[i].view);
    if(res != VK_SUCCESS) {
      fprintf(stderr, 
          "failed to create image view for image %d (%d)\n", i, res);
      return 1;
    }
  }

  return 0;
}

static int create_render_pass(struct resources *r)
{
  VkAttachmentDescription ad[] = {
    {
      .flags = 0,
      .format = r->format,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    }
  };

  VkAttachmentReference ar[] = {
    {
      .attachment = 0,
      .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    }
  };

  VkSubpassDescription sd[] = {
    {
      .flags = 0,
      .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .inputAttachmentCount = 0,
      .pInputAttachments = NULL,
      .colorAttachmentCount = 1,
      .pColorAttachments = ar,
      .pResolveAttachments = NULL,
      .pDepthStencilAttachment = NULL,
      .preserveAttachmentCount = 0,
      .pPreserveAttachments = NULL,
    }
  };

  VkRenderPassCreateInfo rpci = {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .attachmentCount = 1,
    .pAttachments = ad,
    .subpassCount = 1,
    .pSubpasses = sd,
    .dependencyCount = 0,
    .pDependencies = NULL
  };

  VKFN(r->vkdl, vkCreateRenderPass, r->vkd);
  if(!vkCreateRenderPass)
    return 1;
  int res = vkCreateRenderPass(r->vkd, &rpci, NULL, &r->render_pass);
  if(res != VK_SUCCESS) {
    fprintf(stderr, "failed to create render pass (%d)\n", res);
    return 1;
  }

  return 0;
}

static int create_framebuffers(struct resources *r)
{
  VKFN(r->vkdl, vkCreateFramebuffer, r->vkd);
  if(!vkCreateFramebuffer)
    return 1;

  r->framebuffers = malloc(r->nimg*sizeof(VkFramebuffer));
  for(uint32_t i=0; i<r->nimg; i++) {
    VkFramebufferCreateInfo fci = {
      .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .renderPass = r->render_pass,
      .attachmentCount = 1,
      .pAttachments = &r->images[i].view,
      .width = r->surface_area.width,
      .height = r->surface_area.height,
      .layers = 1
    };

    int res = vkCreateFramebuffer(r->vkd, &fci, NULL, &r->framebuffers[i]);
    if(res != VK_SUCCESS) {
      fprintf(stderr, 
          "failed to create framebuffer for image %d (%d)", i, res);
      return 1;
    }

  }

  return 0;
}

static int create_semaphores(struct resources *r)
{
  VKFN(r->vkdl, vkCreateSemaphore, r->vkd);
  if(!vkCreateSemaphore)
    return 1;

  VkSemaphoreCreateInfo sci = {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0
  };
  int res = vkCreateSemaphore(r->vkd, &sci, NULL, &r->image_ready);
  if(res != VK_SUCCESS) {
    fprintf(stderr, "failed to create image-ready semaphore (%d)\n", res);
    return 1;
  }
  res = vkCreateSemaphore(r->vkd, &sci, NULL, &r->rendering_finished);
  if(res != VK_SUCCESS) {
    fprintf(stderr, 
        "failed to create rendering-finished semaphore (%d)\n", res);
    return 1;
  }

  return 0;
}

static int record_command_buffers(struct resources *r)
{
  VkCommandBufferBeginInfo bi = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .pNext = NULL,
    .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
    .pInheritanceInfo = NULL
  };

  VKFN(r->vkdl, vkBeginCommandBuffer, r->vkd);
  if(!vkBeginCommandBuffer)
    return 1;

  VKFN(r->vkdl, vkCmdBeginRenderPass, r->vkd);
  if(!vkCmdBeginRenderPass)
    return 1;

  VKFN(r->vkdl, vkCmdBindPipeline, r->vkd);
  if(!vkCmdBindPipeline)
    return 1;

  VKFN(r->vkdl, vkCmdDraw, r->vkd);
  if(!vkCmdDraw)
    return 1;

  VKFN(r->vkdl, vkCmdDrawIndexed, r->vkd);
  if(!vkCmdDrawIndexed)
    return 1;

  VKFN(r->vkdl, vkCmdEndRenderPass, r->vkd);
  if(!vkCmdEndRenderPass)
    return 1;

  VKFN(r->vkdl, vkEndCommandBuffer, r->vkd);
  if(!vkEndCommandBuffer)
    return 1;

  VKFN(r->vkdl, vkCmdBindVertexBuffers, r->vkd);
  if(!vkCmdBindVertexBuffers)
    return 1;

  VKFN(r->vkdl, vkCmdBindIndexBuffer, r->vkd);
  if(!vkCmdBindIndexBuffer)
    return 1;

  for(uint32_t i=0; i<r->nimg; i++) {
    vkBeginCommandBuffer(r->vkb[i], &bi);

    VkRenderPassBeginInfo rpbi = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .pNext = NULL,
      .renderPass = r->render_pass,
      .framebuffer = r->framebuffers[i],
      .renderArea = {
        .offset = { .x = 0.0f, .y = 0.0f },
        .extent = r->surface_area
      },
      .clearValueCount = 1,
      .pClearValues = &r->clear
    };
    vkCmdBeginRenderPass(r->vkb[i], &rpbi, VK_SUBPASS_CONTENTS_INLINE);

    /* node rendering */
    vkCmdBindPipeline(r->vkb[i], VK_PIPELINE_BIND_POINT_GRAPHICS, r->node_pipeline);
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(r->vkb[i], 0, 1, &r->bufs.node_buffer, &offset);
    vkCmdDraw(r->vkb[i],
        r->data.n, /* vertex count */
        r->data.n, /* instance count */
        0,         /* first vertex */
        0          /* first instance */
    );

    /* link rendering */
    vkCmdBindPipeline(r->vkb[i], VK_PIPELINE_BIND_POINT_GRAPHICS, r->link_pipeline);
    vkCmdBindIndexBuffer(r->vkb[i], r->bufs.link_buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(r->vkb[i], 
        r->data.l*2, /* vertex count (nodes) (2 per line) */
        r->data.l, /* instance count (lines) */
        0,         /* first index */
        0,         /* vertex offset */
        0          /* first instance */
    );


    vkCmdEndRenderPass(r->vkb[i]);
    int res = vkEndCommandBuffer(r->vkb[i]);
    if(res != VK_SUCCESS) {
      fprintf(stderr, "failed to end cmd buffer @ index %d (%d)\n", i, res);
      return 1;
    }
  }

  return 0;
}

static void draw(struct resources *r)
{
  /* get a handle for the next image to present */
  VKFN(r->vkdl, vkAcquireNextImageKHR, r->vkd);
  if(!vkAcquireNextImageKHR)
    return;

  uint32_t img_idx;
  VkResult result = vkAcquireNextImageKHR(
      r->vkd, r->swapchain, UINT64_MAX, r->image_ready, VK_NULL_HANDLE, &img_idx);

  /* determine what to do */
  switch(result) {
    case VK_SUCCESS:
    case VK_SUBOPTIMAL_KHR:
      break;
    case VK_ERROR_OUT_OF_DATE_KHR:
      fprintf(stderr, "minor shitmuffin\n");
      return;
    default:
      fprintf(stderr, "major shitmuffin\n");
      return;
  }

  /* time to submit command buffer to graphics/present queue */
  VKFN(r->vkdl, vkQueueSubmit, r->vkd);
  if(!vkQueueSubmit)
    return;

  VkPipelineStageFlags psf = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  VkSubmitInfo si = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .pNext = NULL,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = &r->image_ready,
    .pWaitDstStageMask = &psf,
    .commandBufferCount = 1,
    .pCommandBuffers = &r->vkb[img_idx],
    .signalSemaphoreCount = 1,
    .pSignalSemaphores = &r->rendering_finished
  };

  VkResult res = vkQueueSubmit(
      r->graphicsq,
      1,              /* number of items to submit */
      &si,            /* the item to submit */
      VK_NULL_HANDLE  /* no memory fence required */
  );
  if(res != VK_SUCCESS) {
    fprintf(stderr, "failed to submit command buffer[%u] to queue %d", img_idx,
        res);
    return;
  }

  /* queue an image for presentation */
  VKFN(r->vkdl, vkQueuePresentKHR, r->vkd);
  if(!vkQueuePresentKHR)
    return;

  VkPresentInfoKHR pi = {
    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .pNext = NULL,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = &r->rendering_finished,
    .swapchainCount = 1,
    .pSwapchains = &r->swapchain,
    .pImageIndices = &img_idx,
    .pResults = NULL
  };

  res = vkQueuePresentKHR(r->presentq, &pi);
  switch(res) {
    case VK_SUCCESS:
      break;
    case VK_ERROR_OUT_OF_DATE_KHR:
    case VK_SUBOPTIMAL_KHR:
      fprintf(stderr, "minor post shitmuffin\n");
      return;
    default:
      fprintf(stderr, "major post shitmuffin\n");
      return;
  }


}

static int init_glfw(struct resources *r)
{
  if(!glfwInit()) {
    fprintf(stderr, "glfw init failed\n");
  }
  glfwSetErrorCallback(glfw_error_callback);

  /* make this system's install glfw supports vulkan */
  if(glfwVulkanSupported() != GLFW_TRUE) {
    fprintf(stderr, "glfw does not support vulkan\n");
    return 1;
  }

  /* get the extensions vulkan requires
     TODO merge these with the @vkx array */
  uint32_t n;
  const char** exts = glfwGetRequiredInstanceExtensions(&n);
  if(!exts) {
    fprintf(stderr, "could not get glfw required instance extensions\n");
    return 1;
  }
  printf("glfw required vulkan extensions\n");
  for(uint32_t i=0; i<n; i++) {
    printf("  %s\n", exts[i]);
  }

  /* make sure glfw is ok with the chosen queue from init_vulkan */
  if(glfwGetPhysicalDevicePresentationSupport(
        r->vki, r->vkpd, r->graphicsq_family_index) != GLFW_TRUE) {
    fprintf(stderr, "glfw cannot work with the chose queue\n");
    return 1;
  }

  /* create the glfw window */
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  r->win = glfwCreateWindow(r->surface_area.width, r->surface_area.height,
      "toucan", NULL, NULL);
  if(!r->win) {
    fprintf(stderr, "failed to create glfw window\n");
  }

  /* create vulkan surface attached to glfw window */
  glfwCreateWindowSurface(r->vki, r->win, NULL, &r->surface);

  return 0;
}


static void glfw_error_callback(int error, const char* description)
{
  fprintf(stderr, "gflw[e]: %s (%d)\n", description, error);
}
