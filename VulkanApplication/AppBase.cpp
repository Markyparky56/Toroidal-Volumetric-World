#include "AppBase.hpp"

const uint32_t AppBase::numFrames = 3;
const VkFormat AppBase::depthFormat = VK_FORMAT_D16_UNORM;

AppBase::AppBase()
  : vulkanLibrary(nullptr)
  , ready(false)

{

}

AppBase::~AppBase()
{

}

bool AppBase::Resize()
{

}

bool AppBase::initVulkan(VulkanInterface::WindowParameters windowParameters, VkImageUsageFlags swapchainImageUsage, bool   useDepth, VkImageUsageFlags depthAttachmentUsage)
{
  // Load Library
  try
  {
    VulkanInterface::LoadVulkanLoaderLibrary(vulkanLibrary);
  }
  catch (std::runtime_error const &e)
  {
    cleanupVulkan();
    throw(UnrecoverableRuntimeException(CreateBasicExceptionMessage("Unable to load Vulkan Library"), e));
    return false;
  }

  // Get vkGetInstanceProcAddr function so we can start loading functions
  try
  {
    VulkanInterface::LoadVulkanFunctionGetter(vulkanLibrary);
  }
  catch (std::runtime_error const &e)
  {
    cleanupVulkan();
    throw(UnrecoverableRuntimeException(CreateBasicExceptionMessage("Unable to load vkGetInstanceProcAddr function"), e));
    return false;
  }

  // Load the global vulkan functions so we can create a Vulkan Instance
  try
  {
    VulkanInterface::LoadGlobalVulkanFunctions();
  }
  catch (std::runtime_error const &e)
  {
    cleanupVulkan();
    throw(UnrecoverableRuntimeException(CreateBasicExceptionMessage("Unable to load Global-Level Functions"), e));
    return false;
  } 

  // Create a Vulkan Instance tailored to our needs, with required extensions (and if validation layers if in debug)
  try
  {
    VulkanInterface::CreateVulkanInstance(desiredExtensions, desiredLayers, "VulkanApplication", *vulkanInstance);
  }
  catch (std::runtime_error const &e)
  {
    cleanupVulkan();
    throw(UnrecoverableRuntimeException(CreateBasicExceptionMessage("Unable to create Vulkan Instance"), e));
    return false;
  }

  // If we're in debug we can attach our debugCallback function now
#if defined(_DEBUG)
  try
  {
    VulkanInterface::SetupDebugCallback(*vulkanInstance, debugCallback, callback);
  }
  catch (std::runtime_error const &e)
  {
    // Not really unrecoverable, but if this is failing in debug something is probably wrong
    cleanupVulkan();
    throw(UnrecoverableRuntimeException(CreateBasicExceptionMessage("Unable to attach debug callback to vulkan instance"), e));
    return false;
  }
#endif

  // Load Instance-level functions
  try
  {
    VulkanInterface::LoadInstanceLevelVulkanFunctions(*vulkanInstance, desiredExtensions);
  }
  catch (std::runtime_error const &e)
  {
    cleanupVulkan();
    throw(UnrecoverableRuntimeException(CreateBasicExceptionMessage("Unable to load Instance-Level Functions"), e));
    return false;
  }

  // Create presentation surface
  try
  {
    presentationSurface = VulkanHandle<VkSurfaceKHR>(*vulkanInstance);
    VulkanInterface::CreatePresentationSurface(*vulkanInstance, windowParameters, *presentationSurface);
  }
  catch (std::runtime_error const&e)
  {
    cleanupVulkan();
    throw(UnrecoverableRuntimeException(CreateBasicExceptionMessage("Could not create presentation surface"), e));
    return false;
  }

  // Get available physical devices
  std::vector<VkPhysicalDevice> physicalDevices;
  try
  {
    VulkanInterface::EnumerateAvailablePhysicalDevices(*vulkanInstance, physicalDevices);
  }
  catch (std::runtime_error const &e)
  {
    cleanupVulkan();
    throw(UnrecoverableRuntimeException(CreateBasicExceptionMessage("Unable to enumerate available physical devices"), e));
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
      if (!VulkanInterface::SelectQueueFamilyThatSupportsPresentationToGivenSurface(physicalDevice, *presentationSurface, presentQueue.FamilyIndex))
      {
        continue;
      }
    }
    catch (std::runtime_error const &e)
    {
      // Exceptions thrown within this block are only relating to Vulkan outright failing to get queue families
      // to check against, I'm assuming if this fails there's an issue with Vulkan or the Device.
      throw(UnrecoverableRuntimeException(CreateBasicExceptionMessage("Failed to check queue families for available physical device"), e));
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
      VulkanInterface::CreateLogicalDevice(physicalDevice, requestedQueues, desiredExtensions, desiredLayers, &desiredDeviceFeatures, *vulkanDevice);
    }
    catch (std::runtime_error const &e)
    {
      std::cerr << "Failed to create logical device from chosen physical device\n";
      std::cerr << e.what() << std::endl;
      std::cerr << "Attempting to find another device." << std::endl;
      continue;
    }
    
    // If we've reached this point we're in the clear
    vulkanPhysicalDevice = physicalDevice;
    VulkanInterface::LoadDeviceLevelVulkanFunctions(*vulkanDevice, desiredExtensions);
    VulkanInterface::vkGetDeviceQueue(*vulkanDevice, graphicsQueue.FamilyIndex, 0, &graphicsQueue.Handle);
    VulkanInterface::vkGetDeviceQueue(*vulkanDevice, computeQueue.FamilyIndex, 0, &computeQueue.Handle);
    VulkanInterface::vkGetDeviceQueue(*vulkanDevice, presentQueue.FamilyIndex, 0, &presentQueue.Handle);
    break;
  }

  if (vulkanDevice == nullptr)
  {
    cleanupVulkan();
    throw(UnrecoverableRuntimeException(CreateBasicExceptionMessage("Failed to create logical device!"), "vulkanDevice == nullptr"));
    return false;
  }

  // Prepare Frame Resources
  VulkanInterface::InitVulkanHandle(*vulkanDevice, commandPool);
  if (!VulkanInterface::CreateCommandPool(*vulkanDevice, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, graphicsQueue.FamilyIndex, *commandPool))
  {
    return false;
  }

  for (uint32_t i = 0; i < numFrames; i++)
  {
    std::vector<VkCommandBuffer> commandBuffer;
    VulkanHandle<VkSemaphore> imageAcquiredSemaphore(vulkanDevice);
    VulkanHandle<VkSemaphore> readyToPresentSemaphore(vulkanDevice);
    VulkanHandle<VkFence> drawingFinishedFence(vulkanDevice);
    VulkanHandle<VkImageView> depthAttachment(vulkanDevice);

    if (!VulkanInterface::AllocateCommandBuffers(*vulkanDevice, *commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1, commandBuffer))
    {
      return false;
    }
    if (!VulkanInterface::CreateSemaphore(*vulkanDevice, *imageAcquiredSemaphore))
    {
      return false;
    }
    if (!VulkanInterface::CreateFence(*vulkanDevice, true, *drawingFinishedFence))
    {
      return false;
    }

    frameResources.push_back(
      {
        commandBuffer[0],
        std::move(imageAcquiredSemaphore),
        std::move(readyToPresentSemaphore),
        std::move(drawingFinishedFence),
        std::move(depthAttachment),
        {}
      }
    );
  }

  if (!createSwapchain(swapchainImageUsage, useDepth, depthAttachmentUsage))
  {
    return false;
  }

  return true;
}

bool AppBase::createSwapchain(VkImageUsageFlags swapchainImageUsage, bool useDepth, VkImageUsageFlags depthAttacmentUsage)
{
  VulkanInterface::WaitForAllSubmittedCommandsToBeFinished(*vulkanDevice);
  ready = false;

  swapchain.imageViewsRaw.clear();
  swapchain.imageViews.clear();
  swapchain.images.clear();

  VulkanHandle<VkSwapchainKHR> oldSwapchain = std::move(swapchain.handle);
  swapchain.handle = VulkanHandle<VkSwapchainKHR>(*vulkanDevice);
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
    swapchain.imageViews.emplace_back(vulkanDevice);
    if (!VulkanInterface::CreateImageView(*vulkanDevice, swapchain.images[i], VK_IMAGE_VIEW_TYPE_2D, swapchain.format, VK_IMAGE_ASPECT_COLOR_BIT, *swapchain.imageViews.back()))
    {
      return false;
    }
  }

  return true;
}

void AppBase::Shutdown()
{
  cleanupVulkan();
}

void AppBase::cleanupVulkan()
{
  // We need to work backwards, destroying device-level objects before instance-level objects, and so on
  if (vulkanDevice) VulkanInterface::vkDestroyDevice(*vulkanDevice, nullptr);

#if defined(_DEBUG)
  if (callback) VulkanInterface::vkDestroyDebugUtilsMessengerEXT(*vulkanInstance, callback, nullptr);
#endif

  if (presentationSurface) VulkanInterface::vkDestroySurfaceKHR(*vulkanInstance, *presentationSurface, nullptr);  
  if (vulkanInstance) VulkanInterface::vkDestroyInstance(*vulkanInstance, nullptr);
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

void AppBase::UpdateTime()
{
  TimerState.Update();
}

bool AppBase::IsReady()
{
  return ready;
}
