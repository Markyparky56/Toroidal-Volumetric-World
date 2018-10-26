#pragma once
#include "VulkanInterface.hpp"

class AppBase
{
public: 
  AppBase();
  ~AppBase();

  virtual bool InitVulkan(VulkanInterface::WindowParameters windowParameters);
  virtual void Update();
  virtual void Resize();
  virtual void Shutdown();
  virtual void Run();
  virtual void MouseClick(size_t buttonIndex, bool state);
  virtual void MouseWheel(float distance);
  virtual void MouseReset();
  virtual void UpdateTime();
  virtual bool IsReady();

private:
  bool ready;
  void cleanup();

  std::vector<char const *> desiredLayers = {
#ifdef _DEBUG
  "VK_LAYER_LUNARG_standard_validation"
#endif
  };

  std::vector<char const *> desiredExtensions = {
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#ifdef VK_USE_PLATFORM_WIN32_KHR
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif defined VK_USE_PLATFORM_XCB_KHR
    VK_KHR_XCB_SURFACE_EXTENSION_NAME,
#elif defined VK_USE_PLATFORM_XLIB_KHR
    VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
#endif
  };

  VkPhysicalDeviceFeatures desiredDeviceFeatures;

  LIBRARY_TYPE vulkanLibrary;

  VkInstance vulkanInstance;
  VkPhysicalDevice vulkanPhysicalDevice;
  VkDevice vulkanDevice;
  VkSurfaceKHR presentationSurface;
  VulkanInterface::QueueParameters graphicsQueue;
  VulkanInterface::QueueParameters computeQueue;
  VulkanInterface::QueueParameters presentQueue;
  VulkanInterface::SwapchainParameters swapchain;

  VkDebugUtilsMessengerEXT callback;

  static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
      VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity
    , VkDebugUtilsMessageTypeFlagsEXT messageType
    , const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData
    , void* pUserData) {
    std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
  }
};
                                                                                                                                                                                                        