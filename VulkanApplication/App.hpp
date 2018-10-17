#pragma once
#include "VulkanInterface.hpp"

class App
{
public: 
  App();
  ~App();

  void run();

private:
  void initVulkan();
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

  LIBRARY_TYPE vulkanLibrary;

  VkInstance vulkanInstance;

  //static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
  //    VkDebugReportFlagsEXT flags
  //  , VkDebugReportObjectTypeEXT objType
  //  , uint64_t obj
  //  , size_t location
  //  , int32_t code
  //  , const char *layerPrefix
  //  , const char *msg
  //  , void *userData)
  //{
  //  std::cerr << "Validation Layer: " << layerPrefix << " Message: " << msg << std::endl;
  //  return VK_FALSE;
  //}

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
