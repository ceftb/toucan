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

struct resources {
  /* vulkan */
  void *vk;
  VkInstance vki;
  VkPhysicalDevice vkpd;
  VkDevice vkd;
  VkQueue vkq;
  PFN_vkGetInstanceProcAddr vkl;
  PFN_vkGetDeviceProcAddr vkdl;
  uint32_t q_family_index, qindex;
  VkSurfaceKHR surface;
  VkCommandPool vkp;
  uint32_t vkb_size;
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
static int init_graphics_pipeline(struct resources*);

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
    .vkq = NULL,
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
    .q_family_index = 0,
    .qindex = 0,
    .vkb_size = 10,
    .vert_sipr_sz = 0,
    .frag_sipr_sz = 0,
    .nvib = 0,
    .nvattr = 0,
    .nviewports = 1,
    .nscissors = 1,

    /* glfw */
    .win = 0,

    .bufs = {
      .node_buffer = NULL,
      .link_buffer = NULL,
      .node_mem = VK_NULL_HANDLE,
      .link_mem = VK_NULL_HANDLE
    }
  };

  if(alloc_resources(&r))
    die(&r);

  r.data = init_data();

  if(net_bufs(&r, r.data)) 
    die(&r);

  if(net_bufs(&r, r.data))
    die(&r);

  if(bind_bufs(&r))
    die(&r);

  if(create_command_pool(&r))

  if(load_shaders(&r))
    die(&r);

  if(init_graphics_pipeline(&r))
    die(&r);

  while(!glfwWindowShouldClose(r.win)) {
    // the toucan is flying
    glfwPollEvents();
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

  /* destroy the command buffer */
  printf("destroying command buffer\n");
  VKFN(r->vkdl, vkFreeCommandBuffers, r->vkd);
  if(vkFreeCommandBuffers)
    vkFreeCommandBuffers(r->vkd, r->vkp, r->vkb_size, r->vkb);
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

  return ret;
}

static int vk_get_queue_info(struct resources *r)
{
  VKFN(r->vkl, vkGetPhysicalDeviceQueueFamilyProperties, r->vki);
  if(!vkGetPhysicalDeviceQueueFamilyProperties)
    return 1;

  uint32_t n;
  vkGetPhysicalDeviceQueueFamilyProperties(r->vkpd, &n, NULL);
  VkQueueFamilyProperties *qps = malloc(n*sizeof(VkQueueFamilyProperties));
  vkGetPhysicalDeviceQueueFamilyProperties(r->vkpd, &n, qps);
  for(uint32_t i=0; i<n; i++){
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

    if(qps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && 
       qps[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
      /* mark the queue family as selected */
      r->q_family_index = i;
      /* grab the first queue in the family */
      r->qindex = qps[i].queueCount - 1;
    }
  }

  free(qps);

  printf("selected queue-%u\n", r->q_family_index);

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
      .flags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT,
      .queueFamilyIndex = r->q_family_index,
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
  /* XXX it's quite worrysome that this returns void and not an error indicator */
  vkGetDeviceQueue(r->vkd, r->q_family_index, r->qindex, &r->vkq);

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

  err = vk_get_queue_info(r);
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
 *  0.          .0
 *    `.      .'
 *      0----0     
 *    .'      '.
 *  0'          `0
 *
 */
static struct network init_data()
{
  struct network net = {
    .n = 6,
    .l = 5,
    .nodes = malloc(net.n*sizeof(struct point2)),
    .links = malloc(net.l*sizeof(uint32_t))
  };

  struct point2 nodes[] = {
    {-10.0,  10.0},
    {-10.0, -10.0},
    { -5.0,   0.0},
    {  5.0,   0.0},
    { 10.0,  10.0},
    { 10.0, -10.0}
  };
  memcpy(net.nodes, nodes, net.n*sizeof(struct point2));

  uint32_t links[] = {
    0, 2,
    1, 2,
    2, 3,
    3, 4,
    3, 5
  };
  memcpy(net.links, links, net.l*sizeof(uint32_t));

  return net;
}

static int net_bufs(struct resources *r, struct network net)
{
  VkBufferCreateInfo nbi = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .size = net.n,
    .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .queueFamilyIndexCount = 0,
    .pQueueFamilyIndices = NULL
  };

  VkBufferCreateInfo lbi = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .size = net.l,
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

    /* chosing memory that exclusively local device memory */
    if(t->propertyFlags == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
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
    fprintf(stderr, "device has no strictly local memory - cannot continue\n");
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

  return 0;
}

static int create_command_pool(struct resources *r)
{
  /* create the pool */
  VkCommandPoolCreateInfo vkpi = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .pNext = NULL,
    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    .queueFamilyIndex = r->q_family_index
  };

  VKFN(r->vkdl, vkCreateCommandPool, r->vkd);
  if(!vkCreateCommandPool)
    return 1;

  int res = vkCreateCommandPool(r->vkd, &vkpi, NULL, &r->vkp);
  if(res != VK_SUCCESS) {
    fprintf(stderr, "failed to create command pool (%d)\n", res);
    return 1;
  }

  /* allocate a single buffer */
  VkCommandBufferAllocateInfo vkbi = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .pNext = NULL,
    .commandPool = r->vkp,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = r->vkb_size
  };

  VKFN(r->vkdl, vkAllocateCommandBuffers, r->vkd);
  if(!vkAllocateCommandBuffers)
    return 1;

  r->vkb = malloc(r->vkb_size*sizeof(VkCommandBuffer));
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
    fprintf(stderr, "failed to stat vertex shader sipr src");
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

  if(shader_src("shaders/vertex.vert.sipr", &r->vert_sipr, &r->vert_sipr_sz))
    return 1;
  if(shader_src("shaders/fragment.frag.sipr", &r->frag_sipr, &r->frag_sipr_sz))
    return 1;

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

static int init_graphics_pipeline(struct resources *r)
{
  VkPipelineShaderStageCreateInfo vkpsi_vert = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .stage = VK_SHADER_STAGE_VERTEX_BIT,
    .module = r->vkvert,
    .pName = "main",
    .pSpecializationInfo = NULL,
  };

  VkVertexInputBindingDescription vkib = {
    .binding = 0,
    .stride = 2*sizeof(float),
    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
  };

  VkVertexInputAttributeDescription vkad = {
    .location = 0,
    .binding = 0,
    .format = VK_FORMAT_R32G32_SFLOAT,
    .offset = 0
  };

  VkPipelineVertexInputStateCreateInfo vkpi = {
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
    .topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
    .primitiveRestartEnable = VK_FALSE,
  };

  VkViewport vp = {
    .x = -20,
    .y = 20,
    .width = 40,
    .height = 40,
    .minDepth = 0.0,
    .maxDepth = 1.0
  };
  r->viewports = malloc(sizeof(vp));
  *r->viewports = vp;

  VkRect2D scissor = {
    .offset = {
      .x = -20,
      .y = 20,
    },
    .extent = {
      .width = 40,
      .height = 40,
    }
  };
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
    .cullMode = VK_CULL_MODE_NONE,
    .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
    .depthBiasEnable = VK_FALSE,
    .depthBiasConstantFactor = 0.0,
    .depthBiasClamp = 0.0,
    .depthBiasSlopeFactor = 0.0,
    .lineWidth = 1.0
  };

  return 0;
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
        r->vki, r->vkpd, r->q_family_index) != GLFW_TRUE) {
    fprintf(stderr, "glfw cannot work with the chose queue\n");
    return 1;
  }

  /* create the glfw window */
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  r->win = glfwCreateWindow(1000, 700, "toucan", NULL, NULL);
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
