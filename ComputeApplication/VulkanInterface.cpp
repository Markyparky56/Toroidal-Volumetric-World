#include "VulkanInterface.hpp"
#include "vk_mem_alloc.h"
#include <sstream>

namespace VulkanInterface
{
  bool SetupDebugCallback(VkInstance instance
    , PFN_vkDebugUtilsMessengerCallbackEXT debugCallbackFunc
    , VkDebugUtilsMessengerEXT & callback)
  {
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {
      VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
      nullptr,
      0,
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT, // messageSeverity
      VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT, // messageType
      debugCallbackFunc,
      nullptr
    };

    if (vkCreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &callback) != VK_SUCCESS)
    {
      // TODO: error, (std::runtime_error("Failed to set up debug callback!"));
      return false;
    }
    return true;
  }

  // Functions for Loading Vulkan Library & Functions
  bool LoadVulkanLoaderLibrary(LIBRARY_TYPE & library)
  {
#if defined(_WIN32)
    library = LoadLibrary("vulkan-1.dll");
#elif defined(__linux)
    library = dlopen("libvulkan.so.1", RTLD_NOW);
#endif
    if (library == nullptr)
    {
      // TODO: error, (std::runtime_error("Unable to load vulkan library, ptr returned is nullptr"));
    }
    return (library != nullptr);
  }

  void ReleaseVulkanLoaderLibrary(LIBRARY_TYPE & library)
  {
    if (library != nullptr)
    {
#if defined _WIN32
      FreeLibrary(library);
#elif defined __linux
      dlclose(library);
#endif
      library = nullptr;
    }
  }

  bool LoadVulkanFunctionGetter(LIBRARY_TYPE & library)
  {
#if defined(_WIN32)
#define LoadFunction GetProcAddress
#elif defined(__linux)
#define LoadFunction dlsym
#endif

#define EXPORTED_VULKAN_FUNCTION( name ) \
    name = (PFN_##name)LoadFunction( library, #name ); \
    if(name == nullptr) { \
      std::cout << "Failed to load function " << #name << std::endl; \
      return false; \
    } 
     
    // This gets us the vkGetInstanceProcAddr function
#include "ListOfVulkanFunctions.inl"
    return true;
  }

  bool LoadGlobalVulkanFunctions()
  {
#define GLOBAL_LEVEL_VULKAN_FUNCTION( name ) \
    name = (PFN_##name)vkGetInstanceProcAddr( nullptr, #name ); \
    if(name == nullptr) { \
      std::cout << "Failed to load global-level function " << #name << std::endl; \
      return false; \
    }

    // This include gets us the global vulkan functions, such as vkCreateInstance
#include "ListOfVulkanFunctions.inl" 
    return true;
  }

  bool LoadInstanceLevelVulkanFunctions(VkInstance instance
                                      , std::vector<char const *> const & enabledExtensions)
  {
    // Load core Vulkan instance-level functions
#define INSTANCE_LEVEL_VULKAN_FUNCTION( name ) \
    name = (PFN_##name)vkGetInstanceProcAddr( instance, #name ); \
    if (name == nullptr) { \
      std::cout << "Failed to load instance-level function " << #name << std::endl; \
      return false; \
    }

    // Load extension Vulkan instance-level functions
#define INSTANCE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION( name, extension ) \
    for(auto & enabledExtension : enabledExtensions) { \
      if(std::string(enabledExtension) == std::string(extension)) { \
        name = (PFN_##name)vkGetInstanceProcAddr( instance, #name ); \
        std::cout << name << std::endl; \
        if (name == nullptr) { \
          std::cout << "Failed to load extension instance-level function " <<  #name << std::endl; \
          return false; \
        } \
      } \
    }

#include "ListOfVulkanFunctions.inl"
    return true;
  }

  bool LoadDeviceLevelVulkanFunctions(VkDevice device, std::vector<char const*> const & enabledExtensions)
  {
    // Load core Vulkan device-level functions
#define DEVICE_LEVEL_VULKAN_FUNCTION( name ) \
    name = (PFN_##name)vkGetDeviceProcAddr( device, #name ); \
    if(name == nullptr) { \
      /* TODO: error, (std::runtime_error("Failed to load device-level function " #name));*/ \
      return false; \
    }

    // Load extension Vulkan device-level functions
#define DEVICE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION( name, extension ) \
    for( auto & enabledExtension : enabledExtensions) { \
      if(std::string(enabledExtension) == std::string(extension)) { \
        name = (PFN_##name)vkGetDeviceProcAddr(device, #name); \
        if(name == nullptr) { \
          /* TODO: error, (std::runtime_error("Failed to load extension device-level function " #name));*/ \
          return false; \
        } \
      } \
    }

#include "ListOfVulkanFunctions.inl"
    return true;
  }

  // Functions to get a vector of the supported validation layers
  bool GetAvailableLayerSupport(std::vector<VkLayerProperties>& availableLayers)
  {
    uint32_t availableLayersCount = 0;
    VkResult result = VK_SUCCESS;

    result = vkEnumerateInstanceLayerProperties(&availableLayersCount, nullptr);
    if ((result != VK_SUCCESS) || (availableLayersCount < static_cast<uint32_t>(availableLayers.size())))
    {
      // TODO: error, (std::runtime_error("Could not get the number of available instance layers"));
      return false;
    }

    availableLayers.resize(availableLayersCount);
    result = vkEnumerateInstanceLayerProperties(&availableLayersCount, availableLayers.data());
    if ((result != VK_SUCCESS) || (availableLayersCount < static_cast<uint32_t>(availableLayers.size())))
    {
      // TODO: error, (std::runtime_error("Could not enumerate available instance layers"));
      return false;
    }

    return true;
  }

  bool IsLayerSupported(std::vector<VkLayerProperties> const & availableLayers
                      , char const * const layer)
  {
    bool found = false;
    for (auto & layerProperties : availableLayers)
    {
      if (strstr(layerProperties.layerName, layer))
      {
        found = true;
        break;
      }
    }
    if (!found)
    {
      // TODO: error, (std::runtime_error("Desired instance layer not found in available layers list"));
    }
    return found;
  }  

  // Functions to get a vector of the supported extensions
  bool GetAvailableInstanceExtensions(std::vector<VkExtensionProperties>& availableExtensions)
  {
    uint32_t extensionsCount = 0;
    VkResult result = VK_SUCCESS;

    result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, nullptr);
    if ((result != VK_SUCCESS) || (extensionsCount == 0))
    {
      // TODO: error, (std::runtime_error("Could not get the number of instance extensions"));
      return false;
    }

    availableExtensions.resize(extensionsCount);
    result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, availableExtensions.data());
    if ((result != VK_SUCCESS) || (extensionsCount == 0))
    {
      // TODO: error, (std::runtime_error("Could not enumerate instance extensions"));
      return false;
    }

    return true;
  }

  bool IsExtensionSupported(std::vector<VkExtensionProperties> const & availableExtensions
                          , char const * const extension)
  {
    for (auto & availableExtension : availableExtensions)
    {
      if (strstr(availableExtension.extensionName, extension))
      {
        return true;
      }
    }
    return false;
  }

  // Function to create an instance
  bool CreateVulkanInstance(std::vector<char const *> const & desiredExtensions
                          , std::vector<char const *> const & desiredLayers
                          , char const * const applicationName
                          , VkInstance & instance)
  {
    std::vector<VkExtensionProperties> availableExtensions;
    if (!GetAvailableInstanceExtensions(availableExtensions))
    {
      return false;
    }
    

    // Search through availableExtensions for extensions in desiredExtensions
    for (auto & desiredExtension : desiredExtensions)
    {
      if (!IsExtensionSupported(availableExtensions, desiredExtension))
      {
        return false;
      }
    }

    std::vector<VkLayerProperties> availableLayers;
    if(!GetAvailableLayerSupport(availableLayers))
    {
      return false;
    }    

    // Search through availableLayers for layers in desiredLayers
    for (auto & desiredLayer : desiredLayers)
    {
      if(!IsLayerSupported(availableLayers, desiredLayer))
      {
        return false;
      }
    }

    VkApplicationInfo applicationInfo = {
      VK_STRUCTURE_TYPE_APPLICATION_INFO,
      nullptr,
      applicationName,
      VK_MAKE_VERSION(0,0,0),
      "VulkanApplication",
      VK_MAKE_VERSION(0,0,0),
      VK_MAKE_VERSION(0,0,0)
    };

    VkInstanceCreateInfo instanceCreateInfo = {
      VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      nullptr,
      0,
      &applicationInfo,
      static_cast<uint32_t>(desiredLayers.size()),
      desiredLayers.data(),
      static_cast<uint32_t>(desiredExtensions.size()),
      desiredExtensions.data()
    };

    VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);
    if ((result != VK_SUCCESS) || (instance == VK_NULL_HANDLE))
    {
      // TODO: error, (std::runtime_error("Could not create Vulkan Instance"));
      return false;
    }

    return true;
  }

  // Functions to aide selecting the correct logical device to create
  bool EnumerateAvailablePhysicalDevices( VkInstance instance
                                        , std::vector<VkPhysicalDevice> & availableDevices)
  {
    uint32_t devicesCount = 0;
    VkResult result = VK_SUCCESS;

    result = vkEnumeratePhysicalDevices(instance, &devicesCount, nullptr);
    if ((result != VK_SUCCESS) || (devicesCount == 0))
    {
      // TODO: error, (std::runtime_error("Could not get the number of available physical devices"));
      return false;
    }

    availableDevices.resize(devicesCount);
    result = vkEnumeratePhysicalDevices(instance, &devicesCount, availableDevices.data());
    if ((result != VK_SUCCESS) || (devicesCount == 0))
    {
      // TODO: error, (std::runtime_error("Could not enumerate physical devices"));
      return false;
    }

    return true;
  }

  bool CheckAvailableDeviceExtensions(VkPhysicalDevice physicalDevice
                                    , std::vector<VkExtensionProperties> &availableExtensions)
  {
    uint32_t extensionsCount = 0;
    VkResult result = VK_SUCCESS;

    result = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionsCount, nullptr);
    if ((result != VK_SUCCESS) || (extensionsCount == 0))
    {
      // TODO: error,  std::runtime_error("Could not get the number of device extensions");
      return false;
    }

    availableExtensions.resize(extensionsCount);
    result = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionsCount, availableExtensions.data());
    if ((result != VK_SUCCESS) || (extensionsCount == 0))
    {
      // TODO: error,  std::runtime_error("Could not enumerate device extensions");
      return false;
    }

    return true;
  }

  void GetFeaturesAndPropertiesOfPhysicalDevice(VkPhysicalDevice physicalDevice
                                              , VkPhysicalDeviceFeatures &deviceFeatures
                                              , VkPhysicalDeviceProperties &deviceProperties)
  {
    vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
  }

  bool CheckAvailableQueueFamiliesAndTheirProperties( VkPhysicalDevice physicalDevice
                                                    , std::vector<VkQueueFamilyProperties> &queueFamilies)
  {
    uint32_t queueFamiliesCount = 0;

    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamiliesCount, nullptr);
    if (queueFamiliesCount == 0)
    {
      // TODO: error,  std::runtime_error("Could not get the number of queue families");
      return false;
    }

    queueFamilies.resize(queueFamiliesCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamiliesCount, queueFamilies.data());
    if (queueFamiliesCount == 0)
    {
      // TODO: error,  std::runtime_error("Could not acquire properties of queue families");
      return false;
    }

    return true;
  }

  bool SelectIndexOfQueueFamilyWithDesiredCapabilities( VkPhysicalDevice physicalDevice
                                                      , VkQueueFlags desiredCapabilities
                                                      , uint32_t &queueFamilyIndex)
  {
    std::vector<VkQueueFamilyProperties> queueFamilies;
    if (!CheckAvailableQueueFamiliesAndTheirProperties(physicalDevice, queueFamilies))
    {
      return false;
    }

    for (uint32_t index = 0; index < static_cast<uint32_t>(queueFamilies.size()); ++index)
    {
      if ((queueFamilies[index].queueCount > 0) 
      && ((queueFamilies[index].queueFlags & desiredCapabilities) == desiredCapabilities))
      {
        queueFamilyIndex = index;
        return true;
      }
    }
    
    return false;
  }

  bool SelectQueueFamilyThatSupportsPresentationToGivenSurface(VkPhysicalDevice physicalDevice, VkSurfaceKHR presentationSurface, uint32_t & queueFamilyIndex)
  {
    std::vector<VkQueueFamilyProperties> queueFamilies;

    if (!CheckAvailableQueueFamiliesAndTheirProperties(physicalDevice, queueFamilies))
    {
      return false;
    }    

    for (uint32_t index = 0; index < static_cast<uint32_t>(queueFamilies.size()); ++index)
    {
      VkBool32 presentationSupported = VK_FALSE;
      VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, index, presentationSurface, &presentationSupported);
      if ((result == VK_SUCCESS) && (presentationSupported == VK_TRUE))
      {
        queueFamilyIndex = index;
        return true;
      }
    }

    return false;
  }

  // Function to create a logical device
  bool CreateLogicalDevice( VkPhysicalDevice physicalDevice
                          , std::vector<QueueInfo> queueInfos
                          , std::vector<char const *> const & desiredExtensions
                          , std::vector<char const *> const & desiredLayers
                          , VkPhysicalDeviceFeatures * desiredFeatures
                          , VkDevice & logicalDevice)
  {
    std::vector<VkExtensionProperties> availableExtensions;
    if (!CheckAvailableDeviceExtensions(physicalDevice, availableExtensions))
    {
      return false;
    }

    for (auto & extension : desiredExtensions)
    {
      if (!IsExtensionSupported(availableExtensions, extension))
      {
        std::stringstream ss;
        ss << "Extension named " << extension << " is not supported by a physical device";
        return false;
      }
    }

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    for (auto & info : queueInfos)
    {
      queueCreateInfos.push_back({
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        nullptr,
        0,
        info.familyIndex,
        static_cast<uint32_t>(info.priorities.size()),
        info.priorities.data()
        }
      );      
    }

    VkDeviceCreateInfo deviceCreateInfo = {
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        nullptr,
        0,
        static_cast<uint32_t>(queueCreateInfos.size()),
        queueCreateInfos.data(),
        static_cast<uint32_t>(desiredLayers.size()),
        desiredLayers.data(),
        static_cast<uint32_t>(desiredExtensions.size()),
        desiredExtensions.data(),
        desiredFeatures
    };

    VkResult result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &logicalDevice);
    if ((result != VK_SUCCESS) || (logicalDevice == VK_NULL_HANDLE))
    {
      // TODO: error,  std::runtime_error("Could not create logical device");
      return false;
    }

    return true;
  }

  bool CreatePresentationSurface(VkInstance instance, WindowParameters windowParameters, VkSurfaceKHR & presentationSurface)
  {
    VkResult result;

#ifdef VK_USE_PLATFORM_WIN32_KHR
    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {
      VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
      nullptr,
      0,
      windowParameters.HInstance,
      windowParameters.HWnd
    };

    result = vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, &presentationSurface);

#elif defined VK_USE_PLATFORM_XLIB_KHR
    VkXlibSurfaceCreateInfoKHR surfaceCreateInfo = {
      VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
      nullptr,
      0,
      windowParameters.Dpy,
      windowParameters.Window
    };

    result = vkCreateXlibSurfaceKHR(instance, &surfaceCreateInfo, nullptr, &presentationSurface);

#elif defined VK_USE_PLATFORM_XCB_KHR
    VkXcbSurfaceCreateInfoKHR surfaceCreateInfo = {
      VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
      nullptr,
      0,
      windowParameters.Connection,
      windowParameters.Window
    };

    result = vkCreateXcbSurfaceKHR(instance, &surfaceCreateInfo, nullptr, &presentationSurface);
#endif

    if ((result != VK_SUCCESS) || (presentationSurface == VK_NULL_HANDLE))
    {
      // TODO: error,  std::runtime_error("Could not create presentation surface");
      return false;
    }

    return true;
  }

  bool SelectDesiredPresentationMode(VkPhysicalDevice physicalDevice, VkSurfaceKHR presentationSurface, VkPresentModeKHR desiredPresentMode, VkPresentModeKHR & presentMode)
  {
    // Enumerate Supported Present Modes
    uint32_t presentModesCount = 0;
    VkResult result = VK_SUCCESS;

    result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, presentationSurface, &presentModesCount, nullptr);
    if ((result != VK_SUCCESS) || (presentModesCount == 0))
    {
      // TODO: error,  std::runtime_error("Could not get the number of supported present modes");
      return false;
    }

    std::vector<VkPresentModeKHR> presentModes(presentModesCount);
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, presentationSurface, &presentModesCount, presentModes.data());
    if ((result != VK_SUCCESS) || (presentModesCount == 0))
    {
      // TODO: error,  std::runtime_error("Could not enumerate present modes");
      return false;
    }

    // Select present mode
    for (auto & currentPresentMode : presentModes)
    {
      if (currentPresentMode == desiredPresentMode)
      {
        presentMode = desiredPresentMode;
        return true;
      }
    }

    std::cout << "Desired present mode is not supported. Selecting default FIFO mode" << std::endl;
    for (auto & currentPresentMode : presentModes)
    {
      if (currentPresentMode == VK_PRESENT_MODE_FIFO_KHR)
      {
        presentMode = VK_PRESENT_MODE_FIFO_KHR;
        return true;
      }
    }

    // TODO: error,  std::runtime_error("VK_PRESENT_MODE_FIFO_KHR is not supported! Run for the hills!");
    return false;
  }

  bool GetCapabilitiesOfPresentationSurface(VkPhysicalDevice physicalDevice, VkSurfaceKHR presentationSurface, VkSurfaceCapabilitiesKHR & surfaceCapabilities)
  {
    VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, presentationSurface, &surfaceCapabilities);
    
    if (result != VK_SUCCESS)
    {
      // TODO: error "Could not get the capabilities of a presentation surface"
      return false;
    }
    return true;
  }

  bool SelectNumberOfSwapchainImages(VkSurfaceCapabilitiesKHR const & surfaceCapabilities, uint32_t & numberOfImages)
  {
    numberOfImages = surfaceCapabilities.minImageCount + 1;
    if ((surfaceCapabilities.maxImageCount > 0) && (numberOfImages > surfaceCapabilities.maxImageCount))
    {
      numberOfImages = surfaceCapabilities.maxImageCount;
    }
    return true;
  }

  bool ChooseSizeOfSwapchainImages(VkSurfaceCapabilitiesKHR const & surfaceCapabilities, VkExtent2D & sizeOfImages)
  {
    // If this is true we've got a special case where window size is determined by the size of the swapchain image
    if (surfaceCapabilities.currentExtent.width == 0xFFFFFFFF)
    {
      sizeOfImages = { 640, 480 }; // Some default

      if (sizeOfImages.width < surfaceCapabilities.minImageExtent.width)
      {
        sizeOfImages.width = surfaceCapabilities.minImageExtent.width;
      }
      else if (sizeOfImages.width > surfaceCapabilities.maxImageExtent.width)
      {
        sizeOfImages.width = surfaceCapabilities.maxImageExtent.width;
      }

      if (sizeOfImages.height < surfaceCapabilities.minImageExtent.height)
      {
        sizeOfImages.height = surfaceCapabilities.minImageExtent.height;
      }
      else if (sizeOfImages.height > surfaceCapabilities.maxImageExtent.height)
      {
        sizeOfImages.height = surfaceCapabilities.maxImageExtent.height;
      }
    }
    else // We're cool
    {
      sizeOfImages = surfaceCapabilities.currentExtent;
    }
    return true;
  }

  bool SelectDesiredUsageScenariosOfSwapchainImages(VkSurfaceCapabilitiesKHR const & surfaceCapabilities, VkImageUsageFlags desiredUsages, VkImageUsageFlags & imageUsage)
  {
    imageUsage = desiredUsages & surfaceCapabilities.supportedUsageFlags;
    return desiredUsages == imageUsage;
  }

  bool SelectTransformationOfSwapchainImages(VkSurfaceCapabilitiesKHR const & surfaceCapabilities, VkSurfaceTransformFlagBitsKHR desiredTransform, VkSurfaceTransformFlagBitsKHR & surfaceTransform)
  {
    if (surfaceCapabilities.supportedTransforms & desiredTransform)
    {
      surfaceTransform = desiredTransform;
    }
    else
    {
      surfaceTransform = surfaceCapabilities.currentTransform;
    }
    return true;
  }

  bool SelectFormatOfSwapchainImages(VkPhysicalDevice physicalDevice, VkSurfaceKHR presentationSurface, VkSurfaceFormatKHR desiredSurfaceFormat, VkFormat & imageFormat, VkColorSpaceKHR & imageColorSpace)
  {
    // Enumerate supported formats
    uint32_t formatsCount = 0;
    VkResult result = VK_SUCCESS;

    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, presentationSurface, &formatsCount, nullptr);
    if ((result != VK_SUCCESS) || (formatsCount == 0))
    {
      // TODO: error, "Could not get the number of supported surface formats"
      return false;
    }

    std::vector<VkSurfaceFormatKHR> surfaceFormats(formatsCount);
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, presentationSurface, &formatsCount, surfaceFormats.data());
    if ((result != VK_SUCCESS) || (formatsCount == 0))
    {
      // TODO: error, "Could not enumerate supported surface formats"
      return false;
    }

// Select surface format
if ((surfaceFormats.size() == 1) && (surfaceFormats[0].format == VK_FORMAT_UNDEFINED))
{
  imageFormat = desiredSurfaceFormat.format;
  imageColorSpace = desiredSurfaceFormat.colorSpace;
  return true;
}

for (auto & surfaceFormat : surfaceFormats)
{
  if ((surfaceFormat.format == desiredSurfaceFormat.format) && (surfaceFormat.colorSpace == desiredSurfaceFormat.colorSpace))
  {
    imageFormat = desiredSurfaceFormat.format;
    imageColorSpace = desiredSurfaceFormat.colorSpace;
    return true;
  }
}

for (auto & surfaceFormat : surfaceFormats)
{
  if (surfaceFormat.format == desiredSurfaceFormat.format)
  {
    imageFormat = desiredSurfaceFormat.format;
    imageColorSpace = surfaceFormat.colorSpace;
    // TODO: warn, "Desired combination of format and colourspace not supported, selecting other colorspace"
    return true;
  }
}

imageFormat = surfaceFormats[0].format;
imageColorSpace = surfaceFormats[0].colorSpace;
// TODO: warn, "Desired format is not supported, selecting first available format/colourspace combination"
return true;
  }

  bool CreateSwapchain(VkDevice logicalDevice, VkSurfaceKHR presentationSurface, uint32_t imageCount, VkSurfaceFormatKHR surfaceFormat, VkExtent2D imageSize, VkImageUsageFlags imageUsage, VkSurfaceTransformFlagBitsKHR surfaceTransform, VkPresentModeKHR presentMode, VkSwapchainKHR & oldSwapchain, VkSwapchainKHR & swapchain)
  {
    VkSwapchainCreateInfoKHR swapchainCreateInfo = {
      VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      nullptr,
      0,
      presentationSurface,
      imageCount,
      surfaceFormat.format,
      surfaceFormat.colorSpace,
      imageSize,
      1,
      imageUsage,
      VK_SHARING_MODE_EXCLUSIVE,
      0,
      nullptr,
      surfaceTransform,
      VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      presentMode,
      VK_TRUE,
      oldSwapchain
    };

    VkResult result = vkCreateSwapchainKHR(logicalDevice, &swapchainCreateInfo, nullptr, &swapchain);
    if ((result != VK_SUCCESS) || (swapchain == nullptr))
    {
      // TODO: error, "Could not create a swapchain"
      return false;
    }

    if (oldSwapchain != nullptr)
    {
      vkDestroySwapchainKHR(logicalDevice, oldSwapchain, nullptr);
      oldSwapchain = nullptr;
    }

    return true;
  }

  bool CreateStandardSwapchain(VkPhysicalDevice physicalDevice, VkSurfaceKHR presentationSurface, VkDevice logicalDevice, VkImageUsageFlags swapchainImageUsage, VkExtent2D & imageSize, VkFormat & imageFormat, VkSwapchainKHR & oldSwapchain, VkSwapchainKHR & swapchain, std::vector<VkImage>& swapchainImages)
  {
    VkPresentModeKHR desiredPresentMode;
    if (!SelectDesiredPresentationMode(physicalDevice, presentationSurface, VK_PRESENT_MODE_MAILBOX_KHR, desiredPresentMode))
    {
      return false;
    }

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    if (!GetCapabilitiesOfPresentationSurface(physicalDevice, presentationSurface, surfaceCapabilities))
    {
      return false;
    }

    uint32_t numberOfImages;
    if (!SelectNumberOfSwapchainImages(surfaceCapabilities, numberOfImages))
    {
      return false;
    }

    if (!ChooseSizeOfSwapchainImages(surfaceCapabilities, imageSize))
    {
      return false;
    }

    if ((imageSize.width == 0) || (imageSize.height == 0))
    {
      return false; 
    }

    VkImageUsageFlags imageUsage;
    if (!SelectDesiredUsageScenariosOfSwapchainImages(surfaceCapabilities, swapchainImageUsage, imageUsage))
    {
      return false;
    }

    VkSurfaceTransformFlagBitsKHR surfaceTransform;
    SelectTransformationOfSwapchainImages(surfaceCapabilities, VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR, surfaceTransform);

    VkColorSpaceKHR imageColorSpace;
    if (!SelectFormatOfSwapchainImages(physicalDevice, presentationSurface, { VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }, imageFormat, imageColorSpace))
    {
      return false;
    }

    if (!CreateSwapchain(logicalDevice, presentationSurface, numberOfImages, { imageFormat, imageColorSpace }, imageSize, imageUsage, surfaceTransform, desiredPresentMode, oldSwapchain, swapchain))
    {
      return false;
    }

    if (!GetHandlesOfSwapchainImages(logicalDevice, swapchain, swapchainImages))
    {
      return false;
    }
    
    return true;
  }

  bool GetHandlesOfSwapchainImages(VkDevice logicalDevice, VkSwapchainKHR swapchain, std::vector<VkImage>& swapchainImages)
  {
    uint32_t imagesCount = 0;
    VkResult result = VK_SUCCESS;

    result = vkGetSwapchainImagesKHR(logicalDevice, swapchain, &imagesCount, nullptr);
    if ((result != VK_SUCCESS) || (imagesCount == 0))
    {
      // TODO: error, "Could not get the number of swapchain images"
      return false;
    }

    swapchainImages.resize(imagesCount);
    result = vkGetSwapchainImagesKHR(logicalDevice, swapchain, &imagesCount, swapchainImages.data());
    if ((result != VK_SUCCESS) || (imagesCount == 0))
    {
      // TODO: error, "Could not enumerate swapchain images"
      return false;
    }

    return true;
  }

  bool AcquireSwapchainImage(VkDevice logicalDevice, VkSwapchainKHR swapchain, VkSemaphore semaphore, VkFence fence, uint32_t & imageIndex)
  {
    VkResult result;

    result = vkAcquireNextImageKHR(logicalDevice, swapchain, 2000000000, semaphore, fence, &imageIndex);
    switch (result)
    {
    case VK_SUCCESS:
    case VK_SUBOPTIMAL_KHR:
      return true;
    default:
      return false;
    }
  }

  bool PresentImage(VkQueue queue, std::vector<VkSemaphore> renderingSemaphores, std::vector<PresentInfo> imagesToPresent)
  {
    VkResult result;
    std::vector<VkSwapchainKHR> swapchains;
    std::vector<uint32_t> imageIndices;

    for (auto & imageToPresent : imagesToPresent)
    {
      swapchains.emplace_back(imageToPresent.swapchain);
      imageIndices.emplace_back(imageToPresent.imageIndex);
    }

    VkPresentInfoKHR presentInfo = {
      VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      nullptr,
      static_cast<uint32_t>(renderingSemaphores.size()),
      renderingSemaphores.data(),
      static_cast<uint32_t>(swapchains.size()),
      swapchains.data(),
      imageIndices.data(),
      nullptr
    };

    result = vkQueuePresentKHR(queue, &presentInfo);
    switch (result) 
    {
    case VK_SUCCESS:
      return true;
    default:
      return false;
    }
  }

  bool CreateCommandPool(VkDevice logicalDevice, VkCommandPoolCreateFlags parameters, uint32_t queueFamily, VkCommandPool & commandPool)
  {
    VkCommandPoolCreateInfo commandPoolCreateInfo = {
      VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      nullptr,
      parameters,
      queueFamily
    };

    VkResult result = vkCreateCommandPool(logicalDevice, &commandPoolCreateInfo, nullptr, &commandPool);
    if (result != VK_SUCCESS)
    {
      // TODO: error, "Could not create command pool"
      return false;
    }
    return true;
  }

  bool AllocateCommandBuffers(VkDevice logicalDevice, VkCommandPool commandPool, VkCommandBufferLevel level, uint32_t count, std::vector<VkCommandBuffer>& commandBuffers)
  {
    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      nullptr,
      commandPool,
      level,
      count
    };

    commandBuffers.resize(count);

    VkResult result = vkAllocateCommandBuffers(logicalDevice, &commandBufferAllocateInfo, commandBuffers.data());
    if (result != VK_SUCCESS)
    {
      // TODO: error, "Could not allocate command buffers"
      return false;
    }
    return true;
  }

  bool BeginCommandBufferRecordingOp(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags usage, VkCommandBufferInheritanceInfo * secondaryCommandBufferInfo)
  {
    VkCommandBufferBeginInfo commandBufferBeginInfo = {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      nullptr,
      usage,
      secondaryCommandBufferInfo
    };

    VkResult result = vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
    if (result != VK_SUCCESS)
    {
      // TODO: error, "Could not begin command buffer recording operation"
      return false;
    }
    return true;
  }

  bool EndCommandBufferRecordingOp(VkCommandBuffer commandBuffer)
  {
    VkResult result = vkEndCommandBuffer(commandBuffer);
    if (result != VK_SUCCESS)
    {
      // TODO: error, "Error occurred during command buffer recording"
      return false;
    }
    return true;
  }

  bool ResetCommandBuffer(VkCommandBuffer commandBuffer, bool releaseResources)
  {
    VkResult result = vkResetCommandBuffer(commandBuffer, releaseResources ? VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT : 0);
    if (result != VK_SUCCESS)
    {
      // TODO: error, "Error occured during command buffer reset"
      return false;
    }
    return true;
  }

  bool ResetCommandPool(VkDevice logicalDevice, VkCommandPool commandPool, bool releaseResources)
  {
    VkResult result = vkResetCommandPool(logicalDevice, commandPool, releaseResources ? VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT : 0);
    if (result != VK_SUCCESS)
    {
      // TODO: error, "Error occurred during command pool reset"
      return false;
    }
    return true;
  }

  bool CreateSemaphore(VkDevice logicalDevice, VkSemaphore & semaphore)
  {
    VkSemaphoreCreateInfo semaphoreCreateInfo = {
      VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      nullptr,
      0
    };

    VkResult result = vkCreateSemaphore(logicalDevice, &semaphoreCreateInfo, nullptr, &semaphore);
    if (result != VK_SUCCESS)
    {
      // TODO: error, "Could not create semaphore"
      return false;
    }
    return true;
  }

  bool CreateFence(VkDevice logicalDevice, bool signaled, VkFence & fence)
  {
    VkFenceCreateInfo fenceCreateInfo = {
      VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      nullptr,
      signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0u
    };

    VkResult result = vkCreateFence(logicalDevice, &fenceCreateInfo, nullptr, &fence);
    if (result != VK_SUCCESS)
    {
      // TODO: error, "Could not create a fence"
      return false;
    }
    return true;
  }

  bool WaitForFences(VkDevice logicalDevice, std::vector<VkFence> const & fences, VkBool32 waitForAll, uint64_t timeout)
  {
    if (fences.size() > 0)
    {
      VkResult result = vkWaitForFences(logicalDevice, static_cast<uint32_t>(fences.size()), fences.data(), waitForAll, timeout);
      if (result != VK_SUCCESS)
      {
        // TODO: error, "Waiting on fence failed"
        return false;
      }
      return true;
    }
    return false;
  }

  bool ResetFences(VkDevice logicalDevice
    , std::vector<VkFence> const & fences)
  {
    if (fences.size() > 0)
    {
      VkResult result = vkResetFences(logicalDevice, static_cast<uint32_t>(fences.size()), fences.data());
      if (result != VK_SUCCESS)
      {
        // TODO: error, "Error occured when trying to reset fences"
        return false;
      }
      return result == VK_SUCCESS;
    }
    return false;
  }

  bool SubmitCommandBuffersToQueue(VkQueue queue
    , std::vector<WaitSemaphoreInfo> waitSemaphoreInfos
    , std::vector<VkCommandBuffer> commandBuffers
    , std::vector<VkSemaphore> signalSemaphores
    , VkFence fence)
  {
    std::vector<VkSemaphore> waitSemaphoreHandles;
    std::vector<VkPipelineStageFlags> waitSemaphoreStages;

    for (auto & waitSemaphoreInfo : waitSemaphoreInfos)
    {
      waitSemaphoreHandles.emplace_back(waitSemaphoreInfo.Semaphore);
      waitSemaphoreStages.emplace_back(waitSemaphoreInfo.WaitingStage);
    }

    VkSubmitInfo submitInfo = {
      VK_STRUCTURE_TYPE_SUBMIT_INFO,
      nullptr,
      static_cast<uint32_t>(waitSemaphoreInfos.size()),
      waitSemaphoreHandles.data(),
      waitSemaphoreStages.data(),
      static_cast<uint32_t>(commandBuffers.size()),
      commandBuffers.data(),
      static_cast<uint32_t>(signalSemaphores.size()),
      signalSemaphores.data()
    };

    VkResult result = vkQueueSubmit(queue, 1, &submitInfo, fence);
    if (result != VK_SUCCESS)
    {
      // TODO: error, "Error occurred during command buffer submission"
      return false;
    }
    return true;
  }

  bool SynchroniseTwoCommandBuffers(VkQueue firstQueue
    , std::vector<WaitSemaphoreInfo> firstWaitSemaphoreInfos
    , std::vector<VkCommandBuffer> firstCommandBuffers
    , std::vector<WaitSemaphoreInfo> synchronisingSemaphores
    , VkQueue secondQueue
    , std::vector<VkCommandBuffer> secondCommandBuffers
    , std::vector<VkSemaphore> secondSignalSemaphores
    , VkFence secondFence)
  {
    std::vector<VkSemaphore> firstSignalSemaphores;
    for (auto & semaphoreInfo : synchronisingSemaphores)
    {
      firstSignalSemaphores.emplace_back(semaphoreInfo.Semaphore);
    }
    if (!SubmitCommandBuffersToQueue(firstQueue, firstWaitSemaphoreInfos, firstCommandBuffers, firstSignalSemaphores, nullptr))
    {
      return false;
    }
    if (!SubmitCommandBuffersToQueue(secondQueue, synchronisingSemaphores, secondCommandBuffers, secondSignalSemaphores, secondFence))
    {
      return false;
    }
    return false;
  }

  bool CheckIfProcesingOfSubmittedCommandBuffersHasFinished(VkDevice logicalDevice
    , VkQueue queue
    , std::vector<WaitSemaphoreInfo> waitSemaphoreInfos
    , std::vector<VkCommandBuffer> commandBuffers
    , std::vector<VkSemaphore> signalSemaphore
    , VkFence fence
    , uint64_t timeout)
  {
    if (!SubmitCommandBuffersToQueue(queue, waitSemaphoreInfos, commandBuffers, signalSemaphore, fence))
    {
      return false;
    }

    return WaitForFences(logicalDevice, { fence }, VK_FALSE, timeout);
  }

  bool WaitUntilAllCommandsSubmittedToQueueAreFinished(VkQueue queue)
  {
    VkResult result = vkQueueWaitIdle(queue);
    if (result != VK_SUCCESS)
    {
      // TODO: error, "Waiting for all operations submitred to queue failed"
      return false;
    }
    return true;
  }

  bool WaitForAllSubmittedCommandsToBeFinished(VkDevice logicalDevice)
  {
    VkResult result = vkDeviceWaitIdle(logicalDevice);
    if (result != VK_SUCCESS)
    {
      // TODO: error, "Waiting on a device failed"
      return false;
    }
    return true;
  }

  void FreeCommandBuffers(VkDevice logicalDevice, VkCommandPool commandPool, std::vector<VkCommandBuffer>& commandBuffers)
  {
    if (commandBuffers.size() > 0)
    {
      vkFreeCommandBuffers(logicalDevice, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
      commandBuffers.clear();
    }
  }

  bool CreateBuffer(VkDevice logicalDevice
                  , VkDeviceSize size
                  , VkBufferUsageFlags usage
                  , VkBuffer & buffer)
  {
    VkBufferCreateInfo bufferCreateInfo = {
      VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      nullptr,
      0,
      size,
      usage,
      VK_SHARING_MODE_EXCLUSIVE,
      0,
      nullptr
    };

    VkResult result = vkCreateBuffer(logicalDevice, &bufferCreateInfo, nullptr, &buffer);
    if (result != VK_SUCCESS)
    {
      // TODO: error, "Could not create buffer"
      return false;
    }

    return true;
  }

  bool AllocateAndBindMemoryObjectToBuffer( VkPhysicalDevice physicalDevice
                                          , VkDevice logicalDevice
                                          , VkBuffer buffer
                                          , VkMemoryPropertyFlagBits memoryProperties
                                          , VkDeviceMemory & memoryObject)
  {
    VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physicalDeviceMemoryProperties);

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(logicalDevice, buffer, &memoryRequirements);

    memoryObject = nullptr;
    for (uint32_t type = 0; type < physicalDeviceMemoryProperties.memoryTypeCount; type++)
    {
      if ((memoryRequirements.memoryTypeBits & (1 << type))
      && ((physicalDeviceMemoryProperties.memoryTypes[type].propertyFlags & memoryProperties) == memoryProperties))
      {
        VkMemoryAllocateInfo bufferMemoryAllocateInfo = {
          VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
          nullptr,
          memoryRequirements.size,
          type
        };

        VkResult result = vkAllocateMemory(logicalDevice, &bufferMemoryAllocateInfo, nullptr, &memoryObject);
        if (result == VK_SUCCESS)
        {
          break;
        }
      }
    }

    if (memoryObject == nullptr)
    {
      // TODO: error, "Could not allocate memory for a buffer"
      return false;
    }

    VkResult result = vkBindBufferMemory(logicalDevice, buffer, memoryObject, 0);
    if (result != VK_SUCCESS)
    {
      // TODO: error, "Could not bind memory object to a buffer"
      return false;
    }

    return true;
  }

  void SetBufferMemoryBarrier(VkCommandBuffer commandBuffer
                            , VkPipelineStageFlags generatingStages
                            , VkPipelineStageFlags consumingStages
                            , std::vector<BufferTransition> bufferTransitions)
  {
    std::vector<VkBufferMemoryBarrier> bufferMemoryBarriers;

    for (auto & bufferTransition : bufferTransitions)
    {
      bufferMemoryBarriers.push_back(
        {
          VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
          nullptr,
          bufferTransition.currentAccess,
          bufferTransition.newAccess,
          bufferTransition.currentQueueFamily,
          bufferTransition.newQueueFamily,
          bufferTransition.buffer,
          0,
          VK_WHOLE_SIZE
        }
      );
    }

    if (bufferMemoryBarriers.size() > 0)
    {
      vkCmdPipelineBarrier(commandBuffer, generatingStages, consumingStages, 0, 0, nullptr, static_cast<uint32_t>(bufferMemoryBarriers.size()), bufferMemoryBarriers.data(), 0, nullptr);
    }
  }

  bool CreateBufferView(VkDevice logicalDevice
                      , VkBuffer buffer
                      , VkFormat format
                      , VkDeviceSize memoryOffset
                      , VkDeviceSize memoryRange
                      , VkBufferView & bufferView)
  {
    VkBufferViewCreateInfo bufferViewCreateInfo = {
      VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,
      nullptr,
      0,
      buffer,
      format,
      memoryOffset,
      memoryRange
    };

    VkResult result = vkCreateBufferView(logicalDevice, &bufferViewCreateInfo, nullptr, &bufferView);
    if (result != VK_SUCCESS)
    {
      // TODO: error, "Could not create buffer view"
      return false;
    }

    return true;
  }

  bool CreateImage( VkDevice logicalDevice
                  , VkImageType type
                  , VkFormat format
                  , VkExtent3D size
                  , uint32_t numMipmaps
                  , uint32_t numLayers
                  , VkSampleCountFlagBits samples
                  , VkImageUsageFlags usageScenarios
                  , bool cubemap
                  , VkImage & image)
  {
    VkImageCreateInfo imageCreateInfo = {
      VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      nullptr,
      cubemap ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0u,
      type,
      format,
      size,
      numMipmaps,
      cubemap ? 6 * numLayers : numLayers,
      samples,
      VK_IMAGE_TILING_OPTIMAL,
      usageScenarios,
      VK_SHARING_MODE_EXCLUSIVE,
      0,
      nullptr,
      VK_IMAGE_LAYOUT_UNDEFINED
    };

    VkResult result = vkCreateImage(logicalDevice, &imageCreateInfo, nullptr, &image);
    if (result != VK_SUCCESS)
    {
      // TODO: error, "Could not create an image"
      return false;
    }

    return true;
  }

  bool AllocateAndBindMemoryObjectToImage(VkPhysicalDevice physicalDevice
                                        , VkDevice logicalDevice
                                        , VkImage image
                                        , VkMemoryPropertyFlagBits memoryProperties
                                        , VkDeviceMemory & memoryObject)
  {
    VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physicalDeviceMemoryProperties);

    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(logicalDevice, image, &memoryRequirements);

    memoryObject = nullptr;
    for (uint32_t type = 0; type < physicalDeviceMemoryProperties.memoryTypeCount; type++)
    {
      if ((memoryRequirements.memoryTypeBits & (1 << type))
      && ((physicalDeviceMemoryProperties.memoryTypes[type].propertyFlags & memoryProperties) == memoryProperties))
      {
        VkMemoryAllocateInfo imageMemoryAllocateInfo = {
          VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
          nullptr,
          memoryRequirements.size,
          type
        };

        VkResult result = vkAllocateMemory(logicalDevice, &imageMemoryAllocateInfo, nullptr, &memoryObject);
        if (result == VK_SUCCESS)
        {
          break;
        }
      }
    }

    if (memoryObject == nullptr)
    {
      // TODO: error, "Could not allocate memory for an image"
      return false;
    }

    VkResult result = vkBindImageMemory(logicalDevice, image, memoryObject, 0);
    if (result != VK_SUCCESS)
    {
      // TODO: error, "Could not bind memory object to an image"
      return false;
    }

    return true;
  }

  void SetImageMemoryBarrier( VkCommandBuffer commandBuffer
                            , VkPipelineStageFlags generatingStages
                            , VkPipelineStageFlags consumingStages
                            , std::vector<ImageTransition> imageTransitions)
  {
    std::vector<VkImageMemoryBarrier> imageMemoryBarriers;

    for (auto & imageTransition : imageTransitions)
    {
      imageMemoryBarriers.push_back(
        {
          VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
          nullptr,
          imageTransition.currentAccess,
          imageTransition.newAccess,
          imageTransition.currentLayout,
          imageTransition.newLayout,
          imageTransition.currentQueueFamily,
          imageTransition.newQueueFamily,
          imageTransition.image,
          {
            imageTransition.aspect,
            0,
            VK_REMAINING_MIP_LEVELS,
            0,
            VK_REMAINING_ARRAY_LAYERS
          }
        }
      );
    }

    if (imageMemoryBarriers.size() > 0)
    {
      vkCmdPipelineBarrier(commandBuffer, generatingStages, consumingStages, 0, 0, nullptr, 0, nullptr, static_cast<uint32_t>(imageMemoryBarriers.size()), imageMemoryBarriers.data());
    }
  }

  bool CreateImageView( VkDevice logicalDevice
                      , VkImage image
                      , VkImageViewType viewType
                      , VkFormat format
                      , VkImageAspectFlags aspect
                      , VkImageView & imageView)
  {
    VkImageViewCreateInfo imageViewCreateInfo = {
      VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      nullptr,
      0,
      image,
      viewType,
      format,
      {
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY
      },
      {
        aspect,
        0,
        VK_REMAINING_MIP_LEVELS,
        0,
        VK_REMAINING_ARRAY_LAYERS
      }
    };

    VkResult result = vkCreateImageView(logicalDevice, &imageViewCreateInfo, nullptr, &imageView);
    if (result != VK_SUCCESS)
    {
      // TODO: error, "Could not create an image view"
      return false;
    }
    return true;
  }

  bool Create2DImageAndView(VkPhysicalDevice physicalDevice
                          , VkDevice logicalDevice
                          , VkFormat format
                          , VkExtent2D size
                          , uint32_t numMipmaps
                          , uint32_t numLayers
                          , VkSampleCountFlagBits samples
                          , VkImageUsageFlags usage
                          , VkImageAspectFlags aspect
                          , VkImage & image
                          , VkDeviceMemory & memoryObject
                          , VkImageView & imageView)
  {
    if (!CreateImage(logicalDevice, VK_IMAGE_TYPE_2D, format, { size.width, size.height, 1 }, numMipmaps, numLayers, samples, usage, false, image))
    {
      return false;
    }
    if (!AllocateAndBindMemoryObjectToImage(physicalDevice, logicalDevice, image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryObject))
    {
      return false;
    }
    if (!CreateImageView(logicalDevice, image, VK_IMAGE_VIEW_TYPE_2D, format, aspect, imageView))
    {
      return false;
    }
    return true;
  }

  bool CreateLayered2DImageWithCubemapView( VkPhysicalDevice physicalDevice
                                          , VkDevice logicalDevice
                                          , uint32_t size
                                          , uint32_t numMipmaps
                                          , VkImageUsageFlags usage
                                          , VkImageAspectFlags aspect
                                          , VkImage & image
                                          , VkDeviceMemory & memoryObject
                                          , VkImageView & imageView)
  {
    if (!CreateImage(logicalDevice, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, { size, size, 1 }, numMipmaps, 6, VK_SAMPLE_COUNT_1_BIT, usage, true, image))
    {
      return false;
    }
    if (!AllocateAndBindMemoryObjectToImage(physicalDevice, logicalDevice, image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryObject))
    {
      return false;
    }
    if (!CreateImageView(logicalDevice, image, VK_IMAGE_VIEW_TYPE_CUBE, VK_FORMAT_R8G8B8A8_UNORM, aspect, imageView))
    {
      return false;
    }
    return true;
  }

  bool MapUpdateAndUnmapHostVisibleMemory(VkDevice logicalDevice
                                        , VkDeviceMemory memoryObject
                                        , VkDeviceSize offset
                                        , VkDeviceSize dataSize
                                        , void * data
                                        , bool unmap
                                        , void ** pointer)
  {
    VkResult result;
    void * localPointer;
    result = vkMapMemory(logicalDevice, memoryObject, offset, dataSize, 0, &localPointer);
    if (result != VK_SUCCESS)
    {
      // TODO: error, "Could not map memory object"
      return false;
    }

    std::memcpy(localPointer, data, dataSize);

    std::vector<VkMappedMemoryRange> memoryRanges = {
      {
        VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
        nullptr,
        memoryObject,
        offset, 
        VK_WHOLE_SIZE
      }
    };

    vkFlushMappedMemoryRanges(logicalDevice, static_cast<uint32_t>(memoryRanges.size()), memoryRanges.data());
    if (result != VK_SUCCESS)
    {
      // TODO: error, "Could not flush mapped memory"
      return false;
    }

    if (unmap)
    {
      vkUnmapMemory(logicalDevice, memoryObject);
    }
    else if (pointer != nullptr)
    {
      *pointer = localPointer;
    }

    return true;
  }

  void CopyDataBetweenBuffers(VkCommandBuffer commandBuffer
                            , VkBuffer sourceBuffer
                            , VkBuffer destinationBuffer
                            , std::vector<VkBufferCopy> regions)
  {
    if (regions.size() > 0)
    {
      vkCmdCopyBuffer(commandBuffer, sourceBuffer, destinationBuffer, static_cast<uint32_t>(regions.size()), regions.data());
    }
  }

  void CopyDataFromBufferToImage( VkCommandBuffer commandBuffer
                                , VkBuffer sourceBuffer
                                , VkImage destinationImage
                                , VkImageLayout imageLayout
                                , std::vector<VkBufferImageCopy> regions)
  {
    if (regions.size() > 0)
    {
      vkCmdCopyBufferToImage(commandBuffer, sourceBuffer, destinationImage, imageLayout, static_cast<uint32_t>(regions.size()), regions.data());
    }
  }

  void CopyDataFromImageToBuffer( VkCommandBuffer commandBuffer
                                , VkImage sourceImage
                                , VkImageLayout imageLayout
                                , VkBuffer destinationBuffer
                                , std::vector<VkBufferImageCopy> regions)
  {
    if (regions.size() > 0)
    {
      vkCmdCopyImageToBuffer(commandBuffer, sourceImage, imageLayout, destinationBuffer, static_cast<uint32_t>(regions.size()), regions.data());
    }
  }

  bool UseStagingBufferToUpdateBufferWithDeviceLocalMemoryBound(
      VkPhysicalDevice physicalDevice
    , VkDevice logicalDevice
    , VkDeviceSize dataSize
    , void * data
    , VkBuffer destinationBuffer
    , VkDeviceSize destinationOffset
    , VkAccessFlags destinationBufferCurrentAccess
    , VkAccessFlags destinationBufferNewAccess
    , VkPipelineStageFlags destinationBufferGeneratingStages
    , VkPipelineStageFlags destinationBufferConsumingStages
    , VkQueue queue
    , VkCommandBuffer commandBuffer
    , std::vector<VkSemaphore> signalSemaphores)
  {
    VulkanHandle(VkBuffer) stagingBuffer;
    InitVulkanHandle(logicalDevice, stagingBuffer);
    if (!CreateBuffer(logicalDevice, dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, *stagingBuffer))
    {
      return false;
    }

    VulkanHandle(VkDeviceMemory) memoryObject;
    InitVulkanHandle(logicalDevice, memoryObject);
    if (!AllocateAndBindMemoryObjectToBuffer(physicalDevice, logicalDevice, *stagingBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, *memoryObject))
    {
      return false;
    }

    if (!MapUpdateAndUnmapHostVisibleMemory(logicalDevice, *memoryObject, 0, dataSize, data, true, nullptr))
    {
      return false;
    }

    if (!BeginCommandBufferRecordingOp(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr))
    {
      return false;
    }

    SetBufferMemoryBarrier(commandBuffer, destinationBufferGeneratingStages, VK_PIPELINE_STAGE_TRANSFER_BIT, { {destinationBuffer, destinationBufferCurrentAccess, VK_ACCESS_TRANSFER_WRITE_BIT, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED} });

    CopyDataBetweenBuffers(commandBuffer, *stagingBuffer, destinationBuffer, { {0, destinationOffset, dataSize} });

    SetBufferMemoryBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, destinationBufferConsumingStages, { {destinationBuffer, VK_ACCESS_TRANSFER_WRITE_BIT, destinationBufferNewAccess, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED} });

    if (!EndCommandBufferRecordingOp(commandBuffer))
    {
      return false;
    }

    VulkanHandle(VkFence) fence;
    InitVulkanHandle(logicalDevice, fence);
    if (!CreateFence(logicalDevice, false, *fence))
    {
      return false;
    }

    if (!SubmitCommandBuffersToQueue(queue, {}, { commandBuffer }, signalSemaphores, *fence))
    {
      return false;
    }

    if (!WaitForFences(logicalDevice, { *fence }, VK_FALSE, 500000000))
    {
      return false;
    }

    return true;
  }

  bool UseStagingBufferToUpdateImageWithDeviceLocalMemoryBound(
      VkPhysicalDevice physicalDevice
    , VkDevice logicalDevice
    , VkDeviceSize dataSize
    , void * data
    , VkImage destinationImage
    , VkImageSubresourceLayers destinationImageSubresource
    , VkOffset3D destinationImageOffset
    , VkExtent3D destinationImageSize
    , VkImageLayout destinationImageCurrentLayout
    , VkImageLayout destinationImageNewLayout
    , VkAccessFlags destinationImageCurrentAccess
    , VkAccessFlags destinationImageNewAccess
    , VkImageAspectFlags destinationImageAspect
    , VkPipelineStageFlags destinationImageGeneratingStages
    , VkPipelineStageFlags destinationImageConsumingStages
    , VkQueue queue
    , VkCommandBuffer commandBuffer
    , std::vector<VkSemaphore> signalSemaphores)
  {
    VulkanHandle(VkBuffer) stagingBuffer;
    InitVulkanHandle(logicalDevice, stagingBuffer);
    if (!CreateBuffer(logicalDevice, dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, *stagingBuffer))
    {
      return false;
    }

    VulkanHandle(VkDeviceMemory) memoryObject; 
    InitVulkanHandle(logicalDevice, memoryObject);
    if (!AllocateAndBindMemoryObjectToBuffer(physicalDevice, logicalDevice, *stagingBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, *memoryObject))
    {
      return false;
    }

    if (!MapUpdateAndUnmapHostVisibleMemory(logicalDevice, *memoryObject, 0, dataSize, data, true, nullptr))
    {
      return false;
    }

    if (!BeginCommandBufferRecordingOp(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr))
    {
      return false;
    }

    SetImageMemoryBarrier(commandBuffer, destinationImageGeneratingStages, VK_PIPELINE_STAGE_TRANSFER_BIT, 
      {
        {
          destinationImage, 
          destinationImageCurrentAccess,
          VK_ACCESS_TRANSFER_WRITE_BIT,
          destinationImageCurrentLayout,
          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
          VK_QUEUE_FAMILY_IGNORED,
          VK_QUEUE_FAMILY_IGNORED,
          destinationImageAspect
        }
      }
    );

    CopyDataFromBufferToImage(commandBuffer, *stagingBuffer, destinationImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      {
        {
          0,
          0,
          0,
          destinationImageSubresource,
          destinationImageOffset,
          destinationImageSize
        }
      }
    );

    SetImageMemoryBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, destinationImageConsumingStages,
      {
        {
          destinationImage,
          VK_ACCESS_TRANSFER_WRITE_BIT,
          destinationImageNewAccess,
          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
          destinationImageNewLayout,
          VK_QUEUE_FAMILY_IGNORED,
          VK_QUEUE_FAMILY_IGNORED,
          destinationImageAspect
        }
      }
    );

    if (!EndCommandBufferRecordingOp(commandBuffer))
    {
      return false;
    }

    VulkanHandle(VkFence) fence;
    InitVulkanHandle(logicalDevice, fence);
    if (!CreateFence(logicalDevice, false, *fence))
    {
      return false;
    }

    if (!SubmitCommandBuffersToQueue(queue, {}, { commandBuffer }, signalSemaphores, *fence))
    {
      return false;
    }

    if (!WaitForFences(logicalDevice, { *fence }, VK_FALSE, 500000000))
    {
      return false;
    }

    return true;
  }

  bool CreateSampler(VkDevice logicalDevice
    , VkFilter magFilter
    , VkFilter minFilter
    , VkSamplerMipmapMode mipmapMode
    , VkSamplerAddressMode uAddressMode
    , VkSamplerAddressMode vAddressMode
    , VkSamplerAddressMode wAddressMode
    , float lodBias
    , bool anisotropyEnable
    , float maxAnisotropy
    , bool compareEnable
    , VkCompareOp compareOperator
    , float minLod
    , float maxLod
    , VkBorderColor borderColor
    , bool unnormalisedCoords
    , VkSampler & sampler)
  {
    VkSamplerCreateInfo samplerCreateInfo = {
      VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      nullptr,
      0,
      magFilter,
      minFilter,
      mipmapMode,
      uAddressMode,
      vAddressMode,
      wAddressMode,
      lodBias,
      anisotropyEnable,
      maxAnisotropy,
      compareEnable,
      compareOperator,
      minLod,
      maxLod,
      borderColor,
      unnormalisedCoords
    };

    VkResult result = vkCreateSampler(logicalDevice, &samplerCreateInfo, nullptr, &sampler);
    if (result != VK_SUCCESS)
    {
      // TODO: error, "Could not create sampler"
      return false;
    }

    return true;
  }

  bool CreateSampledImage(VkPhysicalDevice physicalDevice
    , VkDevice logicalDevice
    , VkImageType type
    , VkFormat format
    , VkExtent3D size
    , uint32_t numMipmaps
    , uint32_t numLayers
    , VkImageUsageFlags usage
    , VkImageViewType viewType
    , VkImageAspectFlags aspect
    , bool linearFiltering
    , VkImage & sampledImage
    , VkDeviceMemory & memoryObject
    , VkImageView & sampledImageView)
  {
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);
    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT))
    {
      // TODO: error, "Provided format is not supported for a sampled image"
      return false;
    }

    if (linearFiltering && !(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
    {
      // TODO: error, "Provided format is not supported for a linear image filtering"
      return false;
    }

    if (!CreateImage(logicalDevice, type, format, size, numMipmaps, numLayers, VK_SAMPLE_COUNT_1_BIT, usage | VK_IMAGE_USAGE_SAMPLED_BIT, false, sampledImage))
    {
      return false;
    }

    if (!AllocateAndBindMemoryObjectToImage(physicalDevice, logicalDevice, sampledImage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryObject))
    {
      return false;
    }

    if (!CreateImageView(logicalDevice, sampledImage, viewType, format, aspect, sampledImageView))
    {
      return false;
    }

    return true;
  }

  bool CreateCombinedImageSampler(VkPhysicalDevice physicalDevice
    , VkDevice logicalDevice
    , VkImageType type
    , VkFormat format
    , VkExtent3D size
    , uint32_t numMipmaps
    , uint32_t numLayers
    , VkImageUsageFlags usage
    , VkImageViewType viewType
    , VkImageAspectFlags aspect
    , VkFilter magFilter
    , VkFilter minFilter
    , VkSamplerMipmapMode mipmapMode
    , VkSamplerAddressMode uAddressMode
    , VkSamplerAddressMode vAddressMode
    , VkSamplerAddressMode wAddressMode
    , float lodBias
    , bool anistropyEnable
    , float maxAnisotropy
    , bool compareEnable
    , VkCompareOp compareOperator
    , float minLod
    , float maxLod
    , VkBorderColor borderColor
    , bool unnormalisedCoords
    , VkSampler & sampler
    , VkImage & sampledImage
    , VkDeviceMemory & memoryObject
    , VkImageView & sampledImageView)
  {
    if (!CreateSampler(logicalDevice, magFilter, minFilter, mipmapMode, uAddressMode, vAddressMode, wAddressMode, lodBias, anistropyEnable, maxAnisotropy, compareEnable, compareOperator, minLod, maxLod, borderColor, unnormalisedCoords, sampler))
    {
      return false;
    }

    bool linearFiltering = (magFilter == VK_FILTER_LINEAR) || (minFilter == VK_FILTER_LINEAR) || (mipmapMode == VK_SAMPLER_MIPMAP_MODE_LINEAR);

    if (!CreateSampledImage(physicalDevice, logicalDevice, type, format, size, numMipmaps, numLayers, usage, viewType, aspect, linearFiltering, sampledImage, memoryObject, sampledImageView))
    {
      return false;
    }

    return true;
  }

  bool CreateStorageImage(VkPhysicalDevice physicalDevice
                        , VkDevice logicalDevice
                        , VkImageType type
                        , VkFormat format
                        , VkExtent3D size
                        , uint32_t numMipmaps
                        , uint32_t numLayers
                        , VkImageUsageFlags usage
                        , VkImageViewType viewType
                        , VkImageAspectFlags aspect
                        , bool atomicOperations
                        , VkImage & storageImage
                        , VkDeviceMemory & memoryObject
                        , VkImageView & storageImagesView)
  {
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);
    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT))
    {
      // TODO: error, "Provided format is not supported for a storage image"
      return false;
    }
    if (atomicOperations && !(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT))
    {
      // TODO: error, "Provided format is not supported for atomic operations on storage images"
      return false;
    }

    if (!CreateImage(logicalDevice, type, format, size, numMipmaps, numLayers, VK_SAMPLE_COUNT_1_BIT, usage | VK_IMAGE_USAGE_STORAGE_BIT, false, storageImage))
    {
      return false;
    }
     
    if (!AllocateAndBindMemoryObjectToImage(physicalDevice, logicalDevice, storageImage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryObject))
    {
      return false;
    }

    if (!CreateImageView(logicalDevice, storageImage, viewType, format, aspect, storageImagesView))
    {
      return false;
    }

    return true;
  }

  bool CreateUniformTexelBuffer(VkPhysicalDevice physicalDevice
                              , VkDevice logicalDevice
                              , VkFormat format
                              , VkDeviceSize size
                              , VkImageUsageFlags usage
                              , VkBuffer & uniformTexelBuffer
                              , VkDeviceMemory & memoryObject
                              , VkBufferView & uniformTexelBufferView)
  {
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);
    if (!(formatProperties.bufferFeatures & VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT))
    {
      // TODO: error, "Provided format is not supported for a uniform texel buffer"
      return false;
    }

    if (!CreateBuffer(logicalDevice, size, usage | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT, uniformTexelBuffer))
    {
      return false;
    }

    if (!AllocateAndBindMemoryObjectToBuffer(physicalDevice, logicalDevice, uniformTexelBuffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryObject))
    {
      return false;
    }

    if (!CreateBufferView(logicalDevice, uniformTexelBuffer, format, 0, VK_WHOLE_SIZE, uniformTexelBufferView))
    {
      return false;
    }

    return true;
  }

  bool CreateStorageTexelBuffer(VkPhysicalDevice physicalDevice
                              , VkDevice logicalDevice
                              , VkFormat format
                              , VkDeviceSize size
                              , VkBufferUsageFlags usage
                              , bool atomicOperations
                              , VkBuffer & storageTexelBuffer
                              , VkDeviceMemory & memoryObject
                              , VkBufferView & storageTexelBufferView)
  {
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);
    if (!(formatProperties.bufferFeatures & VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT))
    {
      // TODO: error, "Provided format is not supported for a uniform texel buffer"
      return false;
    }

    if (atomicOperations && !(formatProperties.bufferFeatures & VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT))
    {
      // TODO: error, "provided format is not supported for atomic operations on storage texel buffers"
      return false;
    }

    if (!CreateBuffer(logicalDevice, size, usage | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT, storageTexelBuffer))
    {
      return false;
    }

    if (!AllocateAndBindMemoryObjectToBuffer(physicalDevice, logicalDevice, storageTexelBuffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryObject))
    {
      return false;
    }

    if (!CreateBufferView(logicalDevice, storageTexelBuffer, format, 0, VK_WHOLE_SIZE, storageTexelBufferView))
    {
      return false;
    }

    return true;
  }

  bool CreateUniformBuffer( VkPhysicalDevice physicalDevice
                          , VkDevice logicalDevice
                          , VkDeviceSize size
                          , VkBufferUsageFlags usage
                          , VkBuffer & uniformBuffer
                          , VkDeviceMemory & memoryObject)
  {
    if (!CreateBuffer(logicalDevice, size, usage | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, uniformBuffer))
    {
      return false;
    }

    if (!AllocateAndBindMemoryObjectToBuffer(physicalDevice, logicalDevice, uniformBuffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryObject))
    {
      return false;
    }

    return true;
  }

  bool CreateStorageBuffer( VkPhysicalDevice physicalDevice
                          , VkDevice logicalDevice
                          , VkDeviceSize size
                          , VkBufferUsageFlags usage
                          , VkBuffer & storageBuffer
                          , VkDeviceMemory & memoryObject)
  {
    if (!CreateBuffer(logicalDevice, size, usage | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, storageBuffer))
    {
      return false;
    }

    if (!AllocateAndBindMemoryObjectToBuffer(physicalDevice, logicalDevice, storageBuffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryObject))
    {
      return false;
    }

    return true;
  }

  bool CreateInputAttachment( VkPhysicalDevice physicalDevice
                            , VkDevice logicalDevice
                            , VkImageType type
                            , VkFormat format
                            , VkExtent3D size
                            , VkImageUsageFlags usage
                            , VkImageViewType viewType
                            , VkImageAspectFlags aspect
                            , VkImage & inputAttachment
                            , VkDeviceMemory & memoryObject
                            , VkImageView & inputAttachmentImageView)
  {
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);
    if ((aspect & VK_IMAGE_ASPECT_COLOR_BIT)
      && !(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT))
    {
      // TODO: error, "Provided format is not supported for an input attachment"
      return false;
    }

    if ((aspect & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)) && !(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
    {
      // TODO: error, "Provided format is not supported for an input attachment"
      return false;
    }

    if (!CreateImage(logicalDevice, type, format, size, 1, 1, VK_SAMPLE_COUNT_1_BIT, usage | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, false, inputAttachment))
    {
      return false;
    }

    if (!AllocateAndBindMemoryObjectToImage(physicalDevice, logicalDevice, inputAttachment, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryObject))
    {
      return false;
    }

    if (!CreateImageView(logicalDevice, inputAttachment, viewType, format, aspect, inputAttachmentImageView))
    {
      return false;
    }

    return true;
  }

  bool CreateDescriptorSetLayout( VkDevice logicalDevice
                                , std::vector<VkDescriptorSetLayoutBinding> const & bindings
                                , VkDescriptorSetLayout & descriptorSetLayout)
  {
    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      nullptr,
      0,
      static_cast<uint32_t>(bindings.size()),
      bindings.data()
    };

    VkResult result = vkCreateDescriptorSetLayout(logicalDevice, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout);
    if (result != VK_SUCCESS)
    {
      // TODO: error, "Could not create a layout for descriptor sets"
      return false;
    }

    return true;
  }

  bool CreateDescriptorPool(VkDevice logicalDevice
                          , bool freeIndividualSets
                          , uint32_t maxSetsCount
                          , std::vector<VkDescriptorPoolSize> const & descriptorTypes
                          , VkDescriptorPool & descriptorPool)
  {
    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      nullptr,
      freeIndividualSets ? VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT : 0u,
      maxSetsCount,
      static_cast<uint32_t>(descriptorTypes.size()),
      descriptorTypes.data()
    };

    VkResult result = vkCreateDescriptorPool(logicalDevice, &descriptorPoolCreateInfo, nullptr, &descriptorPool);
    if (result != VK_SUCCESS)
    {
      // TODO: error, "Could not create a descriptor pool"
      return false;
    }

    return true;
  }

  bool AllocateDescriptorSets(VkDevice logicalDevice
                            , VkDescriptorPool descriptorPool
                            , std::vector<VkDescriptorSetLayout> const & descriptorSetLayouts
                            , std::vector<VkDescriptorSet>& descriptorSets)
  {
    if (descriptorSetLayouts.size() > 0)
    {
      VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        nullptr,
        descriptorPool,
        static_cast<uint32_t>(descriptorSetLayouts.size()),
        descriptorSetLayouts.data()
      };

      descriptorSets.resize(descriptorSetLayouts.size());

      VkResult result = vkAllocateDescriptorSets(logicalDevice, &descriptorSetAllocateInfo, descriptorSets.data());
      if (result != VK_SUCCESS)
      {
        // TODO: error, "Could not allocate descriptor sets"
        return false;
      }
      return true;
    }
    return false;
  }

  void UpdateDescriptorSets(VkDevice logicalDevice
                          , std::vector<ImageDescriptorInfo> const & imageDescriptorInfos
                          , std::vector<BufferDescriptorInfo> const & bufferDescriptorInfos
                          , std::vector<TexelBufferDescriptorInfo> const & texelBufferDescriptorInfos
                          , std::vector<CopyDescriptorInfo> const & copyDescriptorInfos)
  {
    std::vector<VkWriteDescriptorSet> writeDescriptors;
    std::vector<VkCopyDescriptorSet> copyDescriptors;

    // Image Descriptors
    for (auto & imageDescriptor : imageDescriptorInfos)
    {
      writeDescriptors.push_back(
        {
          VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          nullptr,
          imageDescriptor.targetDescriptorSet,
          imageDescriptor.targetDescriptorBinding,
          imageDescriptor.targetArrayElement,
          static_cast<uint32_t>(imageDescriptor.imageInfos.size()),
          imageDescriptor.targetDescriptorType,
          imageDescriptor.imageInfos.data(),
          nullptr,
          nullptr
        }
      );
    }

    // Buffer Descriptors
    for (auto & bufferDescriptor : bufferDescriptorInfos)
    {
      writeDescriptors.push_back(
        {
          VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          nullptr,
          bufferDescriptor.targetDescriptorSet,
          bufferDescriptor.targetDescriptorBinding,
          bufferDescriptor.targetArrayElement,
          static_cast<uint32_t>(bufferDescriptor.bufferInfos.size()),
          bufferDescriptor.targetDescriptorType,
          nullptr,
          bufferDescriptor.bufferInfos.data(),
          nullptr
        }
      );
    }

    // Texel buffer descriptors
    for (auto & texelBufferDescriptor : texelBufferDescriptorInfos)
    {
      writeDescriptors.push_back(
        {
          VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          nullptr,
          texelBufferDescriptor.targetDescriptorSet,
          texelBufferDescriptor.targetDescriptorBinding,
          texelBufferDescriptor.targetArrayElement,
          static_cast<uint32_t>(texelBufferDescriptor.texelBufferViews.size()),
          texelBufferDescriptor.targetDescriptorType,
          nullptr,
          nullptr,
          texelBufferDescriptor.texelBufferViews.data()
        }
      );
    }

    // Copy descriptors
    for (auto & copyDescriptor : copyDescriptorInfos)
    {
      copyDescriptors.push_back(
        {
          VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET,
          nullptr,
          copyDescriptor.sourceDescriptorSet,
          copyDescriptor.sourceDescriptorBinding,
          copyDescriptor.sourceArrayElement,
          copyDescriptor.targetDescriptorSet,
          copyDescriptor.targetDescriptorBinding,
          copyDescriptor.targetArrayElement,
          copyDescriptor.descriptorCount
        }
      );
    }

    vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(writeDescriptors.size()), writeDescriptors.data(), static_cast<uint32_t>(copyDescriptors.size()), copyDescriptors.data());
  }

  void BindDescriptorSets(VkCommandBuffer commandBuffer
                        , VkPipelineBindPoint pipelineType
                        , VkPipelineLayout pipelineLayout
                        , uint32_t indexForFirstSet
                        , std::vector<VkDescriptorSet> const & descriptorSets
                        , std::vector<uint32_t> const & dynamicOffsets)
  {
    vkCmdBindDescriptorSets(commandBuffer, pipelineType, pipelineLayout, indexForFirstSet, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), static_cast<uint32_t>(dynamicOffsets.size()), dynamicOffsets.data());
  }

  bool CreateDescriptorsWithTextureAndUniformBuffer(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, VkExtent3D sampledImageSize, uint32_t uniformBufferSize, VkSampler & sampler, VkImage & sampledImage, VkDeviceMemory & sampledImageMemoryObject, VkImageView & sampledImageView, VkBuffer & uniformBuffer, VkDeviceMemory & uniformBufferMemoryObject, VkDescriptorSetLayout & descriptorSetLayout, VkDescriptorPool & descriptorPool, std::vector<VkDescriptorSet>& descriptorSets)
  {
    if (!CreateCombinedImageSampler(physicalDevice, logicalDevice, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, sampledImageSize, 1, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, 0.f, false, 1.f, false, VK_COMPARE_OP_ALWAYS, 0.f, 0.f, VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK, false, sampler, sampledImage, sampledImageMemoryObject, sampledImageView))
    {
      return false;
    }

    if (!CreateUniformBuffer(physicalDevice, logicalDevice, uniformBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT, uniformBuffer, uniformBufferMemoryObject))
    {
      return false;
    }

    std::vector<VkDescriptorSetLayoutBinding> bindings = {
      {
        0,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        1,
        VK_SHADER_STAGE_FRAGMENT_BIT,
        nullptr
      },
      {
        1,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        1,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        nullptr
      }
    };

    if (!CreateDescriptorSetLayout(logicalDevice, bindings, descriptorSetLayout))
    {
      return false;
    }

    std::vector<VkDescriptorPoolSize> descriptorTypes = {
      {
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        1
      },
      {
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        1,
      }
    };

    if (!CreateDescriptorPool(logicalDevice, false, 1, descriptorTypes, descriptorPool))
    {
      return false;
    }

    if (!AllocateDescriptorSets(logicalDevice, descriptorPool, { descriptorSetLayout }, descriptorSets))
    {
      return false;
    }

    std::vector<ImageDescriptorInfo> imageDescriptorInfos = {
      {
        descriptorSets[0],
        0,
        0,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        {
          {
            sampler,
            sampledImageView,
            VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR
          }
        }
      }
    };

    std::vector<BufferDescriptorInfo> bufferDescriptorInfos = {
      {
        descriptorSets[0],
        1,
        0,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        {
          {
            uniformBuffer,
            0,
            VK_WHOLE_SIZE
          }
        }
      }
    };

    UpdateDescriptorSets(logicalDevice, imageDescriptorInfos, bufferDescriptorInfos, {}, {});
    return true;
  }

  bool FreeDescriptorSets(VkDevice logicalDevice
                        , VkDescriptorPool descriptorPool
                        , std::vector<VkDescriptorSet>& descriptorSets)
  {
    if (descriptorSets.size() > 0)
    {
      VkResult result = vkFreeDescriptorSets(logicalDevice, descriptorPool, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data());
      descriptorSets.clear();
      if (result != VK_SUCCESS)
      {
        // TODO: error, "Error occurred during freeing descriptor sets"
        return false;
      }
    }
    return true;
  }

  bool ResetDescriptorPool(VkDevice logicalDevice, VkDescriptorPool descriptorPool)
  {
    VkResult result = vkResetDescriptorPool(logicalDevice, descriptorPool, 0);
    if (result != VK_SUCCESS)
    {
      // TODO: error, "Error occurred during descriptor pool reset"
      return false;
    }
    return true;
  }

  void SpecifySubpassDescriptions(std::vector<SubpassParameters> const & subpassParameters
                                , std::vector<VkSubpassDescription>& subpassDescriptions)
  {
    subpassDescriptions.clear();

    for (auto & subpassDescription : subpassParameters)
    {
      subpassDescriptions.push_back(
        {
          0,
          subpassDescription.pipelineType,
          static_cast<uint32_t>(subpassDescription.inputAttachments.size()),
          subpassDescription.inputAttachments.data(),
          static_cast<uint32_t>(subpassDescription.colourAttachments.size()),
          subpassDescription.colourAttachments.data(),
          subpassDescription.resolveAttachments.data(),
          subpassDescription.depthStencilAttachment,
          static_cast<uint32_t>(subpassDescription.preserveAttachments.size()),
          subpassDescription.preserveAttachments.data()
        }
      );
    }
  }

  bool CreateRenderPass(VkDevice logicalDevice
                      , std::vector<VkAttachmentDescription> const & attachmentDescriptions
                      , std::vector<SubpassParameters> const & subpassParameters
                      , std::vector<VkSubpassDependency> const & subpassDependencies
                      , VkRenderPass & renderPass)
  {
    std::vector<VkSubpassDescription> subpassDescriptions;
    SpecifySubpassDescriptions(subpassParameters, subpassDescriptions);

    VkRenderPassCreateInfo renderPassCreateInfo = {
      VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      nullptr,
      0,
      static_cast<uint32_t>(attachmentDescriptions.size()),
      attachmentDescriptions.data(),
      static_cast<uint32_t>(subpassDescriptions.size()),
      subpassDescriptions.data(),
      static_cast<uint32_t>(subpassDependencies.size()),
      subpassDependencies.data()
    };

    VkResult result = vkCreateRenderPass(logicalDevice, &renderPassCreateInfo, nullptr, &renderPass);
    if (result != VK_SUCCESS)
    {
      // TODO: error, "Could not create a render pass"
      return false;
    }

    return true;
  }

  bool CreateFramebuffer( VkDevice logicalDevice
                        , VkRenderPass renderPass
                        , std::vector<VkImageView> const & attachments
                        , uint32_t width
                        , uint32_t height
                        , uint32_t layers
                        , VkFramebuffer & framebuffer)
  {
    VkFramebufferCreateInfo framebufferCreateInfo = {
      VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      nullptr,
      0,
      renderPass,
      static_cast<uint32_t>(attachments.size()),
      attachments.data(),
      width,
      height,
      layers
    };

    VkResult result = vkCreateFramebuffer(logicalDevice, &framebufferCreateInfo, nullptr, &framebuffer);
    if (result != VK_SUCCESS)
    {
      // TODO: error, "Could not create a framebuffer"
      return false;
    }

    return true;
  }

  void BeginRenderPass( VkCommandBuffer commandBuffer
                      , VkRenderPass renderPass
                      , VkFramebuffer framebuffer
                      , VkRect2D renderArea
                      , std::vector<VkClearValue> const & clearValues
                      , VkSubpassContents subpassContents)
  {
    VkRenderPassBeginInfo renderPassBeginInfo = {
      VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      nullptr,
      renderPass,
      framebuffer,
      renderArea,
      static_cast<uint32_t>(clearValues.size()),
      clearValues.data()
    };

    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, subpassContents);
  }
  void ProgressToNextSubpass( VkCommandBuffer commandBuffer
                            , VkSubpassContents subpassContents)
  {
    vkCmdNextSubpass(commandBuffer, subpassContents);
  }

  void EndRenderPass(VkCommandBuffer commandBuffer)
  {
    vkCmdEndRenderPass(commandBuffer);
  }
  
  bool CreateShaderModule(VkDevice logicalDevice
                        , std::vector<unsigned char> const & sourceCode
                        , VkShaderModule & shaderModule)
  {
    VkShaderModuleCreateInfo shaderModuleCreateInfo = {
      VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      nullptr,
      0,
      sourceCode.size(),
      reinterpret_cast<uint32_t const *>(sourceCode.data())
    };

    VkResult result = vkCreateShaderModule(logicalDevice, &shaderModuleCreateInfo, nullptr, &shaderModule);
    if (result != VK_SUCCESS)
    {
      // TODO: error, "Could not create a shader module"
      return false;
    }

    return true;
  }

  void SpecifyPipelineShaderStages( std::vector<ShaderStageParameters> const & shaderStageParams
                                  , std::vector<VkPipelineShaderStageCreateInfo>& shaderStageCreateInfos)
  {
    shaderStageCreateInfos.clear();
    for (auto & shaderStage : shaderStageParams)
    {
      shaderStageCreateInfos.push_back(
        {
          VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          nullptr,
          0,
          shaderStage.shaderStage,
          shaderStage.shaderModule,
          shaderStage.entryPointName,
          shaderStage.specialisationInfo
        }
      );
    }
  }

  void SpecifyPipelineVertexInputState( std::vector<VkVertexInputBindingDescription> const & bindingDescriptions
                                      , std::vector<VkVertexInputAttributeDescription> const & attributeDescriptions
                                      , VkPipelineVertexInputStateCreateInfo & vertexInputStateCreateInfo)
  {
    vertexInputStateCreateInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      nullptr,
      0,
      static_cast<uint32_t>(bindingDescriptions.size()),
      bindingDescriptions.data(),
      static_cast<uint32_t>(attributeDescriptions.size()),
      attributeDescriptions.data()
    };
  }

  void SpecifyPipelineInputAssemblyState( VkPrimitiveTopology topology
                                        , bool primitiveRestartEnable
                                        , VkPipelineInputAssemblyStateCreateInfo & inputAssemblyStateCreateInfo)
  {
    inputAssemblyStateCreateInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      nullptr,
      0,
      topology,
      primitiveRestartEnable
    };
  }

  void SpecifyPipelineTessellationState(uint32_t patchControlPointsCount
                                      , VkPipelineTessellationStateCreateInfo & tesselationStateCreateInfo)
  {
    tesselationStateCreateInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
      nullptr,
      0,
      patchControlPointsCount
    };
  }

  void SpecifyPipelineViewportAndScissorTestState(ViewportInfo const & viewportInfos
                                                , VkPipelineViewportStateCreateInfo & viewportStateCreateInfo)
  {
    uint32_t viewportCount = static_cast<uint32_t>(viewportInfos.viewports.size());
    uint32_t scissorCount = static_cast<uint32_t>(viewportInfos.scisscors.size());
    viewportStateCreateInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      nullptr,
      0,
      viewportCount,
      viewportInfos.viewports.data(),
      scissorCount,
      viewportInfos.scisscors.data()
    };
  }

  void SpecifyPipelineRasterisationState( bool depthClampEnable
                                        , bool rasteriserDiscardEnable
                                        , VkPolygonMode polygonMode
                                        , VkCullModeFlags cullingMode
                                        , VkFrontFace frontFace
                                        , bool depthBiasEnable
                                        , float depthBiasConstantFactor
                                        , float depthBiasClamp
                                        , float depthBiasSlopeFactor
                                        , float lineWidth
                                        , VkPipelineRasterizationStateCreateInfo & rasterisationStateCreateInfo)
  {
    rasterisationStateCreateInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      nullptr,
      0,
      depthClampEnable,
      rasteriserDiscardEnable,
      polygonMode,
      cullingMode,
      frontFace,
      depthBiasEnable,
      depthBiasConstantFactor,
      depthBiasClamp,
      depthBiasSlopeFactor,
      lineWidth
    };
  }

  void SpecifyPipelineMultisampleState( VkSampleCountFlagBits sampleCount
                                      , bool perSampleShadingEnable
                                      , float minSampleShading
                                      , VkSampleMask const * sampleMask
                                      , bool alphaToCoverageEnable
                                      , bool alphaToOneEnable
                                      , VkPipelineMultisampleStateCreateInfo & multisampleStateCreateInfo)
  {
    multisampleStateCreateInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      nullptr,
      0,
      sampleCount,
      perSampleShadingEnable,
      minSampleShading,
      sampleMask,
      alphaToCoverageEnable,
      alphaToOneEnable
    };
  }

  void SpecifyPipelineDepthAndStencilState( bool depthTestEnable
                                          , bool depthWriteEnable
                                          , VkCompareOp depthCompareOp
                                          , bool depthBoundsTestEnable
                                          , float minDepthBounds
                                          , float maxDepthBounds
                                          , bool stencilTestEnable
                                          , VkStencilOpState frontStencilTestParameters
                                          , VkStencilOpState backStencilTestParameters
                                          , VkPipelineDepthStencilStateCreateInfo & depthAndStencilStateCreateInfo)
  {
    depthAndStencilStateCreateInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      nullptr,
      0,
      depthTestEnable,
      depthWriteEnable,
      depthCompareOp,
      depthBoundsTestEnable,
      stencilTestEnable,
      frontStencilTestParameters,
      backStencilTestParameters,
      minDepthBounds,
      maxDepthBounds
    };
  }

  void SpecifyPipelineBlendState( bool logicOpEnable
                                , VkLogicOp logicOp
                                , std::vector<VkPipelineColorBlendAttachmentState> const & attachmentBlendStates
                                , std::array<float, 4> const & blendConstants
                                , VkPipelineColorBlendStateCreateInfo & blendStateCreateInfo)
  {
    blendStateCreateInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      nullptr,
      0,
      logicOpEnable,
      logicOp,
      static_cast<uint32_t>(attachmentBlendStates.size()),
      attachmentBlendStates.data(),
      {
        blendConstants[0],
        blendConstants[1],
        blendConstants[2],
        blendConstants[3]
      }
    };
  }

  void SpecifyPipelineDynamicStates(std::vector<VkDynamicState> const & dynamicStates
                                  , VkPipelineDynamicStateCreateInfo & dynamicStateCreateInfo)
  {
    dynamicStateCreateInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      nullptr,
      0,
      static_cast<uint32_t>(dynamicStates.size()),
      dynamicStates.data()
    };
  }

  bool CreatePipelineLayout(VkDevice logicalDevice
                          , std::vector<VkDescriptorSetLayout> const & descriptorSetLayouts
                          , std::vector<VkPushConstantRange> const & pushConstantRanges
                          , VkPipelineLayout & pipelineLayout)
  {
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      nullptr,
      0,
      static_cast<uint32_t>(descriptorSetLayouts.size()),
      descriptorSetLayouts.data(),
      static_cast<uint32_t>(pushConstantRanges.size()),
      pushConstantRanges.data()
    };

    VkResult result = vkCreatePipelineLayout(logicalDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout);

    if (result != VK_SUCCESS)
    {
      // TODO: error, "Could not create pipeline layout"
      return false;
    }

    return true;
  }
  void SpecifyGraphicsPipelineCreationParameters( VkPipelineCreateFlags additionalOptions
                                                , std::vector<VkPipelineShaderStageCreateInfo> const & shaderStageCreateInfos
                                                , VkPipelineVertexInputStateCreateInfo const & vertexInputStateCreateInfo
                                                , VkPipelineInputAssemblyStateCreateInfo const & inputAssemblyStateCreateInfo
                                                , VkPipelineTessellationStateCreateInfo const * tessellationStateCreateInfo
                                                , VkPipelineViewportStateCreateInfo const * viewportStateCreateInfo
                                                , VkPipelineRasterizationStateCreateInfo const & rasterisationStateCreateInfo
                                                , VkPipelineMultisampleStateCreateInfo const * multisampleStateCreateInfo
                                                , VkPipelineDepthStencilStateCreateInfo const * depthAndStencilStateCreateInfo
                                                , VkPipelineColorBlendStateCreateInfo const * blendStateCreateInfo
                                                , VkPipelineDynamicStateCreateInfo const * dynamicStateCreateInfo
                                                , VkPipelineLayout pipelineLayout
                                                , VkRenderPass renderPass
                                                , uint32_t subpass
                                                , VkPipeline basePipelineHandle
                                                , int32_t basePipelineIndex
                                                , VkGraphicsPipelineCreateInfo & graphicsPipelineCreateInfo)
  {
    graphicsPipelineCreateInfo = {
      VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      nullptr,
      additionalOptions,
      static_cast<uint32_t>(shaderStageCreateInfos.size()),
      shaderStageCreateInfos.data(),
      &vertexInputStateCreateInfo,
      &inputAssemblyStateCreateInfo,
      tessellationStateCreateInfo,
      viewportStateCreateInfo,
      &rasterisationStateCreateInfo,
      multisampleStateCreateInfo,
      depthAndStencilStateCreateInfo,
      blendStateCreateInfo,
      dynamicStateCreateInfo,
      pipelineLayout,
      renderPass,
      subpass,
      basePipelineHandle,
      basePipelineIndex
    };
  }

  bool CreatePipelineCacheObject( VkDevice logicalDevice
                                , std::vector<unsigned char> const & cacheData
                                , VkPipelineCache & pipelineCache)
  {
    VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
      nullptr,
      0,
      static_cast<uint32_t>(cacheData.size()),
      cacheData.data()
    };

    VkResult result = vkCreatePipelineCache(logicalDevice, &pipelineCacheCreateInfo, nullptr, &pipelineCache);
    if (result != VK_SUCCESS)
    {
      // TODO: error, "Could not create pipeline cache"
      return false;
    }

    return true;
  }

  bool RetrieveDataFromPipelineCache( VkDevice logicalDevice
                                    , VkPipelineCache pipelineCache
                                    , std::vector<unsigned char>& pipelineCacheData)
  {
    size_t dataSize = 0;
    VkResult result = VK_SUCCESS;

    result = vkGetPipelineCacheData(logicalDevice, pipelineCache, &dataSize, nullptr);
    if ((result != VK_SUCCESS) || (dataSize == 0))
    {
      // TODO: error, "Could not get the size of the pipeline cache"
      return false;
    }
    pipelineCacheData.resize(dataSize);

    result = vkGetPipelineCacheData(logicalDevice, pipelineCache, &dataSize, pipelineCacheData.data());
    if ((result != VK_SUCCESS) || (dataSize == 0))
    {
      // TODO: error, "Could not acquire pipeline cache data"
      return false;
    }

    return true;
  }

  bool MergeMultiplePipelineCacheObjects( VkDevice logicalDevice
                                        , VkPipelineCache targetPipelineCache
                                        , std::vector<VkPipelineCache> const & sourcePipelineCaches)
  {
    if (sourcePipelineCaches.size() > 0)
    {
      VkResult result = vkMergePipelineCaches(logicalDevice, targetPipelineCache, static_cast<uint32_t>(sourcePipelineCaches.size()), sourcePipelineCaches.data());
      if (result != VK_SUCCESS)
      {
        // TODO: error, "Could not merge pipeline cache objects"
        return false;
      }
      return true;
    }
    return false;
  }

  bool CreateGraphicsPipelines( VkDevice logicalDevice
                              , std::vector<VkGraphicsPipelineCreateInfo> const & graphicsPipelineCreateInfos
                              , VkPipelineCache pipelineCache
                              , std::vector<VkPipeline>& graphicsPipelines)
  {
    if (graphicsPipelineCreateInfos.size() > 0)
    {
      graphicsPipelines.resize(graphicsPipelineCreateInfos.size());
      VkResult result = vkCreateGraphicsPipelines(logicalDevice, pipelineCache, static_cast<uint32_t>(graphicsPipelineCreateInfos.size()), graphicsPipelineCreateInfos.data(), nullptr, graphicsPipelines.data());
      if (result != VK_SUCCESS)
      {
        // TODO: error, "Could not create a graphics pipeline"
        return false;
      }
      return true;
    }
    return false;
  }

  bool CreateComputePipeline( VkDevice logicalDevice
                            , VkPipelineCreateFlags additionalOptions
                            , VkPipelineShaderStageCreateInfo const & computeShaderStage
                            , VkPipelineLayout pipelineLayout
                            , VkPipeline basePipelineHandle
                            , VkPipelineCache pipelineCache
                            , VkPipeline & computePipeline)
  {
    VkComputePipelineCreateInfo computePipelineCreateInfo = {
      VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
      nullptr,
      additionalOptions,
      computeShaderStage,
      pipelineLayout,
      basePipelineHandle,
      -1
    };

    VkResult result = vkCreateComputePipelines(logicalDevice, pipelineCache, 1, &computePipelineCreateInfo, nullptr, &computePipeline);
    if (result != VK_SUCCESS)
    {
      // TODO: error, "Could not create compute pipeline"
      return false;
    }

    return true;
  }

  void BindPipelineObject(VkCommandBuffer commandBuffer
                        , VkPipelineBindPoint pipelineType
                        , VkPipeline pipeline)
  {
    vkCmdBindPipeline(commandBuffer, pipelineType, pipeline);
  }

  void ClearColorImage( VkCommandBuffer commandBuffer
                      , VkImage image
                      , VkImageLayout imageLayout
                      , std::vector<VkImageSubresourceRange> const & imageSubresourceRanges
                      , VkClearColorValue & clearColor)
  {
    vkCmdClearColorImage(commandBuffer, image, imageLayout, &clearColor, static_cast<uint32_t>(imageSubresourceRanges.size()), imageSubresourceRanges.data());
  }

  void ClearDepthStencilImage(VkCommandBuffer commandBuffer
                            , VkImage image
                            , VkImageLayout imageLayout
                            , std::vector<VkImageSubresourceRange> const & imageSubresourceRanges
                            , VkClearDepthStencilValue & clearValue)
  {
    vkCmdClearDepthStencilImage(commandBuffer, image, imageLayout, &clearValue, static_cast<uint32_t>(imageSubresourceRanges.size()), imageSubresourceRanges.data());
  }

  void ClearRenderPassAttachments(VkCommandBuffer commandBuffer
                                , std::vector<VkClearAttachment> const & attachments
                                , std::vector<VkClearRect> const & rects)
  {
    vkCmdClearAttachments(commandBuffer, static_cast<uint32_t>(attachments.size()), attachments.data(), static_cast<uint32_t>(rects.size()), rects.data());
  }

  void BindVertexBuffers( VkCommandBuffer commandBuffer
                        , uint32_t firstBinding
                        , std::vector<VertexBufferParameters> const & bufferParameters)
  {
    if (bufferParameters.size() > 0)
    {
      std::vector<VkBuffer> buffers;
      std::vector<VkDeviceSize> offsets;
      for (auto & bufferParameter : bufferParameters)
      {
        buffers.push_back(bufferParameter.buffer);
        offsets.push_back(bufferParameter.memoryOffset);
      }
      vkCmdBindVertexBuffers(commandBuffer, firstBinding, static_cast<uint32_t>(bufferParameters.size()), buffers.data(), offsets.data());
    }
  }

  void BindIndexBuffer( VkCommandBuffer commandBuffer
                      , VkBuffer buffer
                      , VkDeviceSize memoryOffset
                      , VkIndexType indexType)
  {
    vkCmdBindIndexBuffer(commandBuffer, buffer, memoryOffset, indexType);
  }

  void ProvideDataToShadersThroughPushConstants(VkCommandBuffer commandBuffer
                                              , VkPipelineLayout pipelineLayout
                                              , VkShaderStageFlags pipelineStages
                                              , uint32_t offset
                                              , uint32_t size
                                              , void * data)
  {
    vkCmdPushConstants(commandBuffer, pipelineLayout, pipelineStages, offset, size, data);
  }

  void SetViewportStateDynamically( VkCommandBuffer commandBuffer
                                  , uint32_t firstViewport
                                  , std::vector<VkViewport> const & viewports)
  {
    vkCmdSetViewport(commandBuffer, firstViewport, static_cast<uint32_t>(viewports.size()), viewports.data());
  }

  void SetScissorStateDynamically(VkCommandBuffer commandBuffer
                                , uint32_t firstScissor
                                , std::vector<VkRect2D> const & scissors)
  {
    vkCmdSetScissor(commandBuffer, firstScissor, static_cast<uint32_t>(scissors.size()), scissors.data());
  }

  void SetLineWidthStateDynamically(VkCommandBuffer commandBuffer
                                  , float lineWidth)
  {
    vkCmdSetLineWidth(commandBuffer, lineWidth);
  }

  void SetDepthBiasStateDynamically(VkCommandBuffer commandBuffer
                                  , float constantFactor
                                  , float clampValue
                                  , float slopeFactor)
  {
    vkCmdSetDepthBias(commandBuffer, constantFactor, clampValue, slopeFactor);
  }

  void SetBlendConstantsStateDynamically( VkCommandBuffer commandBuffer
                                        , std::array<float, 4> const & blendConstants)
  {
    vkCmdSetBlendConstants(commandBuffer, blendConstants.data());
  }

  void DrawGeometry(VkCommandBuffer commandBuffer
                  , uint32_t vertexCount
                  , uint32_t instanceCount
                  , uint32_t firstVertex
                  , uint32_t firstInstance)
  {
    vkCmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
  }

  void DrawIndexedGeometry( VkCommandBuffer commandBuffer
                          , uint32_t indexCount
                          , uint32_t instanceCount
                          , uint32_t firstIndex
                          , uint32_t vertexOffset
                          , uint32_t firstInstance)
  {
    vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
  }

  void DispatchComputeWork( VkCommandBuffer commandBuffer
                          , uint32_t xSize
                          , uint32_t ySize
                          , uint32_t zSize)
  {
    vkCmdDispatch(commandBuffer, xSize, ySize, zSize);
  }

  bool GetBinaryFileContents( std::string const & filename
                            , std::vector<unsigned char>& contents)
  {
    contents.clear();

    std::ifstream file(filename, std::ios::binary);
    if (file.fail())
    {
      std::cout << "Could not open '" << filename << "' file." << std::endl;
      return false;
    }

    std::streampos begin;
    std::streampos end;
    begin = file.tellg();
    file.seekg(0, std::ios::end);
    end = file.tellg();

    if ((end - begin) == 0)
    {
      std::cout << "The '" << filename << "' file is empty." << std::endl;
      return false;
    }

    contents.resize(static_cast<size_t>(end - begin));
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(contents.data()), end - begin);
    file.close();

    return true;
  }

  bool CreateFramebuffersForFrameResources(VkDevice logicalDevice
    , VkRenderPass renderPass
    , SwapchainParameters & swapchain
    , std::vector<FrameResources> & frameResources)
  {
    uint32_t imgIndex = 0;
    for (auto & frameResource : frameResources)
    {
      // Destroy any pre-existing framebuffer
      if (frameResource.framebuffer) DestroyFramebuffer(logicalDevice, *frameResource.framebuffer);

      std::vector<VkImageView> attachments = { swapchain.imageViewsRaw[imgIndex] };
      if (frameResource.depthAttachment) attachments.push_back(*frameResource.depthAttachment);

      InitVulkanHandle(logicalDevice, frameResource.framebuffer);
      if (!CreateFramebuffer( logicalDevice
                            , renderPass
                            , attachments
                            , swapchain.size.width
                            , swapchain.size.height
                            , 1
                            , *frameResource.framebuffer)
          )
      {
        return false; // You done broke it
      }
      imgIndex++;
    }
    return true;
  }

  bool PrepareSingleFrameOfAnimation(VkDevice logicalDevice
    , VkQueue graphicsQueue
    , VkQueue presentQueue
    , VkSwapchainKHR swapchain
    , std::vector<WaitSemaphoreInfo> const & waitInfos
    , VkSemaphore imageAcquiredSemaphore
    , VkSemaphore readyToPresentSemaphore
    , VkFence finishedDrawingFence
    , std::function<bool(VkCommandBuffer, uint32_t, VkFramebuffer)> recordCommandBuffer
    , VkCommandBuffer commandBuffer
    , VulkanHandle(VkFramebuffer) & framebuffer)
  {
    uint32_t imageIndex;
    if (!AcquireSwapchainImage(logicalDevice, swapchain, imageAcquiredSemaphore, VK_NULL_HANDLE, imageIndex))
    {
      return false;
    }

    if (!framebuffer) return false; // Yo, create your framebuffers first, we ain't doing that per-frame leak stuff anymore

    if (!recordCommandBuffer(commandBuffer, imageIndex, *framebuffer))
    {
      return false;
    }

    std::vector<WaitSemaphoreInfo> waitSemaphoreInfos = waitInfos;
    waitSemaphoreInfos.push_back(
      {
        imageAcquiredSemaphore,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
      }
    );
    if (!SubmitCommandBuffersToQueue( graphicsQueue
                                    , waitSemaphoreInfos
                                    , { commandBuffer }
                                    , { readyToPresentSemaphore }
                                    , finishedDrawingFence)
      )
    {
      return false;
    }

    PresentInfo presentInfo = {
      swapchain,
      imageIndex
    };

    if (!PresentImage(presentQueue, { readyToPresentSemaphore }, { presentInfo }))
    {
      return false;
    }

    return true;
  }

  bool RenderWithFrameResources(VkDevice logicalDevice
    , VkQueue graphicsQueue
    , VkQueue presentQueue
    , VkSwapchainKHR swapchain
    , std::vector<WaitSemaphoreInfo> const & waitInfos
    , std::function<bool(VkCommandBuffer, uint32_t, VkFramebuffer)> recordCommandBuffer
    , std::vector<FrameResources>& frameResources
    , uint32_t & nextFrameIndex)
  {
    static uint32_t frameIndex = 0;
    FrameResources & currentFrame = frameResources[frameIndex];

    if (!WaitForFences(logicalDevice, { *currentFrame.drawingFinishedFence }, false, std::numeric_limits<uint64_t>::max()))
    {
      return false;
    }

    if (!ResetFences(logicalDevice, { *currentFrame.drawingFinishedFence }))
    {
      return false;
    }

    if (!PrepareSingleFrameOfAnimation( logicalDevice
                                      , graphicsQueue
                                      , presentQueue
                                      , swapchain
                                      , waitInfos
                                      , *currentFrame.imageAcquiredSemaphore
                                      , *currentFrame.readyToPresentSemaphore
                                      , *currentFrame.drawingFinishedFence
                                      , recordCommandBuffer
                                      , currentFrame.commandBuffer
                                      , currentFrame.framebuffer)
      )
    {
      return false;
    }

    frameIndex = (frameIndex + 1) % frameResources.size();
    nextFrameIndex = frameIndex;

    return true;
  }

  void ExecuteSecondaryCommandBuffers(VkCommandBuffer commandBuffer
                                    , std::vector<VkCommandBuffer> const & secondaryCommandBuffers)
  {
    if (secondaryCommandBuffers.size() > 0)
    {
      vkCmdExecuteCommands(commandBuffer, static_cast<uint32_t>(secondaryCommandBuffers.size()), secondaryCommandBuffers.data());
    }
  }

  bool RecordAndsubmitCommandBuffersConcurrently( std::vector<CommandBufferRecordingParameters> const & recordingOperations
                                                , VkQueue queue
                                                , std::vector<WaitSemaphoreInfo> waitSemaphoreInfos
                                                , std::vector<VkSemaphore> signalSemaphores
                                                , VkFence fence
                                                , tf::Taskflow * const taskflow)
  {
    // Add recording ops as tasks
    for (auto op : recordingOperations)
    {
      taskflow->emplace([&]() { 
        op.recordingFunction(op.commandBuffer); 
      });
    }

    // Dispatch tasks
    auto dispatch = taskflow->dispatch();

    // Construct command buffer vector for submission
    std::vector<VkCommandBuffer> commandBuffers(recordingOperations.size());
    for (size_t i = 0; i < recordingOperations.size(); ++i)
    {
      commandBuffers[i] = recordingOperations[i].commandBuffer;
    }

    // Wait for tasks to finish
    dispatch.get();
    
    if (!SubmitCommandBuffersToQueue(queue, waitSemaphoreInfos, commandBuffers, signalSemaphores, fence))
    {
      return false;
    }
    return true;
  }

  void DestroyLogicalDevice(VkDevice & logicalDevice)
  {
    if (logicalDevice != VK_NULL_HANDLE)
    {
      vkDestroyDevice(logicalDevice, nullptr);
      logicalDevice = VK_NULL_HANDLE;
    }
  }

  void DestroyVulkanInstance(VkInstance & instance)
  {
    if (instance != VK_NULL_HANDLE)
    {
      vkDestroyInstance(instance, nullptr);
      instance = VK_NULL_HANDLE;
    }
  }
  void DestroySwapchain(VkDevice logicalDevice, VkSwapchainKHR & swapchain)
  {
    if (swapchain != VK_NULL_HANDLE)
    {
      vkDestroySwapchainKHR(logicalDevice, swapchain, nullptr);
      swapchain = VK_NULL_HANDLE;
    }
  }

  void DestroyPresentationSurface(VkInstance instance, VkSurfaceKHR & presentationSurface)
  {
    if (presentationSurface != VK_NULL_HANDLE)
    {
      vkDestroySurfaceKHR(instance, presentationSurface, nullptr);
      presentationSurface = VK_NULL_HANDLE;
    }
  }
  void DestroyCommandPool(VkDevice logicalDevice, VkCommandPool & commandPool)
  {
    if (commandPool != VK_NULL_HANDLE)
    {
      vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
      commandPool = VK_NULL_HANDLE;
    }
  }

  void DestroySemaphore(VkDevice logicalDevice, VkSemaphore & semaphore)
  {
    if (semaphore != VK_NULL_HANDLE)
    {
      vkDestroySemaphore(logicalDevice, semaphore, nullptr);
      semaphore = VK_NULL_HANDLE;
    }
  }

  void DestroyFence(VkDevice logicalDevice, VkFence & fence)
  {
    if (fence != VK_NULL_HANDLE)
    {
      vkDestroyFence(logicalDevice, fence, nullptr);
      fence = VK_NULL_HANDLE;
    }
  }

  void DestroyBuffer(VkDevice logicalDevice, VkBuffer & buffer)
  {
    if (buffer != VK_NULL_HANDLE)
    {
      vkDestroyBuffer(logicalDevice, buffer, nullptr);
      buffer = VK_NULL_HANDLE;
    }
  }

  void FreeMemoryObject(VkDevice logicalDevice, VkDeviceMemory & memoryObject)
  {
    if (memoryObject != VK_NULL_HANDLE)
    {
      vkFreeMemory(logicalDevice, memoryObject, nullptr);
      memoryObject = VK_NULL_HANDLE;
    }
  }

  void DestroyBufferView(VkDevice logicalDevice, VkBufferView & bufferView)
  {
    if (bufferView != VK_NULL_HANDLE)
    {
      vkDestroyBufferView(logicalDevice, bufferView, nullptr);
      bufferView = VK_NULL_HANDLE;
    }
  }

  void DestroyImage(VkDevice logicalDevice, VkImage & image)
  {
    if (image != VK_NULL_HANDLE)
    {
      vkDestroyImage(logicalDevice, image, nullptr);
      image = VK_NULL_HANDLE;
    }
  }

  void DestroyImageView(VkDevice logicalDevice, VkImageView & imageView)
  {
    if (imageView != VK_NULL_HANDLE)
    {
      vkDestroyImageView(logicalDevice, imageView, nullptr);
      imageView = VK_NULL_HANDLE;
    }
  }

  void DestroySampler(VkDevice logicalDevice, VkSampler & sampler)
  {
    if (sampler != VK_NULL_HANDLE)
    {
      vkDestroySampler(logicalDevice, sampler, nullptr);
      sampler = VK_NULL_HANDLE;
    }
  }

  void DestroyDescriptorSetLayout(VkDevice logicalDevice, VkDescriptorSetLayout & descriptorSetLayout)
  {
    if (descriptorSetLayout != VK_NULL_HANDLE)
    {
      vkDestroyDescriptorSetLayout(logicalDevice, descriptorSetLayout, nullptr);
      descriptorSetLayout = VK_NULL_HANDLE;
    }
  }

  void DestroyDescriptorPool(VkDevice logicalDevice, VkDescriptorPool & descriptorPool)
  {
    if (descriptorPool != VK_NULL_HANDLE)
    {
      vkDestroyDescriptorPool(logicalDevice, descriptorPool, nullptr);
      descriptorPool = VK_NULL_HANDLE;
    }
  }

  void DestroyFramebuffer(VkDevice logicalDevice
    , VkFramebuffer & framebuffer)
  {
    if (framebuffer != nullptr)
    {
      vkDestroyFramebuffer(logicalDevice, framebuffer, nullptr);
      framebuffer = nullptr;
    }
  }

  void DestroyRenderPass(VkDevice logicalDevice
    , VkRenderPass & renderPass)
  {
    if (renderPass != nullptr)
    {
      vkDestroyRenderPass(logicalDevice, renderPass, nullptr);
      renderPass = nullptr;
    }
  }

  void DestroyPipeline(VkDevice logicalDevice, VkPipeline & pipeline)
  {
    if (pipeline != VK_NULL_HANDLE)
    {
      vkDestroyPipeline(logicalDevice, pipeline, nullptr);
      pipeline = VK_NULL_HANDLE;
    }
  }

  void DestroyPipelineCache(VkDevice logicalDevice, VkPipelineCache & pipelineCache)
  {
    if (pipelineCache != VK_NULL_HANDLE)
    {
      vkDestroyPipelineCache(logicalDevice, pipelineCache, nullptr);
      pipelineCache = VK_NULL_HANDLE;
    }
  }

  void DestroyPipelineLayout(VkDevice logicalDevice, VkPipelineLayout & pipelineLayout)
  {
    if (pipelineLayout != VK_NULL_HANDLE)
    {
      vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);
      pipelineLayout = VK_NULL_HANDLE;
    }
  }

  void DestroyShaderModule(VkDevice logicalDevice, VkShaderModule & shaderModule)
  {
    if (shaderModule != VK_NULL_HANDLE)
    {
      vkDestroyShaderModule(logicalDevice, shaderModule, nullptr);
      shaderModule = VK_NULL_HANDLE;
    }
  }

  // VulkanMemoryAllocator specific functions
  bool CreateBuffer(VmaAllocator allocator
    , VkDeviceSize size
    , VkBufferUsageFlags bufferUsage
    , VkBuffer & buffer
    , VmaAllocationCreateFlags allocationFlags
    , VmaMemoryUsage memUsage
    , VkMemoryPropertyFlags memoryProperties
    , VmaPool pool
    , VmaAllocation & allocation)
  {
    VkBufferCreateInfo bufferCreateInfo = {
      VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      nullptr,
      0,
      size,
      bufferUsage,
      VK_SHARING_MODE_EXCLUSIVE,
      0,
      nullptr
    };
    
    VmaAllocationCreateInfo allocInfo = {
      allocationFlags,
      memUsage,
      memoryProperties,
      0,
      0,
      pool,

    };

    if (!vmaCreateBuffer(allocator, &bufferCreateInfo, &allocInfo, &buffer, &allocation, nullptr))
    {
      return false;
    }

    return true;
  }

  // VulkanMemoryAllocator specific functions
  bool CreateImage(VmaAllocator allocator
    , VkImageType type
    , VkFormat format
    , VkExtent3D size
    , uint32_t numMipmaps
    , uint32_t numLayers
    , VkSampleCountFlagBits samples
    , VkImageUsageFlags usageScenarios
    , bool cubemap
    , VkImage & image
    , VmaAllocationCreateFlags allocationFlags
    , VmaMemoryUsage memUsage
    , VkMemoryPropertyFlags memoryProperties
    , VmaPool pool
    , VmaAllocation & allocation)
  {
    VkImageCreateInfo imageCreateInfo = {
      VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      nullptr,
      cubemap ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0u,
      type,
      format,
      size,
      numMipmaps,
      cubemap ? 6 * numLayers : numLayers,
      samples,
      VK_IMAGE_TILING_OPTIMAL,
      usageScenarios,
      VK_SHARING_MODE_EXCLUSIVE,
      0,
      nullptr,
      VK_IMAGE_LAYOUT_UNDEFINED
    };

    VmaAllocationCreateInfo allocInfo = {
      allocationFlags,
      memUsage,
      memoryProperties,
      0,
      0,
      pool,

    };

    if (!vmaCreateImage(allocator, &imageCreateInfo, &allocInfo, &image, &allocation, nullptr))
    {
      return false;
    }

    return true;
  }

  bool Create2DImageAndView(VkDevice logicalDevice
    , VmaAllocator allocator
    , VkFormat format
    , VkExtent2D size
    , uint32_t numMipmaps
    , uint32_t numLayers
    , VkSampleCountFlagBits samples
    , VkImageUsageFlags usage
    , VkImageAspectFlags aspect
    , VkImage & image
    , VkImageView & imageView
    , VmaAllocationCreateFlags allocationFlags
    , VmaMemoryUsage memUsage
    , VkMemoryPropertyFlags memoryProperties
    , VmaPool pool
    , VmaAllocation & allocation)
  {
    if (!CreateImage(allocator, VK_IMAGE_TYPE_2D, format, { size.width, size.height, 1 }, numMipmaps, numLayers, samples, usage, false, image, allocationFlags, memUsage, memoryProperties, pool, allocation))
    {
      return false;
    }
    if (!CreateImageView(logicalDevice, image, VK_IMAGE_VIEW_TYPE_2D, format, aspect, imageView))
    {
      return false;
    }
    return true;
  }

  bool CreateLayered2DImageWithCubemapView(VkDevice logicalDevice
    , VmaAllocator allocator
    , uint32_t size
    , uint32_t numMipmaps
    , VkImageUsageFlags usage
    , VkImageAspectFlags aspect
    , VkImage & image
    , VkImageView & imageView
    , VmaAllocationCreateFlags allocationFlags
    , VmaMemoryUsage memUsage
    , VkMemoryPropertyFlags memoryProperties
    , VmaPool pool
    , VmaAllocation & allocation)
  {
    if (!CreateImage(allocator, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, { size, size, 1 }, numMipmaps, 6, VK_SAMPLE_COUNT_1_BIT, usage, true, image, allocationFlags, memUsage, memoryProperties, pool, allocation))
    {
      return false;
    }
    if (!CreateImageView(logicalDevice, image, VK_IMAGE_VIEW_TYPE_CUBE, VK_FORMAT_R8G8B8A8_UNORM, aspect, imageView))
    {
      return false;
    }
    return true;
  }

  bool MapUpdateAndUnmapHostVisibleMemory(VmaAllocator allocator
    , VmaAllocation allocation
    , VkDeviceSize dataSize
    , void * data
    , bool unmap
    , void ** pointer)
  {
    VkResult result;
    void * localPointer;
    result = vmaMapMemory(allocator, allocation, &localPointer);
    if (result != VK_SUCCESS)
    {
      // TODO: error, "Could not map memory object"
      return false;
    }

    std::memcpy(localPointer, data, dataSize);

    vmaFlushAllocation(allocator, allocation, 0, dataSize);
    if (unmap)
    {
      vmaUnmapMemory(allocator, allocation);
    }
    else if (pointer != nullptr)
    {
      *pointer = localPointer;
    }

    return true;
  }

  bool UseStagingBufferToUpdateBufferWithDeviceLocalMemoryBound(
      VkDevice logicalDevice
    , VmaAllocator allocator
    , VkDeviceSize dataSize
    , void * data
    , VkBuffer destinationBuffer
    , VkDeviceSize destinationOffset
    , VkAccessFlags destinationBufferCurrentAccess
    , VkAccessFlags destinationBufferNewAccess
    , VkPipelineStageFlags destinationBufferGeneratingStages
    , VkPipelineStageFlags destinationBufferConsumingStages
    , VkQueue queue
    , VkCommandBuffer commandBuffer
    , std::vector<VkSemaphore> signalSemaphores)
  {
    VkBuffer stagingBuffer;
    VmaAllocation allocation;
    if (!CreateBuffer(
        allocator, dataSize
      , VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingBuffer
      , VMA_ALLOCATION_CREATE_STRATEGY_FIRST_FIT_BIT
      , VMA_MEMORY_USAGE_CPU_TO_GPU
      , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
      , VK_NULL_HANDLE, allocation))
    {
      return false;
    }

    if (!MapUpdateAndUnmapHostVisibleMemory(allocator, allocation, dataSize, data, true, nullptr))
    {
      return false;
    }

    if (!BeginCommandBufferRecordingOp(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr))
    {
      return false;
    }

    SetBufferMemoryBarrier(commandBuffer, destinationBufferGeneratingStages, VK_PIPELINE_STAGE_TRANSFER_BIT, { {destinationBuffer, destinationBufferCurrentAccess, VK_ACCESS_TRANSFER_WRITE_BIT, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED} });

    CopyDataBetweenBuffers(commandBuffer, stagingBuffer, destinationBuffer, { {0, destinationOffset, dataSize} });

    SetBufferMemoryBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, destinationBufferConsumingStages, { {destinationBuffer, VK_ACCESS_TRANSFER_WRITE_BIT, destinationBufferNewAccess, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED} });

    if (!EndCommandBufferRecordingOp(commandBuffer))
    {
      return false;
    }

    VulkanHandle(VkFence) fence;
    InitVulkanHandle(logicalDevice, fence);
    if (!CreateFence(logicalDevice, false, *fence))
    {
      return false;
    }

    if (!SubmitCommandBuffersToQueue(queue, {}, { commandBuffer }, signalSemaphores, *fence))
    {
      return false;
    }

    if (!WaitForFences(logicalDevice, { *fence }, VK_FALSE, 500000000))
    {
      return false;
    }

    vmaDestroyBuffer(allocator, stagingBuffer, allocation);

    return true;
  }

  bool UseStagingBufferToUpdateImageWithDeviceLocalMemoryBound(
      VkDevice logicalDevice
    , VmaAllocator allocator
    , VkDeviceSize dataSize
    , void * data
    , VkImage destinationImage
    , VkImageSubresourceLayers destinationImageSubresource
    , VkOffset3D destinationImageOffset
    , VkExtent3D destinationImageSize
    , VkImageLayout destinationImageCurrentLayout
    , VkImageLayout destinationImageNewLayout
    , VkAccessFlags destinationImageCurrentAccess
    , VkAccessFlags destinationImageNewAccess
    , VkImageAspectFlags destinationImageAspect
    , VkPipelineStageFlags destinationImageGeneratingStages
    , VkPipelineStageFlags destinationImageConsumingStages
    , VkQueue queue
    , VkCommandBuffer commandBuffer
    , std::vector<VkSemaphore> signalSemaphores)
  {
    //VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
    VkBuffer stagingBuffer;
    VmaAllocation allocation;
    if (!CreateBuffer(
        allocator, dataSize
      , VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingBuffer
      , VMA_ALLOCATION_CREATE_STRATEGY_FIRST_FIT_BIT
      , VMA_MEMORY_USAGE_CPU_TO_GPU
      , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
      , VK_NULL_HANDLE, allocation))
    {
      return false;
    }    

    if (!MapUpdateAndUnmapHostVisibleMemory(allocator, allocation, dataSize, data, true, nullptr))
    {
      return false;
    }

    if (!BeginCommandBufferRecordingOp(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr))
    {
      return false;
    }

    SetImageMemoryBarrier(commandBuffer, destinationImageGeneratingStages, VK_PIPELINE_STAGE_TRANSFER_BIT,
      {
        {
          destinationImage,
          destinationImageCurrentAccess,
          VK_ACCESS_TRANSFER_WRITE_BIT,
          destinationImageCurrentLayout,
          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
          VK_QUEUE_FAMILY_IGNORED,
          VK_QUEUE_FAMILY_IGNORED,
          destinationImageAspect
        }
      }
    );

    CopyDataFromBufferToImage(commandBuffer, stagingBuffer, destinationImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      {
        {
          0,
          0,
          0,
          destinationImageSubresource,
          destinationImageOffset,
          destinationImageSize
        }
      }
    );

    SetImageMemoryBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, destinationImageConsumingStages,
      {
        {
          destinationImage,
          VK_ACCESS_TRANSFER_WRITE_BIT,
          destinationImageNewAccess,
          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
          destinationImageNewLayout,
          VK_QUEUE_FAMILY_IGNORED,
          VK_QUEUE_FAMILY_IGNORED,
          destinationImageAspect
        }
      }
    );

    if (!EndCommandBufferRecordingOp(commandBuffer))
    {
      return false;
    }

    VulkanHandle(VkFence) fence;
    InitVulkanHandle(logicalDevice, fence);
    if (!CreateFence(logicalDevice, false, *fence))
    {
      return false;
    }

    if (!SubmitCommandBuffersToQueue(queue, {}, { commandBuffer }, signalSemaphores, *fence))
    {
      return false;
    }

    if (!WaitForFences(logicalDevice, { *fence }, VK_FALSE, 500000000))
    {
      return false;
    }

    vmaDestroyBuffer(allocator, stagingBuffer, allocation);

    return true;
  }

  bool CreateSampledImage(VkPhysicalDevice physicalDevice
    , VkDevice logicalDevice
    , VmaAllocator allocator
    , VkImageType type
    , VkFormat format
    , VkExtent3D size
    , uint32_t numMipmaps
    , uint32_t numLayers
    , VkImageUsageFlags usage
    , VkImageViewType viewType
    , VkImageAspectFlags aspect
    , bool linearFiltering
    , VkImage & sampledImage
    , VmaMemoryUsage memUsage
    , VmaAllocation & allocation
    , VkImageView & sampledImageView)
  {
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);
    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT))
    {
      // TODO: error, "Provided format is not supported for a sampled image"
      return false;
    }

    if (linearFiltering && !(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
    {
      // TODO: error, "Provided format is not supported for linear image filtering"
      return false;
    }

    if (!CreateImage(allocator
      , type, format, size, numMipmaps, numLayers
      , VK_SAMPLE_COUNT_1_BIT
      , usage | VK_IMAGE_USAGE_SAMPLED_BIT
      , false, sampledImage
      , VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT
      , memUsage
      , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
      , VK_NULL_HANDLE, allocation))
    {
      return false;
    }

    if (!CreateImageView(logicalDevice, sampledImage, viewType, format, aspect, sampledImageView))
    {
      return false;
    }

    return true;
  }

  bool CreateCombinedImageSampler(VkPhysicalDevice physicalDevice
    , VkDevice logicalDevice
    , VmaAllocator allocator
    , VkImageType type
    , VkFormat format
    , VkExtent3D size
    , uint32_t numMipmaps
    , uint32_t numLayers
    , VkImageUsageFlags usage
    , VkImageViewType viewType
    , VkImageAspectFlags aspect
    , VkFilter magFilter
    , VkFilter minFilter
    , VkSamplerMipmapMode mipmapMode
    , VkSamplerAddressMode uAddressMode
    , VkSamplerAddressMode vAddressMode
    , VkSamplerAddressMode wAddressMode
    , float lodBias
    , bool anistropyEnable
    , float maxAnisotropy
    , bool compareEnable
    , VkCompareOp compareOperator
    , float minLod
    , float maxLod
    , VkBorderColor borderColor
    , bool unnormalisedCoords
    , VkSampler & sampler
    , VkImage & sampledImage
    , VmaMemoryUsage memUsage
    , VmaAllocation & allocation
    , VkImageView & sampledImageView)
  {
    if (!CreateSampler(logicalDevice, magFilter, minFilter, mipmapMode, uAddressMode, vAddressMode, wAddressMode, lodBias, anistropyEnable, maxAnisotropy, compareEnable, compareOperator, minLod, maxLod, borderColor, unnormalisedCoords, sampler))
    {
      return false;
    }

    bool linearFiltering = (magFilter == VK_FILTER_LINEAR) || (minFilter == VK_FILTER_LINEAR) || (mipmapMode == VK_SAMPLER_MIPMAP_MODE_LINEAR);

    if (!CreateSampledImage(physicalDevice, logicalDevice, allocator, type, format, size, numMipmaps, numLayers, usage, viewType, aspect, linearFiltering, sampledImage, memUsage, allocation, sampledImageView))
    {
      return false;
    }

    return true;
  }

  bool CreateStorageImage(VkPhysicalDevice physicalDevice
    , VkDevice logicalDevice
    , VmaAllocator allocator
    , VkImageType type
    , VkFormat format
    , VkExtent3D size
    , uint32_t numMipmaps
    , uint32_t numLayers
    , VkImageUsageFlags usage
    , VkImageViewType viewType
    , VkImageAspectFlags aspect
    , bool atomicOperations
    , VkImage & storageImage
    , VmaMemoryUsage memUsage
    , VmaAllocation & allocation
    , VkImageView & storageImagesView)
  {
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);

    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT))
    {
      // TODO: error, "Provided format is not supported for a storage image"
      return false;
    }
    if (atomicOperations && !(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT))
    {
      // TODO: error, "Provided format is not supported for atomic operations on storage images"
      return false;
    }

    if (!CreateImage(allocator, type, format, size, numMipmaps, numLayers, VK_SAMPLE_COUNT_1_BIT, usage | VK_IMAGE_USAGE_STORAGE_BIT, false, storageImage, VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT, memUsage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_NULL_HANDLE, allocation))
    {
      return false;
    }

    if (!CreateImageView(logicalDevice, storageImage, viewType, format, aspect, storageImagesView))
    {
      return false;
    }

    return true;
  }

  bool CreateUniformTexelBuffer(VkPhysicalDevice physicalDevice
    , VkDevice logicalDevice
    , VmaAllocator allocator
    , VkFormat format
    , VkDeviceSize size
    , VkImageUsageFlags usage
    , VkBuffer & uniformTexelBuffer
    , VmaMemoryUsage memUsage
    , VmaAllocation & allocation
    , VkBufferView & uniformTexelBufferView)
  {
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);
    if (!(formatProperties.bufferFeatures & VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT))
    {
      // TODO: error, "Provided format is not supported for a uniform texel buffer"
      return false;
    }

    if (!CreateBuffer(allocator, size, usage | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT, uniformTexelBuffer
      , VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT, memUsage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_NULL_HANDLE, allocation))
    {
      return false;
    }

    if (!CreateBufferView(logicalDevice, uniformTexelBuffer, format, 0, VK_WHOLE_SIZE, uniformTexelBufferView))
    {
      return false;
    }

    return true;
  }

  bool CreateStorageTexelBuffer(VkPhysicalDevice physicalDevice
    , VkDevice logicalDevice
    , VmaAllocator allocator
    , VkFormat format
    , VkDeviceSize size
    , VkBufferUsageFlags usage
    , bool atomicOperations
    , VkBuffer & storageTexelBuffer
    , VmaMemoryUsage memUsage
    , VmaAllocation & allocation
    , VkBufferView & storageTexelBufferView)
  {
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);
    if (!(formatProperties.bufferFeatures & VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT))
    {
      // TODO: error, "Provided format is not supported for a uniform texel buffer"
      return false;
    }

    if (atomicOperations && !(formatProperties.bufferFeatures & VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT))
    {
      // TODO: error, "provided format is not supported for atomic operations on storage texel buffers"
      return false;
    }

    if (!CreateBuffer(allocator, size, usage | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT, storageTexelBuffer
    , VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT, memUsage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_NULL_HANDLE, allocation))
    {
      return false;
    }

    if (!CreateBufferView(logicalDevice, storageTexelBuffer, format, 0, VK_WHOLE_SIZE, storageTexelBufferView))
    {
      return false;
    }

    return true;
  }

  bool CreateUniformBuffer(VmaAllocator allocator
    , VkDeviceSize size
    , VkBufferUsageFlags usage
    , VkBuffer & uniformBuffer
    , VmaMemoryUsage memUsage
    , VmaAllocation & allocation)
  {
    if (!CreateBuffer(allocator, size, usage | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, uniformBuffer, VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT, memUsage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_NULL_HANDLE, allocation))
    {
      return false;
    }

    return true;
  }

  bool CreateStorageBuffer(VmaAllocator allocator
    , VkDeviceSize size
    , VkBufferUsageFlags usage
    , VkBuffer & storageBuffer
    , VmaMemoryUsage memUsage
    , VmaAllocation & allocation)
  {
    if (!CreateBuffer(allocator, size, usage | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, storageBuffer, VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT, memUsage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_NULL_HANDLE, allocation))
    {
      return false;
    }

    return true;
  }

  bool CreateInputAttachment(VkPhysicalDevice physicalDevice
    , VkDevice logicalDevice
    , VmaAllocator allocator
    , VkImageType type
    , VkFormat format
    , VkExtent3D size
    , VkImageUsageFlags usage
    , VkImageViewType viewType
    , VkImageAspectFlags aspect
    , VkImage & inputAttachment
    , VmaMemoryUsage memUsage
    , VmaAllocation & allocation
    , VkImageView & inputAttachmentImageView)
  {
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);
    if ((aspect & VK_IMAGE_ASPECT_COLOR_BIT)
      && !(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT))
    {
      // TODO: error, "Provided format is not supported for an input attachment"
      return false;
    }

    if ((aspect & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)) && !(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
    {
      // TODO: error, "Provided format is not supported for an input attachment"
      return false;
    }

    if (!CreateImage(allocator, type, format, size, 1, 1, VK_SAMPLE_COUNT_1_BIT, usage | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, false, inputAttachment, VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT, memUsage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_NULL_HANDLE, allocation))
    {
      return false;
    }    

    if (!CreateImageView(logicalDevice, inputAttachment, viewType, format, aspect, inputAttachmentImageView))
    {
      return false;
    }

    return true;
  }
}
