#include "VulkanInterface.hpp"
#include <sstream>

namespace VulkanInterface
{
  // Debug Message Setup
  VkResult CreateDebugUtilsMessengerEXT(VkInstance instance
    , VkDebugUtilsMessengerCreateInfoEXT const * pCreateInfo
    , VkAllocationCallbacks * pAllocator
    , VkDebugUtilsMessengerEXT * pCallback)
  {
    auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    if (func != nullptr)
    {
      return func(instance, pCreateInfo, pAllocator, pCallback);
    }
    else
    {
      return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
  }

  void DestroyDebugUtilsMessengerEXT(VkInstance instance
    , VkDebugUtilsMessengerEXT callback
    , VkAllocationCallbacks const * pAllocator)
  {
    auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
    if (func != nullptr)
    {
      func(instance, callback, pAllocator);
    }
  }

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

    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &callback) != VK_SUCCESS)
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
}
