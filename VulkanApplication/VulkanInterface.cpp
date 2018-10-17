#include "VulkanInterface.hpp"

namespace VulkanInterface
{
  // Debug Callback Setup Functions
  //static PFN_vkCreateDebugReportCallbackEXT pfn_vkCreateDebugReportCallbackEXT;
  //VkResult vkCreateDebugReportCallbackEXT(
  //  VkInstance                                instance,
  //  const VkDebugReportCallbackCreateInfoEXT *pCreateInfo,
  //  const VkAllocationCallbacks              *pAllocator,
  //  VkDebugReportCallbackEXT                 *pCallback)
  //{
  //  return pfn_vkCreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pCallback);
  //}

  //static PFN_vkDestroyDebugReportCallbackEXT pfn_vkDestroyDebugReportCallbackEXT;
  //void vkDestroyDebugReportCallbackEXT(
  //  VkInstance                    instance,
  //  VkDebugReportCallbackEXT      callback,
  //  const VkAllocationCallbacks  *pAllocator)
  //{
  //  pfn_vkDestroyDebugReportCallbackEXT(instance, callback, pAllocator);
  //}

  //void LoadDebugCallbackFuncs(VkInstance instance)
  //{
  //  pfn_vkCreateDebugReportCallbackEXT = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT"));
  //  if (pfn_vkCreateDebugReportCallbackEXT == nullptr)
  //    throw UnrecoverableRuntimeException(CreateBasicExceptionMessage("Failed to load vkCreateDebugReportCallbackEXT"), "vkGetInstanceProcAddr");
  //  pfn_vkDestroyDebugReportCallbackEXT = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT"));
  //  if (pfn_vkCreateDebugReportCallbackEXT == nullptr)
  //    throw UnrecoverableRuntimeException(CreateBasicExceptionMessage("Failed to load vkDestroyDebugReportCallbackEXT"), "vkGetInstanceProcAddr");
  //}

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
                                      , std::vector<char const*> const & enabledExtensions)
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

  bool CreateVulkanInstance(std::vector<char const *> const & desiredExtensions
                          , std::vector<char const *> desiredLayers
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
}
