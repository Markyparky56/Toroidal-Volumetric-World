#pragma once
#ifdef _WIN32
#include <Windows.h>
#endif

#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <thread>
#include <functional>
#include <stdexcept>

#include <vulkan/vulkan.h>
#include "VulkanInterface.Functions.hpp"

#include "UnrecoverableException.hpp"

namespace VulkanInterface
{
#ifdef _WIN32
#define LIBRARY_TYPE HMODULE
#endif

  bool LoadVulkanLoaderLibrary(LIBRARY_TYPE &library);
  bool LoadVulkanFunctionGetter(LIBRARY_TYPE &library);
  bool LoadGlobalVulkanFunctions();
  bool LoadInstanceLevelVulkanFunctions(VkInstance instance
                                      , std::vector<char const *> const & enabledExtensions);
  bool LoadDeviceLevelVulkanFunctions(VkDevice device);

  bool GetAvailableLayerSupport(std::vector<VkLayerProperties> & availableLayers);
  bool IsLayerSupported( std::vector<VkLayerProperties> const & availableLayers
                       , char const * const layer);
  bool GetAvailableInstanceExtensions(std::vector<VkExtensionProperties> & availableExtensions);
  bool IsExtensionSupported( std::vector<VkExtensionProperties> const & availableExtensions
                           , char const * const extension);

  bool CreateVulkanInstance(std::vector<char const *> const & desiredExtensions
                          , std::vector<char const *> desiredLayers
                          , char const * const applicationName
                          , VkInstance & instance);

  bool SetupDebugCallback(VkInstance instance
                        , PFN_vkDebugUtilsMessengerCallbackEXT debugCallbackFunc
                        , VkDebugUtilsMessengerEXT & callback);
  void DestroyDebugUtilsMessengerEXT( VkInstance instance
                                    , VkDebugUtilsMessengerEXT callback
                                    , VkAllocationCallbacks const * pAllocator);

}
