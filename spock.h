#pragma once

#include "toucan.h"

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#define UNUSED(x) (void)(x)

#define VKFN(loader, name, instance) \
  PFN_##name name = (PFN_##name) loader(instance, #name); \
  if(!name) { \
    fprintf(stderr, "failed to load " #name "\n"); \
  }

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


struct vulkanrt {
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

  float *world;
  float x, y, zoom;
  float pan_delta, zoom_delta;

  /* glfw */
  GLFWwindow *win;

  /* data */
  struct network_resources bufs;
  uint32_t memory_type_index;
};

static inline struct vulkanrt new_vulkanrt() {

  struct vulkanrt r = {
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
    .x = 1.0,
    .y = 1.0,
    .zoom = 1.0,
    .pan_delta = 5,
    .zoom_delta = 0.03,
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

  return r;
}

int init_glfw(struct vulkanrt*);
void glfw_key_callback(GLFWwindow *w, int key, int scancode, int action, int mods);
void glfw_error_callback(int error, const char* description);
int init_vulkan(struct vulkanrt*);
int configure_vulkan(struct vulkanrt*, const struct network*);
int free_vulkanrt(struct vulkanrt*);
struct network init_data();
int net_bufs(struct vulkanrt*, const struct network*);
int bind_bufs(struct vulkanrt*, const struct network*);
int create_command_pool(struct vulkanrt*);
int load_shaders(struct vulkanrt*);
int init_graphics_pipeline(struct vulkanrt*, VkPrimitiveTopology pt,
    VkPipelineLayout *layout, VkPipeline *pipeline);
int create_swapchain(struct vulkanrt*);
int create_render_pass(struct vulkanrt*);
int create_framebuffers(struct vulkanrt*);
int create_semaphores(struct vulkanrt*);
int record_command_buffers(struct vulkanrt*, const struct network*);
int get_queue_info(struct vulkanrt*);
int world_matrix(struct vulkanrt*);
void update_world(struct vulkanrt*);
void draw(struct vulkanrt*);

