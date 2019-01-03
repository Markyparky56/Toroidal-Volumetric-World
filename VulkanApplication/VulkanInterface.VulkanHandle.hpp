#pragma once
// Based on Vulkan Cookbook's VkDestroyer Class
#include <utility>
#include <functional>
#include "VulkanInterface.Functions.hpp"

namespace VulkanInterface
{
  struct VkInstanceWrapper
  {
    VkInstance handle;
  };

  struct VkDeviceWrapper
  {
    VkDevice handle;
  };

  struct VkSurfaceKHRWrapper
  {
    VkSurfaceKHR handle;
  };

  template<class VkType>
  void DestroyVulkanObject(VkType object);

  template<>
  inline void DestroyVulkanObject<VkInstanceWrapper>(VkInstanceWrapper object)
  {
    vkDestroyInstance(object.handle, nullptr);
  }

  template<>
  inline void DestroyVulkanObject<VkDeviceWrapper>(VkDeviceWrapper object)
  {
    vkDestroyDevice(object.handle, nullptr);
  }

  template<class VkParent, class VkChild>
  void DestroyVulkanObject(VkParent parent, VkChild object);

  template<>
  inline void DestroyVulkanObject<VkInstance, VkSurfaceKHRWrapper>(VkInstance instance, VkSurfaceKHRWrapper surface)
  {
    vkDestroySurfaceKHR(instance, surface.handle, nullptr);
  }

#define VulkanHandleSpecialisation(VkChild, VkDeleter) \
  struct VkChild##Wrapper{ \
    VkChild handle; \
  }; \
      \
  template<> \
  inline void DestroyVulkanObject<VkDevice, VkChild##Wrapper>(VkDevice device, VkChild##Wrapper object) { \
    VkDeleter(device, object.handle, nullptr); \
  } \

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
  

  template<class VkTypeWrapper>
  class VulkanHandle 
  {
  public:
    VulkanHandle()
      : DestroyerFunction(nullptr)
    {
      obj.handle = nullptr;
    }

    VulkanHandle(std::function<void(VkTypeWrapper)> destroyerFunction)
      : DestroyerFunction(destroyerFunction)
    {
      obj.handle = nullptr;
    }

    VulkanHandle(VkTypeWrapper object, std::function<void(VkTypeWrapper)> destroyerFunction)
      : DestroyerFunction(destroyerFunction)
    {
      obj.handle = object.handle;
    }

    ~VulkanHandle()
    {
      if (DestroyerFunction && obj.handle)
      {
        DestroyerFunction(obj);
      }
    }

    VulkanHandle(VulkanHandle<VkTypeWrapper> && other)
      : DestroyerFunction(other.DestroyerFunction)
    {
      obj.handle = other.obj.handle;
      other.obj.handle = nullptr;
      other.DestroyerFunction = nullptr;
    }

    VulkanHandle & operator=(VulkanHandle<VkTypeWrapper> && other)
    {
      if (&other != this)
      {
        VkTypeWrapper object = obj;
        std::function<void(VkTypeWrapper)> destroyerFunction = DestroyerFunction;

        obj.handle = other.obj.handle;
        DestroyerFunction = other.DestroyerFunction;

        other.obj.handle = obj.handle;
        other.DestroyerFunction = destroyerFunction;
      }     

      return *this;
    }

    decltype(VkTypeWrapper::handle) & operator*()
    {
      return obj.handle;
    }

    decltype(VkTypeWrapper::handle) const & operator*() const
    {
      return obj.handle;
    }

    bool operator!() const
    {
      return obj.hande == nullptr;
    }

    operator bool() const
    {
      return obj.handle != nullptr;
    }

    VulkanHandle(VulkanHandle<VkTypeWrapper> const &) = delete;
    VulkanHandle operator=(VulkanHandle<VkTypeWrapper> const &) = delete;

  private:
    VkTypeWrapper obj;
    std::function<void(VkTypeWrapper)> DestroyerFunction;
  };

#define VulkanHandle(VkType) VulkanHandle<VulkanInterface::VkType##Wrapper>

  inline void InitVulkanHandle(VulkanHandle<VkInstanceWrapper> & handle)
  {
    handle = VulkanHandle<VkInstanceWrapper>(std::bind(DestroyVulkanObject<VkInstanceWrapper>, std::placeholders::_1));
  }

  inline void InitVulkanHandle(VulkanHandle<VkDeviceWrapper> & handle)
  {
    handle = VulkanHandle<VkDeviceWrapper>(std::bind(DestroyVulkanObject<VkDeviceWrapper>, std::placeholders::_1));
  }

  template<class VkParent, class VkType>
  inline void InitVulkanHandle(VkParent const & parent, VulkanHandle<VkType> & handle)
  {
    handle = VulkanHandle<VkType>(std::bind(VulkanHandleHelper::DestroyVulkanObject<VkParent, VkType>, parent, std::placeholders::_1));
  }

  template<class VkParent, class VkType>
  inline void InitVulkanHandle(VulkanHandle<VkParent> const & parent, VulkanHandle<VkType> & handle)
  {
    handle = VulkanHandle<VkType>(std::bind(VulkanHandleHelper::DestroyVulkanObject<decltype(VkParent::handle), VkType), *parent, std::placeholders::_1));
  }
}
