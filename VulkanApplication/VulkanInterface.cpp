#include "VulkanInterface.hpp"
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
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
      VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
      debugCallbackFunc,
      nullptr
    };

    if (vkCreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &callback) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to set up debug callback!");
      return false;
    }
    return true;
  }

  // Functions for Loading Vulkan Library & Functions
  bool LoadVulkanLoaderLibrary(LIBRARY_TYPE & library)
  {
#if defined(_WIN32)
#include <Windows.h>
    library = LoadLibrary("vulkan-1.dll");
#elif defined(__linux)
    library = dlopen("libvulkan.so.1", RTLD_NOW);
#endif
    if (library == nullptr)
    {
      throw std::runtime_error("Unable to load vulkan library, ptr returned is nullptr");
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
      throw std::runtime_error("Failed to load function " #name); \
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
      throw std::runtime_error("Failed to load global-level function " #name); \
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
      throw std::runtime_error("Failed to load instance-level function " #name); \
      return false; \
    }

    // Load extension Vulkan instance-level functions
#define INSTANCE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION( name, extension ) \
    for(auto & enabledExtension : enabledExtensions) { \
      if(std::string(enabledExtension) == std::string(extension)) { \
        name = (PFN_##name)vkGetInstanceProcAddr( instance, #name ); \
        if (name == nullptr) { \
          throw std::runtime_error("Failed to load extension instance-level function " #name); \
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
      throw std::runtime_error("Failed to load device-level function " #name); \
      return false; \
    }

    // Load extension Vulkan device-level functions
#define DEVICE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION( name, extension ) \
    for( auto & enabledExtension : enabledExtensions) { \
      if(std::string(enabledExtensions) == std::string(extension)) { \
        name = (PFN_##name)vkGetDeviceProcAddr(device, #name); \
        if(name == nullptr) { \
          throw std::runtime_error("Failed to load extension device-level function " #name); \
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
      throw std::runtime_error("Could not get the number of available instance layers");
      return false;
    }

    availableLayers.resize(availableLayersCount);
    result = vkEnumerateInstanceLayerProperties(&availableLayersCount, availableLayers.data());
    if ((result != VK_SUCCESS) || (availableLayersCount < static_cast<uint32_t>(availableLayers.size())))
    {
      throw std::runtime_error("Could not enumerate available instance layers");
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
      throw std::runtime_error("Desired instance layer not found in available layers list");
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
      throw std::runtime_error("Could not get the number of instance extensions");
      return false;
    }

    availableExtensions.resize(extensionsCount);
    result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, availableExtensions.data());
    if ((result != VK_SUCCESS) || (extensionsCount == 0))
    {
      throw std::runtime_error("Could not enumerate instance extensions");
      return false;
    }

    return true;
  }

  bool IsExtensionSupported(std::vector<VkExtensionProperties> const & availableExtensions
                          , char const * const extension)
  {
    bool found = false;
    for (auto & availableExtension : availableExtensions)
    {
      if (strstr(availableExtension.extensionName, extension))
      {
        found = true;
        break;
      }
    }
    if (!found)
    {
      throw std::runtime_error("Desired Extension not found in Available Extensions list");
    }
    return found;
  }

  // Function to create an instance
  bool CreateVulkanInstance(std::vector<char const *> const & desiredExtensions
                          , std::vector<char const *> const & desiredLayers
                          , char const * const applicationName
                          , VkInstance & instance)
  {
    std::vector<VkExtensionProperties> availableExtensions;
    try
    {
      GetAvailableInstanceExtensions(availableExtensions);
    }
    catch (std::runtime_error const &e)
    {
      throw UnrecoverableRuntimeException(CreateBasicExceptionMessage("Unable to get available extensions"), e);
    }

    // Search through availableExtensions for extensions in desiredExtensions
    for (auto & desiredExtension : desiredExtensions)
    {
      try
      {
        IsExtensionSupported(availableExtensions, desiredExtension);
      }
      catch (std::runtime_error const &e)
      {
        throw UnrecoverableRuntimeException(CreateBasicExceptionMessage("Desired extension is not supported by instance"), e);
      }
    }

    std::vector<VkLayerProperties> availableLayers;
    try
    {
      GetAvailableLayerSupport(availableLayers);
    }
    catch (std::runtime_error const &e)
    {
      throw UnrecoverableRuntimeException(CreateBasicExceptionMessage("Instance does not support desired validation layers"), e);
    }

    // Search through availableLayers for layers in desiredLayers
    for (auto & desiredLayer : desiredLayers)
    {
      try
      {
        IsLayerSupported(availableLayers, desiredLayer);
      }
      catch (std::runtime_error const &e)
      {
        throw UnrecoverableRuntimeException(CreateBasicExceptionMessage("Desired layer is not supported by instance"), e);
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
      throw std::runtime_error("Could not create Vulkan Instance");
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
      throw std::runtime_error("Could not get the number of available physical devices");
      return false;
    }

    availableDevices.resize(devicesCount);
    result = vkEnumeratePhysicalDevices(instance, &devicesCount, availableDevices.data());
    if ((result != VK_SUCCESS) || (devicesCount == 0))
    {
      throw std::runtime_error("Could not enumerate physical devices");
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
      throw std::runtime_error("Could not get the number of device extensions");
      return false;
    }

    availableExtensions.resize(extensionsCount);
    result = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionsCount, availableExtensions.data());
    if ((result != VK_SUCCESS) || (extensionsCount == 0))
    {
      throw std::runtime_error("Could not enumerate device extensions");
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
      throw std::runtime_error("Could not get the number of queue families");
      return false;
    }

    queueFamilies.resize(queueFamiliesCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamiliesCount, queueFamilies.data());
    if (queueFamiliesCount == 0)
    {
      throw std::runtime_error("Could not acquire properties of queue families");
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
      if ((queueFamilies[index].queueCount > 0) && (queueFamilies[index].queueFlags & desiredCapabilities))
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
    try
    {
      CheckAvailableQueueFamiliesAndTheirProperties(physicalDevice, queueFamilies);      
    }
    catch (std::runtime_error const &e)
    {
      throw UnrecoverableRuntimeException(CreateBasicExceptionMessage("Unable to get queue families to check for family that supports given surface"), e);
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
        throw std::runtime_error(ss.str());
        return false;
      }
    }

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    for (auto & info : queueInfos)
    {
      queueCreateInfos.push_back({
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        nullptr,
        0,
        info.FamilyIndex,
        static_cast<uint32_t>(info.Priorities.size()),
        info.Priorities.data()
        }
      );

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
        throw std::runtime_error("Could not create logical device");
        return false;
      }
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
      throw std::runtime_error("Could not create presentation surface");
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
      throw std::runtime_error("Could not get the number of supported present modes");
      return false;
    }

    std::vector<VkPresentModeKHR> presentModes(presentModesCount);
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, presentationSurface, &presentModesCount, presentModes.data());
    if ((result != VK_SUCCESS) || (presentModesCount == 0))
    {
      throw std::runtime_error("Could not enumerate present modes");
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

    throw std::runtime_error("VK_PRESENT_MODE_FIFO_KHR is not supported! Run for the hills!");
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

    return false;
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

  bool ResetFences(VkDevice logicalDevice, std::vector<VkFence> const & fences)
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

  bool SubmitCommandBuffersToQueue(VkQueue queue, std::vector<WaitSemaphoreInfo> waitSemaphoreInfos, std::vector<VkCommandBuffer> commandBuffers, std::vector<VkSemaphore> signalSemaphores, VkFence fence)
  {
    std::vector<VkSemaphore> waitSemaphoreHandles;
    std::vector<VkPipelineStageFlags> waitSemaphoreStages;

    for (auto & waitSemaphoreInfo : waitSemaphoreInfos)
    {
      waitSemaphoreHandles.emplace_back(waitSemaphoreInfo.Semaphore);
      waitSemaphoreHandles.emplace_back(waitSemaphoreInfo.WaitingStage);
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

  bool SynchroniseTwoCommandBuffers(VkQueue firstQueue, std::vector<WaitSemaphoreInfo> firstWaitSemaphoreInfos, std::vector<VkCommandBuffer> firstCommandBuffers, std::vector<WaitSemaphoreInfo> synchronisingSemaphores, VkQueue secondQueue, std::vector<VkCommandBuffer> secondCommandBuffers, std::vector<VkSemaphore> secondSignalSemaphores, VkFence secondFence)
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

  bool CheckIfProcesingOfSubmittedCommandBuffersHasFinished(VkDevice logicalDevice, VkQueue queue, std::vector<WaitSemaphoreInfo> waitSemaphoreInfos, std::vector<VkCommandBuffer> commandBuffers, std::vector<VkSemaphore> signalSemaphore, VkFence fence, uint64_t timeout, VkResult & waitStatus)
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

}
