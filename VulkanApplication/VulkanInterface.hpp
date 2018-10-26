#pragma once
#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <thread>
#include <functional>
#include <stdexcept>

#include <vulkan/vulkan.h>
#include "VulkanInterface.Functions.hpp"
#include "VulkanInterface.OSWindow.hpp"

#include "UnrecoverableException.hpp"

namespace VulkanInterface
{
  struct QueueInfo {
    uint32_t FamilyIndex;
    std::vector<float> Priorities;
  };

  struct QueueParameters {
    VkQueue Handle;
    uint32_t FamilyIndex;
  };

  struct SwapchainParameters {
    VkSwapchainKHR handle;
    VkFormat format;
    VkExtent2D size;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    std::vector<VkImageView> imageViewsRaw;
  };

  bool SetupDebugCallback(VkInstance instance
                        , PFN_vkDebugUtilsMessengerCallbackEXT debugCallbackFunc
                        , VkDebugUtilsMessengerEXT & callback);
  void DestroyDebugUtilsMessengerEXT( VkInstance instance
                                    , VkDebugUtilsMessengerEXT callback
                                    , VkAllocationCallbacks const * pAllocator);

  bool LoadVulkanLoaderLibrary(LIBRARY_TYPE &library);
  bool LoadVulkanFunctionGetter(LIBRARY_TYPE &library);
  bool LoadGlobalVulkanFunctions();
  bool LoadInstanceLevelVulkanFunctions(VkInstance instance
                                      , std::vector<char const *> const & enabledExtensions);
  bool LoadDeviceLevelVulkanFunctions(VkDevice device, std::vector<char const *> const & enabledExtensions);

  bool GetAvailableLayerSupport(std::vector<VkLayerProperties> & availableLayers);
  bool IsLayerSupported( std::vector<VkLayerProperties> const & availableLayers
                       , char const * const layer);
  bool GetAvailableInstanceExtensions(std::vector<VkExtensionProperties> & availableExtensions);
  bool IsExtensionSupported( std::vector<VkExtensionProperties> const & availableExtensions
                           , char const * const extension);

  bool CreateVulkanInstance(std::vector<char const *> const & desiredExtensions
                          , std::vector<char const *> const & desiredLayers
                          , char const * const applicationName
                          , VkInstance & instance);  

  bool EnumerateAvailablePhysicalDevices(VkInstance instance
    , std::vector<VkPhysicalDevice> & availableDevices);
  bool CheckAvailableDeviceExtensions(VkPhysicalDevice physicalDevice
                                    , std::vector<VkExtensionProperties> &availableExtensions);
  void GetFeaturesAndPropertiesOfPhysicalDevice(VkPhysicalDevice physicalDevice
                                              , VkPhysicalDeviceFeatures &deviceFeatures
                                              , VkPhysicalDeviceProperties &deviceProperties);
  bool CheckAvailableQueueFamiliesAndTheirProperties( VkPhysicalDevice physicalDevice
                                                    , std::vector<VkQueueFamilyProperties> &queueFamilies);
  bool SelectIndexOfQueueFamilyWithDesiredCapabilities( VkPhysicalDevice physicalDevice
                                                      , VkQueueFlags desiredCapabilities
                                                      , uint32_t &queueFamilyIndex);
  bool SelectQueueFamilyThatSupportsPresentationToGivenSurface(VkPhysicalDevice physicalDevice, VkSurfaceKHR presentationSurface, uint32_t & queueFamilyIndex);

  bool CreateLogicalDevice( VkPhysicalDevice physicalDevice
                          , std::vector<QueueInfo> queueInfos
                          , std::vector<char const *> const & desiredExtensions
                          , std::vector<char const *> const & desiredLayers
                          , VkPhysicalDeviceFeatures * desiredFeatures
                          , VkDevice & logicalDevice);

  bool SelectDesiredPresentationMode(VkPhysicalDevice physicalDevice, VkSurfaceKHR presentationSurface, VkPresentModeKHR desiredPresentMode, VkPresentModeKHR & presentMode);
  bool CreatePresentationSurface(VkInstance instance, WindowParameters windowParameters, VkSurfaceKHR & presentationSurface);
}
