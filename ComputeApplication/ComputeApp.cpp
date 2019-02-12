#include "ComputeApp.hpp"

bool ComputeApp::Initialise(VulkanInterface::WindowParameters windowParameters)
{
  if (!setupVulkanAndCreateSwapchain(windowParameters))
  {
    return false;
  }
}

bool ComputeApp::Update()
{
  return false;
}

bool ComputeApp::Resize()
{
  ImGui_ImplVulkan_InvalidateDeviceObjects();
  if (!createSwapchain(
    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
    , true
    , VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
    )
  {
    return false;
  }

  if (!VulkanInterface::CreateFramebuffersForFrameResources(
    *vulkanDevice
    , *renderPass
    , swapchain
    , graphicsPipeline.frameResources)
    )
  {
    return false;
  }
  ImGui_ImplVulkan_CreateDeviceObjects();
  return true;
}

bool ComputeApp::setupVulkanAndCreateSwapchain(VulkanInterface::WindowParameters windowParameters)
{
  // Load Library
  if (!VulkanInterface::LoadVulkanLoaderLibrary(vulkanLibrary))
  {
    return false;
  }

  // Get vkGetInstanceProcAddr function
  if (!VulkanInterface::LoadVulkanFunctionGetter(vulkanLibrary))
  {
    return false;
  }

  // Load the global vulkan functions
  if (!VulkanInterface::LoadGlobalVulkanFunctions())
  {
    return false;
  }

  // Create a Vulkan Instance tailored to our needs with required extensions (and validation layers if in debug)
  VulkanInterface::InitVulkanHandle(vulkanInstance);
  if (!VulkanInterface::CreateVulkanInstance(desiredInstanceExtensions, desiredLayers, "Compute Pipeline App", *vulkanInstance))
  {
    return false;
  }

  // Load Instance-level functions
  if (!VulkanInterface::LoadInstanceLevelVulkanFunctions(*vulkanInstance, desiredInstanceExtensions))
  {
    return false;
  }

  // If we're in debug we can attach out debug callback function now
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

  // Get available physical device
  std::vector<VkPhysicalDevice> physicalDevices;
  VulkanInterface::EnumerateAvailablePhysicalDevices(*vulkanInstance, physicalDevices);

  for (auto & physicalDevice : physicalDevices)
  {
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceProperties properties;
    VulkanInterface::GetFeaturesAndPropertiesOfPhysicalDevice(physicalDevice, features, properties);
    std::cout << "Checking Device " << properties.deviceName << std::endl;

    // Try to find a device which supports all our desired capabilities (graphics, compute, present);
    // Make sure we have graphics on this devie
    if (!VulkanInterface::SelectIndexOfQueueFamilyWithDesiredCapabilities(physicalDevice, VK_QUEUE_GRAPHICS_BIT, graphicsQueueParameters.familyIndex))
    {
      continue;
    }

    // Make sure we have compute on this device
    if (!VulkanInterface::SelectIndexOfQueueFamilyWithDesiredCapabilities(physicalDevice, VK_QUEUE_COMPUTE_BIT, computeQueueParameters.familyIndex))
    {
      continue;
    }

    if (!VulkanInterface::SelectQueueFamilyThatSupportsPresentationToGivenSurface(physicalDevice, *presentationSurface, presentQueueParameters.familyIndex))
    {
      continue;
    }

    // Check how many concurrent threads we have available (like 4 or 8, depending on cpu)
    auto numConcurrentThreads = std::thread::hardware_concurrency();
    // We want to reserve one for rendering and allocate the rest for compute
    int numComputeThreads = numConcurrentThreads - 1;
    if (numComputeThreads < 1)
      return false; // Bail, this isn't going to end well with a single thread    

    if (graphicsQueueParameters.familyIndex == computeQueueParameters.familyIndex
      && graphicsQueueParameters.familyIndex == presentQueueParameters.familyIndex)
      // Yay, probably a nVidia GPU, all queues are capable at doing everything, only possible thing to improve on this 
      // is to find if we have a dedicated transfer queue and work out how to use that properly. 
      // Will use the same queue family for all commands for now.
    {
      const float computeQueuePriority = 1.f; // Unsure what priority compute queues should have versus the graphics queue, stick with 1.f for now
      std::vector<float> queuePriorities = { 1.f }; // One for the graphics queue

      // Check how many compute queues we can have
      std::vector<VkQueueFamilyProperties> queueFamilies;
      if (!VulkanInterface::CheckAvailableQueueFamiliesAndTheirProperties(physicalDevice, queueFamilies))
      {
        return false;
      }

      if (queueFamilies[graphicsQueueParameters.familyIndex].queueCount < numComputeThreads)
      {
        numComputeThreads = queueFamilies[graphicsQueueParameters.familyIndex].queueCount - 1; // again, save one for dedicated graphics work
        if (numComputeThreads < 1)
        {
          return false; // Nope. Nope. Nope.
        }
      }

      for (int i = 0; i < numComputeThreads; i++)
      {
        queuePriorities.push_back(computeQueuePriority);
      }

      // Construct requestedQueues vector
      std::vector<VulkanInterface::QueueInfo> requestedQueues = {
        {graphicsQueueParameters.familyIndex, queuePriorities} // One graphics queue, numComputeThreads queues
      };

      VulkanInterface::InitVulkanHandle(vulkanDevice);
      if (!VulkanInterface::CreateLogicalDevice(physicalDevice, requestedQueues, desiredDeviceExtensions, desiredLayers, &desiredDeviceFeatures, *vulkanDevice))
      {
        // Try again, maybe there's a better device?
        continue;
      }
      else
      {
        vulkanPhysicalDevice = physicalDevice;
        VulkanInterface::LoadDeviceLevelVulkanFunctions(*vulkanDevice, desiredDeviceExtensions);
        // Retrieve graphics queue handle
        VulkanInterface::vkGetDeviceQueue(*vulkanDevice, graphicsQueueParameters.familyIndex, 0, &graphicsQueue);
        // Retrieve "present queue" handle
        VulkanInterface::vkGetDeviceQueue(*vulkanDevice, presentQueueParameters.familyIndex, 0, &presentQueue);
        // Retrieve compute queue handles
        computeQueues.resize(numComputeThreads, VK_NULL_HANDLE);
        for (int i = 1; i < numComputeThreads; i++)
        {
          VulkanInterface::vkGetDeviceQueue(*vulkanDevice, graphicsQueueParameters.familyIndex, i, &computeQueues[i - 1]);
        }
      }
    }
    else // A more involved setup... coming soon to an application near you
    {
      std::vector<float> gQueuePriorities = { 1.f };
      std::vector<float> cQueuePriorities = {};
      
      continue;
    }
  }
  // Check we actually created a device...
  if (!vulkanDevice)
  {
    cleanupVulkan();
    return false;
  }

  // Create that swapchain
  if (!createSwapchain( VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
                      , true
                      , VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT))
  {
    return false;
  }
}

bool ComputeApp::setupTaskflow()
{
  tfExecutor = std::make_shared<tf::Taskflow::Executor>(std::thread::hardware_concurrency()); // maybe -1?

  graphicsTaskflow = std::make_unique<tf::Taskflow>(tfExecutor);
  computeTaskflow = std::make_unique<tf::Taskflow>(tfExecutor);

  return false;
}

bool ComputeApp::initialiseVulkanMemoryAllocator()
{
  VmaAllocatorCreateInfo allocatorInfo = {};
  allocatorInfo.physicalDevice = vulkanPhysicalDevice;
  allocatorInfo.device = *vulkanDevice;

  VkResult result = vmaCreateAllocator(&allocatorInfo, &allocator);
  if (result != VK_SUCCESS) 
  {
    return false;
  }
  else
  {
    return true;
  }
}

bool ComputeApp::initImGui(HWND hwnd)
{
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGui::StyleColorsDark();

  if (!ImGui_ImplWin32_Init(hwnd))
  {
    return false;
  }

  ImGui_ImplVulkan_InitInfo initInfo = { 0 };
  initInfo.Instance = *vulkanInstance;
  initInfo.PhysicalDevice = vulkanPhysicalDevice;
  initInfo.Device = *vulkanDevice;
  initInfo.QueueFamily = graphicsQueueParameters.familyIndex;
  initInfo.Queue = graphicsQueue;
  initInfo.PipelineCache = VK_NULL_HANDLE;
  initInfo.DescriptorPool = *(graphicsPipeline.descriptorPool);
  initInfo.Allocator = VK_NULL_HANDLE;
  initInfo.CheckVkResultFn = [](VkResult err) { if (err == VK_SUCCESS) return; else { std::cout << "ImGui Error (Non-success return value), Code: " << err << std::endl; if (err < 0) abort; }};

  if (!ImGui_ImplVulkan_Init(&initInfo, *renderPass))
  {
    return false;
  }

  return true;
}

bool ComputeApp::setupGraphicsPipeline()
{
  return false;
}

void ComputeApp::shutdownVulkanMemoryAllocator()
{
  if (allocator)
  {
    vmaDestroyAllocator(allocator);
    allocator = VK_NULL_HANDLE;
  }
}

void ComputeApp::cleanupVulkan()
{
  if (vulkanDevice) VulkanInterface::vkDestroyDevice(*vulkanDevice, nullptr);

#if defined(_DEBUG)
  if (callback) VulkanInterface::vkDestroyDebugUtilsMessengerEXT(*vulkanInstance, callback, nullptr);
#endif

  if (presentationSurface) VulkanInterface::vkDestroySurfaceKHR(*vulkanInstance, *presentationSurface, nullptr);
  if (vulkanInstance) VulkanInterface::vkDestroyInstance(*vulkanInstance, nullptr);
  if (vulkanLibrary) VulkanInterface::ReleaseVulkanLoaderLibrary(vulkanLibrary);
}
