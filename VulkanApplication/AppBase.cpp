#include "AppBase.hpp"

AppBase::AppBase()
  : vulkanLibrary(nullptr)
{

}

AppBase::~AppBase()
{

}

void AppBase::Run()
{
  // Loop
}

bool AppBase::InitVulkan(VulkanInterface::WindowParameters windowParameters)
{
  // Load Library
  try
  {
    VulkanInterface::LoadVulkanLoaderLibrary(vulkanLibrary);
  }
  catch (std::runtime_error const &e)
  {
    cleanup();
    throw UnrecoverableRuntimeException(CreateBasicExceptionMessage("Unable to load Vulkan Library"), e);
    return false;
  }

  // Get vkGetInstanceProcAddr function so we can start loading functions
  try
  {
    VulkanInterface::LoadVulkanFunctionGetter(vulkanLibrary);
  }
  catch (std::runtime_error const &e)
  {
    cleanup();
    throw UnrecoverableRuntimeException(CreateBasicExceptionMessage("Unable to load vkGetInstanceProcAddr function"), e);
    return false;
  }

  // Load the global vulkan functions so we can create a Vulkan Instance
  try
  {
    VulkanInterface::LoadGlobalVulkanFunctions();
  }
  catch (std::runtime_error const &e)
  {
    cleanup();
    throw UnrecoverableRuntimeException(CreateBasicExceptionMessage("Unable to load Global-Level Functions"), e);
    return false;
  } 

  // Create a Vulkan Instance tailored to our needs, with required extensions (and if validation layers if in debug)
  try
  {
    VulkanInterface::CreateVulkanInstance(desiredExtensions, desiredLayers, "VulkanApplication", vulkanInstance);
  }
  catch (std::runtime_error const &e)
  {
    cleanup();
    throw UnrecoverableRuntimeException(CreateBasicExceptionMessage("Unable to create Vulkan Instance"), e);
    return false;
  }

  // If we're in debug we can attach our debugCallback function now
#if defined(_DEBUG)
  try
  {
    VulkanInterface::SetupDebugCallback(vulkanInstance, debugCallback, callback);
  }
  catch (std::runtime_error const &e)
  {
    // Not really unrecoverable, but if this is failing in debug something is probably wrong
    cleanup();
    throw UnrecoverableRuntimeException(CreateBasicExceptionMessage("Unable to attach debug callback to vulkan instance"), e);
    return false;
  }
#endif

  // Load Instance-level functions
  try
  {
    VulkanInterface::LoadInstanceLevelVulkanFunctions(vulkanInstance, desiredExtensions);
  }
  catch (std::runtime_error const &e)
  {
    cleanup();
    throw UnrecoverableRuntimeException(CreateBasicExceptionMessage("Unable to load Instance-Level Functions"), e);
    return false;
  }

  // Create presentation surface
  try
  {
    VulkanInterface::CreatePresentationSurface(vulkanInstance, windowParameters, presentationSurface);
  }
  catch (std::runtime_error const&e)
  {
    cleanup();
    throw UnrecoverableRuntimeException(CreateBasicExceptionMessage("Could not create presentation surface"), e);
    return false;
  }

  // Get available physical devices
  std::vector<VkPhysicalDevice> physicalDevices;
  try
  {
    VulkanInterface::EnumerateAvailablePhysicalDevices(vulkanInstance, physicalDevices);
  }
  catch (std::runtime_error const &e)
  {
    cleanup();
    throw UnrecoverableRuntimeException(CreateBasicExceptionMessage("Unable to enumerate available physical devices"), e);
    return false;
  }
  
  for (auto & physicalDevice : physicalDevices)
  {
    // Try to find a device which supports all our desired capabilities
    try
    {
      // Make sure we have graphics on this device
      if (!VulkanInterface::SelectIndexOfQueueFamilyWithDesiredCapabilities(physicalDevice, VK_QUEUE_GRAPHICS_BIT, graphicsQueue.FamilyIndex))
      {
        continue;
      }

      // Make sure we have compute on this device
      if (!VulkanInterface::SelectIndexOfQueueFamilyWithDesiredCapabilities(physicalDevice, VK_QUEUE_COMPUTE_BIT, computeQueue.FamilyIndex))
      {
        continue;
      }

      // Make sure we have a queue family that supports presenting to our surface
      if (!VulkanInterface::SelectQueueFamilyThatSupportsPresentationToGivenSurface(physicalDevice, presentationSurface, presentQueue.FamilyIndex))
      {
        continue;
      }
    }
    catch (std::runtime_error const &e)
    {
      // Exceptions thrown within this block are only relating to Vulkan outright failing to get queue families
      // to check against, I'm assuming if this fails there's an issue with Vulkan or the Device.
      throw UnrecoverableRuntimeException(CreateBasicExceptionMessage("Failed to check queue families for available physical device"), e);
      return false;
    }

    // Construct requestedQueues vector
    std::vector<VulkanInterface::QueueInfo> requestedQueues = {
      { graphicsQueue.FamilyIndex, {1.f} }      
    };
    // Graphics Queue Family could be the same as the Compute Queue Family so check before we add it twice
    if (graphicsQueue.FamilyIndex != computeQueue.FamilyIndex) 
    {
      requestedQueues.push_back(
        { computeQueue.FamilyIndex, {1.f} }
      );
    }
    // Present Queue Family, same deal, but check against both Graphics and Compute queues
    if ((graphicsQueue.FamilyIndex != presentQueue.FamilyIndex) 
     && (computeQueue.FamilyIndex  != presentQueue.FamilyIndex))
    {
      requestedQueues.push_back(
        { presentQueue.FamilyIndex, { 1.f} }
      );
    }

    try
    {
      VulkanInterface::CreateLogicalDevice(physicalDevice, requestedQueues, desiredExtensions, desiredLayers, &desiredDeviceFeatures, vulkanDevice);
    }
    catch (std::runtime_error const &e)
    {
      std::cout << "Failed to create logical device from chosen physical device\n";
      std::cout << e.what() << std::endl;
      std::cout << "Attempting to find another device." << std::endl;
      continue;
    }
    
    // If we've reached this point we're in the clear
    vulkanPhysicalDevice = physicalDevice;
    VulkanInterface::LoadDeviceLevelVulkanFunctions(vulkanDevice, desiredExtensions);
    VulkanInterface::vkGetDeviceQueue(vulkanDevice, graphicsQueue.FamilyIndex, 0, &graphicsQueue.Handle);
    VulkanInterface::vkGetDeviceQueue(vulkanDevice, computeQueue.FamilyIndex, 0, &computeQueue.Handle);
    VulkanInterface::vkGetDeviceQueue(vulkanDevice, presentQueue.FamilyIndex, 0, &presentQueue.Handle);
    break;
  }

  if (vulkanDevice == nullptr)
  {
    cleanup();
    throw UnrecoverableRuntimeException(CreateBasicExceptionMessage("Failed to create logical device!"), "vulkanDevice == nullptr");
    return false;
  }

  return true;
}

void AppBase::Update()
{
}

void AppBase::cleanup()
{
  // We need to work backwards, destroying device-level objects before instance-level objects, and so on
  if (vulkanDevice) VulkanInterface::vkDestroyDevice(vulkanDevice, nullptr);

#if defined(_DEBUG)
  if (callback) VulkanInterface::DestroyDebugUtilsMessengerEXT(vulkanInstance, callback, nullptr);
#endif

  if (presentationSurface) VulkanInterface::vkDestroySurfaceKHR(vulkanInstance, presentationSurface, nullptr);  
  if (vulkanInstance) VulkanInterface::vkDestroyInstance(vulkanInstance, nullptr);
}
