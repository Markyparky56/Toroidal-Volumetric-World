#include "AppBase.hpp"

AppBase::AppBase()
  : vulkanLibrary(nullptr)
  , ready(false)
  , callback()
{

}

AppBase::~AppBase()
{

}

bool AppBase::initVulkan(VulkanInterface::WindowParameters windowParameters, VkImageUsageFlags swapchainImageUsage, bool   useDepth, VkImageUsageFlags depthAttachmentUsage)
{
  // Load Library
  if (!VulkanInterface::LoadVulkanLoaderLibrary(vulkanLibrary))
  {
    return false;
  }

  // Get vkGetInstanceProcAddr function so we can start loading functions
  if (!VulkanInterface::LoadVulkanFunctionGetter(vulkanLibrary))
  {
    return false;
  }

  // Load the global vulkan functions so we can create a Vulkan Instance
  if (!VulkanInterface::LoadGlobalVulkanFunctions())
  {
    return false;
  }

  // Create a Vulkan Instance tailored to our needs, with required extensions (and validation layers if in debug)
  VulkanInterface::InitVulkanHandle(vulkanInstance);
  if (!VulkanInterface::CreateVulkanInstance(desiredInstanceExtensions, desiredLayers, "VulkanApplication", *vulkanInstance))
  {
    return false;
  }

  // Load Instance-level functions
  if(!VulkanInterface::LoadInstanceLevelVulkanFunctions(*vulkanInstance, desiredInstanceExtensions))
  {
    return false;
  }

  // If we're in debug we can attach our debugCallback function now
#if defined(_DEBUG)
  if (!VulkanInterface::SetupDebugCallback(*vulkanInstance, debugCallback, callback))
  {
    return false;
  }
#endif

  // Create presentation surface
  VulkanInterface::InitVulkanHandle(*vulkanInstance, presentationSurface);
  if (!VulkanInterface::CreatePresentationSurface(*vulkanInstance, windowParameters, *presentationSurface))
  {
    return false;
  }
  
  // Get available physical devices
  std::vector<VkPhysicalDevice> physicalDevices;
  VulkanInterface::EnumerateAvailablePhysicalDevices(*vulkanInstance, physicalDevices);
  
  for (auto & physicalDevice : physicalDevices)
  {
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceProperties properties;
    VulkanInterface::GetFeaturesAndPropertiesOfPhysicalDevice(physicalDevice, features, properties);
    std::cout << "Checking Device " << properties.deviceName << std::endl;;

    // Try to find a device which supports all our desired capabilities
    // Make sure we have graphics on this device
    if (!VulkanInterface::SelectIndexOfQueueFamilyWithDesiredCapabilities(physicalDevice, VK_QUEUE_GRAPHICS_BIT, graphicsQueueParameters.familyIndex))
    {
      continue;
    }

    // Make sure we have compute on this device
    if (!VulkanInterface::SelectIndexOfQueueFamilyWithDesiredCapabilities(physicalDevice, VK_QUEUE_COMPUTE_BIT, computeQueueParameters.familyIndex))
    {
      continue;
    }

    // Make sure we have a queue family that supports presenting to our surface
    if (!VulkanInterface::SelectQueueFamilyThatSupportsPresentationToGivenSurface(physicalDevice, *presentationSurface, presentQueueParameters.familyIndex))
    {
      continue;
    }


    // Construct requestedQueues vector
    std::vector<VulkanInterface::QueueInfo> requestedQueues = {
      { graphicsQueueParameters.familyIndex, {1.f} }
    };
    // Graphics Queue Family could be the same as the Compute Queue Family so check before we add it twice
    if (graphicsQueueParameters.familyIndex != computeQueueParameters.familyIndex)
    {
      requestedQueues.push_back(
        { computeQueueParameters.familyIndex, {1.f} }
      );
    }
    // Present Queue Family, same deal, but check against both Graphics and Compute queues
    if ((graphicsQueueParameters.familyIndex != presentQueueParameters.familyIndex)
     && (computeQueueParameters.familyIndex  != presentQueueParameters.familyIndex))
    {
      requestedQueues.push_back(
        { presentQueueParameters.familyIndex, { 1.f} }
      );
    }
    
    VulkanInterface::InitVulkanHandle(vulkanDevice);
    if (!VulkanInterface::CreateLogicalDevice(physicalDevice, requestedQueues, desiredDeviceExtensions, desiredLayers, &desiredDeviceFeatures, *vulkanDevice))
    {
      // Try again
      continue;
    } 
    else
    {
      // If we've reached this point we're in the clear
      vulkanPhysicalDevice = physicalDevice;
      VulkanInterface::LoadDeviceLevelVulkanFunctions(*vulkanDevice, desiredDeviceExtensions);
      vkGetDeviceQueue(*vulkanDevice, graphicsQueueParameters.familyIndex, 0, &graphicsQueueParameters.handle);
      vkGetDeviceQueue(*vulkanDevice, computeQueueParameters.familyIndex, 0, &computeQueueParameters.handle);
      vkGetDeviceQueue(*vulkanDevice, presentQueueParameters.familyIndex, 0, &presentQueueParameters.handle);
      break;
    }
  }

  if (!vulkanDevice)
  {
    cleanupVulkan();
    // TODO: error "Failed to create logical device!" "vulkanDevice == nullptr";
    return false;
  }

  if (!createSwapchain(swapchainImageUsage, useDepth, depthAttachmentUsage))
  {
    return false;
  }

  return true;
}

bool AppBase::createSwapchain(VkImageUsageFlags swapchainImageUsage, bool /*useDepth*/, VkImageUsageFlags /*depthAttacmentUsage*/)
{
  VulkanInterface::WaitForAllSubmittedCommandsToBeFinished(*vulkanDevice);
  ready = false;

  swapchain.imageViewsRaw.clear();
  swapchain.imageViews.clear();
  swapchain.images.clear();

  VulkanHandle(VkSwapchainKHR) oldSwapchain = std::move(swapchain.handle);
  VulkanInterface::InitVulkanHandle(*vulkanDevice, swapchain.handle);
  if (!VulkanInterface::CreateStandardSwapchain(vulkanPhysicalDevice, *presentationSurface, *vulkanDevice, swapchainImageUsage, swapchain.size, swapchain.format, *oldSwapchain, *swapchain.handle, swapchain.images))
  {
    return false;
  }
  if (!swapchain.handle)
  {
    return false;
  }

  for (size_t i = 0; i < swapchain.images.size(); i++)
  {
    swapchain.imageViews.emplace_back(VulkanHandle(VkImageView)());
    VulkanInterface::InitVulkanHandle(vulkanDevice, swapchain.imageViews.back());
    if (!VulkanInterface::CreateImageView(*vulkanDevice, swapchain.images[i], VK_IMAGE_VIEW_TYPE_2D, swapchain.format, VK_IMAGE_ASPECT_COLOR_BIT, *swapchain.imageViews.back()))
    {
      return false;
    }
    swapchain.imageViewsRaw.push_back(*swapchain.imageViews.back());
  }

  ready = true;
  return true;
}

void AppBase::Shutdown()
{
  if (vulkanDevice) VulkanInterface::WaitForAllSubmittedCommandsToBeFinished(*vulkanDevice);
  cleanupVulkan();
}

void AppBase::cleanupVulkan()
{
  // We need to work backwards, destroying device-level objects before instance-level objects, and so on
  if (vulkanDevice) vkDestroyDevice(*vulkanDevice, nullptr);

#if defined(_DEBUG)
  if (callback) vkDestroyDebugUtilsMessengerEXT(*vulkanInstance, callback, nullptr);
#endif

  if (presentationSurface) vkDestroySurfaceKHR(*vulkanInstance, *presentationSurface, nullptr);  
  if (vulkanInstance) vkDestroyInstance(*vulkanInstance, nullptr);
  if (vulkanLibrary) VulkanInterface::ReleaseVulkanLoaderLibrary(vulkanLibrary);
}

void AppBase::OnMouseEvent()
{
  // Empty, override on derived classes
}

void AppBase::MouseClick(size_t buttonIndex, bool state)
{
  if (buttonIndex > 2)
  {
    MouseState.Buttons[buttonIndex] = { state, state, !state };
    OnMouseEvent();
  }
}

void AppBase::MouseMove(int x, int y)
{
  //std::cout << x << ", " << y << std::endl;
  MouseState.Position.Delta = { x - MouseState.Position.X, y - MouseState.Position.Y };
  MouseState.Position = { x, y };
  OnMouseEvent();
}

void AppBase::MouseWheel(float distance)
{
  MouseState.Wheel.WasMoved = true;
  MouseState.Wheel.Distance = distance;
  OnMouseEvent();
}

void AppBase::MouseReset()
{
  MouseState.Position.Delta = { 0, 0 };
  MouseState.Buttons[0] = { false, false };
  MouseState.Buttons[1] = { false, false };
  MouseState.Wheel = { false, 0.f };
}

void AppBase::KeyDown(unsigned int keyIndex)
{
  KeyboardState.Keys[keyIndex].IsDown = true;
}

void AppBase::KeyUp(unsigned int keyIndex)
{
  KeyboardState.Keys[keyIndex].IsDown = false;
}

void AppBase::UpdateTime()
{
  TimerState.Update();
}

bool AppBase::IsReady()
{
  return ready;
}
