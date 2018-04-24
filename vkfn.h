#include "spock.h"

#define UNUSED(x) (void)(x)

#define DECL_VKFN(name) PFN_##name name = NULL

#define DEF_VKFN(loader, name, instance)                 \
  name = (PFN_##name) loader(instance, #name);           \
  if(!name) {                                            \
    fprintf(stderr, "vkfn: failed to load " #name "\n"); \
    return 1;                                            \
  }

DECL_VKFN(vkWaitForFences);
DECL_VKFN(vkAcquireNextImageKHR);
DECL_VKFN(vkQueueSubmit);
DECL_VKFN(vkResetFences);
DECL_VKFN(vkQueuePresentKHR);
DECL_VKFN(vkCmdPushConstants);
DECL_VKFN(vkAllocateMemory);
DECL_VKFN(vkGetBufferMemoryRequirements);
DECL_VKFN(vkMapMemory);
DECL_VKFN(vkFlushMappedMemoryRanges);
DECL_VKFN(vkUnmapMemory);
DECL_VKFN(vkBeginCommandBuffer);
DECL_VKFN(vkCmdCopyBuffer);
DECL_VKFN(vkCmdPipelineBarrier);
DECL_VKFN(vkEndCommandBuffer);
DECL_VKFN(vkDeviceWaitIdle);
DECL_VKFN(vkCreateCommandPool);
//DECL_VKFN(vkAllocateCommandBuffers);
DECL_VKFN(vkCreateShaderModule);
DECL_VKFN(vkCreatePipelineLayout);
DECL_VKFN(vkCreateGraphicsPipelines);
DECL_VKFN(vkCreateSwapchainKHR);
DECL_VKFN(vkDestroySwapchainKHR);
DECL_VKFN(vkGetSwapchainImagesKHR);
DECL_VKFN(vkCreateImageView);
DECL_VKFN(vkCreateRenderPass);
DECL_VKFN(vkCreateFramebuffer);
DECL_VKFN(vkCreateSemaphore);
DECL_VKFN(vkCreateFence);
DECL_VKFN(vkCmdBeginRenderPass);
DECL_VKFN(vkCmdBindPipeline);
DECL_VKFN(vkCmdDraw);
DECL_VKFN(vkCmdDrawIndexed);
DECL_VKFN(vkCmdEndRenderPass);
DECL_VKFN(vkCmdBindVertexBuffers);
DECL_VKFN(vkCmdBindIndexBuffer);

static inline int vkfn_device_init(struct vulkanrt *r) {

  DEF_VKFN(r->vkdl, vkWaitForFences, r->vkd);
  DEF_VKFN(r->vkdl, vkAcquireNextImageKHR, r->vkd);
  DEF_VKFN(r->vkdl, vkQueueSubmit, r->vkd);
  DEF_VKFN(r->vkdl, vkResetFences, r->vkd);
  DEF_VKFN(r->vkdl, vkQueuePresentKHR, r->vkd);
  DEF_VKFN(r->vkdl, vkCmdPushConstants, r->vkd);
  DEF_VKFN(r->vkdl, vkAllocateMemory, r->vkd);
  DEF_VKFN(r->vkdl, vkGetBufferMemoryRequirements, r->vkd);
  DEF_VKFN(r->vkdl, vkMapMemory, r->vkd);
  DEF_VKFN(r->vkdl, vkFlushMappedMemoryRanges, r->vkd);
  DEF_VKFN(r->vkdl, vkUnmapMemory, r->vkd);
  DEF_VKFN(r->vkdl, vkCmdCopyBuffer, r->vkd);
  DEF_VKFN(r->vkdl, vkCmdPipelineBarrier, r->vkd);
  DEF_VKFN(r->vkdl, vkBeginCommandBuffer, r->vkd);
  DEF_VKFN(r->vkdl, vkEndCommandBuffer, r->vkd);
  DEF_VKFN(r->vkdl, vkDeviceWaitIdle, r->vkd);
  DEF_VKFN(r->vkdl, vkCreateCommandPool, r->vkd);
  //DEF_VKFN(r->vkdl, vkAllocateCommandBuffers, r->vkd);
  DEF_VKFN(r->vkdl, vkCreateShaderModule, r->vkd);
  DEF_VKFN(r->vkdl, vkCreatePipelineLayout, r->vkd);
  DEF_VKFN(r->vkdl, vkCreateGraphicsPipelines, r->vkd);
  DEF_VKFN(r->vkdl, vkCreateSwapchainKHR, r->vkd);
  DEF_VKFN(r->vkdl, vkDestroySwapchainKHR, r->vkd);
  DEF_VKFN(r->vkdl, vkGetSwapchainImagesKHR, r->vkd);
  DEF_VKFN(r->vkdl, vkCreateImageView, r->vkd);
  DEF_VKFN(r->vkdl, vkCreateRenderPass, r->vkd);
  DEF_VKFN(r->vkdl, vkCreateFramebuffer, r->vkd);
  DEF_VKFN(r->vkdl, vkCreateSemaphore, r->vkd);
  DEF_VKFN(r->vkdl, vkCreateFence, r->vkd);
  DEF_VKFN(r->vkdl, vkCmdBeginRenderPass, r->vkd);
  DEF_VKFN(r->vkdl, vkCmdBindPipeline, r->vkd);
  DEF_VKFN(r->vkdl, vkCmdDraw, r->vkd);
  DEF_VKFN(r->vkdl, vkCmdDrawIndexed, r->vkd);
  DEF_VKFN(r->vkdl, vkCmdEndRenderPass, r->vkd);
  DEF_VKFN(r->vkdl, vkCmdBindVertexBuffers, r->vkd);
  DEF_VKFN(r->vkdl, vkCmdBindIndexBuffer, r->vkd);

  return 0;
}

//DECL_VKFN(vkGetPhysicalDeviceMemoryProperties);
//DECL_VKFN(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);

static inline int vkfn_instance_init(struct vulkanrt *r) {
  UNUSED(r);

  //DEF_VKFN(r->vkl, vkGetPhysicalDeviceMemoryProperties, r->vki);
  //DEF_VKFN(r->vkl, vkGetPhysicalDeviceSurfaceCapabilitiesKHR, r->vki);

  return 0;
}
