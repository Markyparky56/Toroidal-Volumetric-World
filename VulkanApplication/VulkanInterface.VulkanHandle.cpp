#include "VulkanHandle.hpp"

namespace VulkanInterface
{
#define VulkanHandleSpecialisation( type, deleter ) \
  template<> \
  void VulkanHandle<type>::Destroy() { \
    if(deleter != nullptr) { \
      deleter(Device, Object, nullptr); \
    } \
  }

  void VulkanHandle<VkInstance>::Destroy() 
  {
    if (vkDestroyInstance != nullptr)
    {
      vkDestroyInstance(Instance, nullptr);
    }
  }

  void VulkanHandle<VkDevice>::Destroy()
  {
    if (vkDestroyDevice != nullptr)
    {
      vkDestroyDevice(LogicalDevice, nullptr);
    }
  }

  void VulkanHandle<VkSurfaceKHR>::Destroy()
  {
    if (vkDestroySurfaceKHR != nullptr)
    {
      vkDestroySurfaceKHR(Instance, Object, nullptr);
    }
  }

  VulkanHandleSpecialisation(VkSemaphore, vkDestroySemaphore)
  VulkanHandleSpecialisation(VkFence, vkDestroyFence)
  VulkanHandleSpecialisation(VkDeviceMemory, vkFreeMemory)
  VulkanHandleSpecialisation(VkBuffer, vkDestroyBuffer)
  VulkanHandleSpecialisation(VkImage, vkDestroyImage)
  VulkanHandleSpecialisation(VkEvent, vkDestroyEvent)
  VulkanHandleSpecialisation(VkQueryPool, vkDestroyQueryPool)
  VulkanHandleSpecialisation(VkBufferView, vkDestroyBufferView)
  VulkanHandleSpecialisation(VkImageView, vkDestroyImageView)
  VulkanHandleSpecialisation(VkShaderModule, vkDestroyShaderModule)
  VulkanHandleSpecialisation(VkPipelineCache, vkDestroyPipelineCache)
  VulkanHandleSpecialisation(VkPipelineLayout, vkDestroyPipelineLayout)
  VulkanHandleSpecialisation(VkRenderPass, vkDestroyRenderPass)
  VulkanHandleSpecialisation(VkPipeline, vkDestroyPipeline)
  VulkanHandleSpecialisation(VkDescriptorSetLayout, vkDestroyDescriptorSetLayout)
  VulkanHandleSpecialisation(VkSampler, vkDestroySampler)
  VulkanHandleSpecialisation(VkDescriptorPool, vkDestroyDescriptorPool)
  VulkanHandleSpecialisation(VkFramebuffer, vkDestroyFramebuffer)
  VulkanHandleSpecialisation(VkCommandPool, vkDestroyCommandPool)
  VulkanHandleSpecialisation(VkSwapchainKHR, vkDestroySwapchainKHR)
}