#include "ComputeApp.hpp"
#include <glm/gtc/matrix_transform.hpp>

//#include "chunkDirectionalLight.vert.spv.h"
//#include "chunkDirectionalLight.frag.spv.h"

static void check_vk_result(VkResult err)
{
  if (err == 0) return;
  printf("VkResult %d\n", err);
  if (err < 0)
    abort();
}

bool ComputeApp::Initialise(VulkanInterface::WindowParameters windowParameters)
{
  // Setup some basic data
  gameTime = 0.0;
  settingsLastChangeTimes = { 0.f };
  lockMouse = true;
  //ShowCursor(!lockMouse);
  camera.SetPosition({ 0.f, 32.f, 0.f });
  camera.LookAt({ 0.f, 0.f, 0.f }, { 0.f, 0.f, 1.f }, { 0.f, 1.f, 0.f });  

  POINT pt;
  pt.x = static_cast<LONG>(screenCentre.x);
  pt.y = static_cast<LONG>(screenCentre.y);
  ClientToScreen(static_cast<HWND>(hWnd), &pt);
  SetCursorPos(pt.x, pt.y);
  MouseState.Position.X = pt.x;
  MouseState.Position.Y = pt.y;

  hWnd = windowParameters.HWnd;

  if (!setupVulkanAndCreateSwapchain(windowParameters))
  {
    return false;
  }

  screenCentre = { static_cast<float>(swapchain.size.width)/2, static_cast<float>(swapchain.size.height)/2 };

  if (!setupTaskflow())
  {
    return false;
  }

  // Prepare setup tasks
  bool resVMA, resImGui, resCmdBufs, resRenderpass, resGpipe, resChnkMngr, resTerGen, resECS, resSurface;
  auto[vma, imgui, commandBuffers, renderpass, gpipeline, frameres, chunkmanager, terraingen, surface, ecs] = systemTaskflow->emplace(
    [&]() { resVMA = initialiseVulkanMemoryAllocator(); },
    [&]() { resImGui = initImGui(windowParameters.HWnd); },
    [&]() { resCmdBufs = setupCommandPoolAndBuffers(); },
    [&]() { resRenderpass = setupRenderPass(); },
    [&]() { 
      if (!resRenderpass) { resGpipe = false; return; }
      resGpipe = setupGraphicsPipeline(); },
    [&]() { setupFrameResources(); },
    [&]() { 
      if (!resECS && !resVMA) { resChnkMngr = false; return; }
      resChnkMngr = setupChunkManager(); },
    [&]() { resTerGen = setupTerrainGenerator(); },
    [&]() { resSurface = setupSurfaceExtractor(); },
    [&]() { resECS = setupECS(); }
  );

  // Task dependencies
  vma.precede(gpipeline);
  vma.precede(chunkmanager);
  vma.precede(frameres);
  renderpass.precede(gpipeline);
  //renderpass.precede(imgui);
  ecs.precede(chunkmanager);
  commandBuffers.precede(surface);
  commandBuffers.precede(frameres);
  frameres.precede(imgui);

  // Execute and wait for completion
  systemTaskflow->dispatch().get();  

  //resVMA = initialiseVulkanMemoryAllocator();
  //resRenderpass = setupRenderPass();
  //resImGui = initImGui(windowParameters.HWnd);
  //resGpipe = setupGraphicsPipeline();
  //resCmdBufs = setupCommandPoolAndBuffers();
  //resECS = setupECS();
  //resChnkMngr = setupChunkManager();
  //resTerGen = setupTerrainGenerator();

  if (!resVMA || !resImGui || !resCmdBufs || !resRenderpass || !resGpipe || !resChnkMngr || !resTerGen || !resSurface || !resECS)
  {
    return false;
  }

  return true;
}

bool ComputeApp::Update()
{
  gameTime += TimerState.GetDeltaTime();

  // Massive memory leak
  /*if (!commandPools->graphicsPools.resetFramePools(nextFrameIndex))
  {
    return false;
  }*/

  tf::Taskflow tf(std::thread::hardware_concurrency());

  auto[userUpdate, spawnChunks, renderList/*, recordDrawCalls*/] = tf.emplace(
    [&]() { updateUser(); },
    [&]() { checkForNewChunks(); },
    [&]() { getChunkRenderList(); }
    //[&]() { recordChunkDrawCalls(); } 
  );

  userUpdate.precede(spawnChunks);
  userUpdate.precede(renderList);
  spawnChunks.precede(renderList);
  //spawnChunks.precede(renderList);
  //renderList.precede(recordDrawCalls);
  //recordDrawCalls.precede(draw);

  tf.dispatch().get();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();
  {
    ImGui::ShowDemoWindow();

    ImGui::Begin("Hello, world!");
    ImGui::Text("Avg %.3f ms/frame (%.1f FPS)", 1000.f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();
  }
  ImGui::EndFrame();
  ImGui::Render();
  //ImGui_ImplVulkan_NewFrame();

  drawChunks();
  //std::cout << "Draw\t" << nextFrameIndex << std::endl;
  //std::cout << systemTaskflow->num_topologies() << std::endl;
  //std::cout << graphicsTaskflow->num_topologies() << std::endl;
  //std::cout << computeTaskflow->num_topologies() << std::endl;

  return true;
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
    , renderPass
    , swapchain
    , frameResources)
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
    uint32_t numComputeThreads = numConcurrentThreads - 1;
    if (numComputeThreads < 1)
      return false; // Bail, this isn't going to end well with a single thread    

    if (graphicsQueueParameters.familyIndex == computeQueueParameters.familyIndex
      && graphicsQueueParameters.familyIndex == presentQueueParameters.familyIndex)
      // Yay, probably a nVidia GPU, all queues are capable at doing everything, only possible thing to improve on this 
      // is to find if we have a dedicated transfer queue and work out how to use that properly. 
      // Will use the same queue family for all commands for now.
    {
      const float computeQueuePriority = 1.f; // Unsure what priority compute queues should have versus the graphics queue, stick with 1.f for now
      std::vector<float> queuePriorities = { 1.f, 1.f }; // One for the graphics queue, one for transfer

      // Check how many compute queues we can have
      std::vector<VkQueueFamilyProperties> queueFamilies;
      if (!VulkanInterface::CheckAvailableQueueFamiliesAndTheirProperties(physicalDevice, queueFamilies))
      {
        return false;
      }

      if (queueFamilies[graphicsQueueParameters.familyIndex].queueCount < numComputeThreads)
      {
        numComputeThreads = queueFamilies[graphicsQueueParameters.familyIndex].queueCount - 2; // again, save one for dedicated graphics work
        if (numComputeThreads < 1)
        {
          return false; // Nope. Nope. Nope.
        }
      }

      for (uint32_t i = 0; i < numComputeThreads; i++)
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
        vkGetDeviceQueue(*vulkanDevice, graphicsQueueParameters.familyIndex, 0, &graphicsQueue);
        // Retrieve the transfer queue handle
        // TODO: Find optimal transfer queue
        vkGetDeviceQueue(*vulkanDevice, graphicsQueueParameters.familyIndex, 1, &transferQueue);
        // Retrieve "present queue" handle
        vkGetDeviceQueue(*vulkanDevice, presentQueueParameters.familyIndex, 0, &presentQueue);
        // Retrieve compute queue handles
        computeQueues.resize(numComputeThreads, VK_NULL_HANDLE);
        for (uint32_t i = 2; i < numComputeThreads; i++)
        {
          vkGetDeviceQueue(*vulkanDevice, graphicsQueueParameters.familyIndex, i, &computeQueues[i - 1]);
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

  return true;
}

bool ComputeApp::setupTaskflow()
{
  tfExecutor = std::make_shared<tf::Taskflow::Executor>(std::thread::hardware_concurrency()); // maybe -1?

  graphicsTaskflow = std::make_unique<tf::Taskflow>(std::thread::hardware_concurrency());
  computeTaskflow = std::make_unique<tf::Taskflow>(std::thread::hardware_concurrency());
  systemTaskflow = std::make_unique<tf::Taskflow>(std::thread::hardware_concurrency()-1);

  return true;
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
  
  std::vector<VkDescriptorPoolSize> poolSizes = 
  {
    { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
    { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
    { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
    { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
    { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
    { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
  };
  if (!VulkanInterface::CreateDescriptorPool(*vulkanDevice, true, 1000 * static_cast<uint32_t>(poolSizes.size()), poolSizes, imGuiDescriptorPool))
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
  initInfo.DescriptorPool = imGuiDescriptorPool;
  initInfo.Allocator = VK_NULL_HANDLE;
  initInfo.CheckVkResultFn = [](VkResult err) { 
    if (err == VK_SUCCESS) return; 
    else { 
      std::cout << "ImGui Error (Non-success return value), Code: " << err << std::endl; 
#if defined(_DEBUG)
      if (err < 0) 
        abort(); 
#endif
    }
  };

  if (!ImGui_ImplVulkan_Init(&initInfo, renderPass))
  {
    return false;
  }

  // Upload fonts since it's not done automatically
  {
    VkCommandBuffer cbuf = frameResources[0].commandBuffer;

    VkResult err;
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    err = vkBeginCommandBuffer(cbuf, &begin_info);
    check_vk_result(err);

    ImGui_ImplVulkan_CreateFontsTexture(cbuf);

    VkSubmitInfo end_info = {};
    end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    end_info.commandBufferCount = 1;
    end_info.pCommandBuffers = &cbuf;
    err = vkEndCommandBuffer(cbuf);
    check_vk_result(err);
    err = vkQueueSubmit(graphicsQueue, 1, &end_info, VK_NULL_HANDLE);
    check_vk_result(err);

    err = vkDeviceWaitIdle(*vulkanDevice);
    check_vk_result(err);
    ImGui_ImplVulkan_InvalidateFontUploadObjects();

    /*
    if (!VulkanInterface::BeginCommandBufferRecordingOp(cbuf, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr))
    {
      return false;
    }

    if (!ImGui_ImplVulkan_CreateFontsTexture(cbuf))
    {
      return false;
    }

    if (!VulkanInterface::EndCommandBufferRecordingOp(cbuf))
    {
      return false;
    }

    if (!VulkanInterface::SubmitCommandBuffersToQueue(graphicsQueue, {}, { cbuf }, {}, {}))
    {
      return false;
    }

    VkResult res = vkDeviceWaitIdle(*vulkanDevice);
    if (res != VK_SUCCESS)
    {
      return false;
    }
    ImGui_ImplVulkan_InvalidateFontUploadObjects();*/
  }

  return true;
}

bool ComputeApp::setupCommandPoolAndBuffers()
{
  size_t numWorkers = graphicsTaskflow->num_workers();

  commandPools = std::make_unique<TaskflowCommandPools>(
    &*vulkanDevice
    , graphicsQueueParameters.familyIndex
    , graphicsQueueParameters.familyIndex
    , numWorkers);
  /*
  // Create Graphics Command Pool
  for (int i = 0; i < numWorkers; i++)
  {
    graphicsCommandPools.push_back(VkCommandPool());
    if (!VulkanInterface::CreateCommandPool(*vulkanDevice
      , VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
      , graphicsQueueParameters.familyIndex
      , graphicsCommandPools[i]))
    {
      return false;
    }
  }
  // Allocate an extra one for frameResources
  graphicsCommandPools.push_back(VkCommandPool());
  if (!VulkanInterface::CreateCommandPool(*vulkanDevice
    , VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
    , graphicsQueueParameters.familyIndex
    , graphicsCommandPools[numWorkers]))
  {
    return false;
  }

  // Allocate primary command buffers for frame resources
  if (!VulkanInterface::AllocateCommandBuffers(*vulkanDevice
    , graphicsCommandPools[numWorkers]
    , VK_COMMAND_BUFFER_LEVEL_PRIMARY
    , numFrames
    , frameCommandBuffers))
  {
    return false;
  }

  for (int i = 0; i < graphicsCommandPools.size(); i++)
  {
    chunkCommandBuffersVecs.push_back(std::vector<VkCommandBuffer>());
    // Allocate command buffers for chunk models, note: secondary command buffers
    if (!VulkanInterface::AllocateCommandBuffers(*vulkanDevice
      , graphicsCommandPools[i]
      , VK_COMMAND_BUFFER_LEVEL_SECONDARY
      , maxChunks*numFrames // Excessive amount
      , chunkCommandBuffersVecs[i]))
    {
      return false;
    }
  }

  // Setup command buffer stacks
  for (uint32_t i = 0; i < numFrames; i++)
  {
    for (uint32_t buffer = maxChunks*i; buffer < maxChunks; buffer++)
    {
      chunkCommandBufferStacks[i].push(&chunkCommandBuffersVec[buffer]);
    }
  }

  // Create command pools for transfer command buffers
  for (int i = 0; i < graphicsTaskflow->num_workers(); i++)
  {
    if (!VulkanInterface::CreateCommandPool(*vulkanDevice
      , VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
      , graphicsQueueParameters.familyIndex // Ideally this should have its own dedicated transfer queue
      , transferCommandPools[i]))
    {
      return false;
    }
  }

  for (int i = 0; i < transferCommandPools.size(); i++)
  {  
    // Allocate transfer command buffers
    if (!VulkanInterface::AllocateCommandBuffers(*vulkanDevice
      , transferCommandPools[i]
      , VK_COMMAND_BUFFER_LEVEL_PRIMARY
      , tfExecutor->num_workers()
      , transferCommandBuffers))
    {
      return false;
    }
  }

  // Setup transfer command buffer stack
  for(auto & buffer : transferCommandBuffers)
  {
    transferCommandBuffersStack.push(&buffer);
  }

  // TODO: Compute command pool and buffers
  */
  return true;
}

bool ComputeApp::setupRenderPass()
{
  std::vector<VkAttachmentDescription> attachmentDescriptions = {
    {
      0,
      swapchain.format,
      VK_SAMPLE_COUNT_1_BIT,
      VK_ATTACHMENT_LOAD_OP_CLEAR,
      VK_ATTACHMENT_STORE_OP_STORE,
      VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      VK_ATTACHMENT_STORE_OP_DONT_CARE,
      VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    },
    {
      0,
      VK_FORMAT_D16_UNORM,
      VK_SAMPLE_COUNT_1_BIT,
      VK_ATTACHMENT_LOAD_OP_CLEAR,
      VK_ATTACHMENT_STORE_OP_DONT_CARE,
      VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      VK_ATTACHMENT_STORE_OP_DONT_CARE,
      VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    }
  };

  VkAttachmentReference depthAttachment = {
    1,
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
  };

  std::vector<VulkanInterface::SubpassParameters> subpassParameters = {
    {
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      {}, // InputAttachments
      {
        {
          0,
          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        }
      }, // ColorAttachments
      {}, // ResolveAttachments
      &depthAttachment,
      {}
    }
  };

  std::vector<VkSubpassDependency> subpassDependecies = {
    {
      VK_SUBPASS_EXTERNAL,
      0,
      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      VK_ACCESS_MEMORY_READ_BIT,
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      VK_DEPENDENCY_BY_REGION_BIT
    },
    {
      0,
      VK_SUBPASS_EXTERNAL,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      VK_ACCESS_MEMORY_READ_BIT,
      VK_DEPENDENCY_BY_REGION_BIT
    }
  };

  if (!VulkanInterface::CreateRenderPass(*vulkanDevice, attachmentDescriptions, subpassParameters, subpassDependecies, renderPass))
  {
    return false;
  }
  return true;
}

bool ComputeApp::setupGraphicsPipeline()
{
  //graphicsPipeline = std::make_unique<GraphicsPipeline>(&(*vulkanDevice), &renderPass, "Data/vert.spv", "Data/frag.spv");
  //std::vector<VkDescriptorPoolSize> poolSizes = {
  //  { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 }
  //};
  //graphicsPipeline->setupDescriptorPool(poolSizes);
  //graphicsPipeline->layoutBindings.push_back(
  //  { 
  //    VkDescriptorSetLayoutBinding
  //    {
  //      0,
  //      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
  //      1,
  //      VK_SHADER_STAGE_VERTEX_BIT,
  //      nullptr
  //    } 
  //  }
  //);

  // Create buffers
  //if (!VulkanInterface::CreateUniformBuffer(allocator, sizeof(glm::mat4), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, viewprojUBuffer, VMA_MEMORY, viewprojAlloc))
  //{
  //  return false;
  //}

  if (!VulkanInterface::CreateBuffer(allocator
    , sizeof(ViewProj)
    , VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
    , viewprojUBuffer
    , VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT
    , VMA_MEMORY_USAGE_UNKNOWN
    , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
    , VK_NULL_HANDLE, viewprojAlloc))
  {
    return false;
  }

  //if (!VulkanInterface::CreateUniformBuffer(allocator, sizeof(glm::mat4), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, modelUBuffer, VMA_MEMORY_USAGE_, modelAlloc))
  //{
  //  return false;
  //}

  VkPhysicalDeviceProperties props;
  vkGetPhysicalDeviceProperties(vulkanPhysicalDevice, &props);
  size_t deviceAlignment = props.limits.minUniformBufferOffsetAlignment;
  dynamicAlignment = (sizeof(PerChunkData) / deviceAlignment) * deviceAlignment + ((sizeof(PerChunkData) % deviceAlignment) > 0 ? deviceAlignment : 0);

  // Always mapped model buffer for easy copy
  if (!VulkanInterface::CreateBuffer(allocator
    , dynamicAlignment*256
    , VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
    , modelUBuffer
    , VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT
    , VMA_MEMORY_USAGE_UNKNOWN
    , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
    , VK_NULL_HANDLE, modelAlloc))
  {
    return false;
  }

  //if (!VulkanInterface::CreateUniformBuffer(allocator, sizeof(LightData), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, lightUBuffer, VMA_MEMORY_USAGE_UNKNOWN, lightAlloc))
  //{
  //  return false;
  //}

  if (!VulkanInterface::CreateBuffer(allocator
    , sizeof(LightData)
    , VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
    , lightUBuffer
    , VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT
    , VMA_MEMORY_USAGE_UNKNOWN
    , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
    , VK_NULL_HANDLE, lightAlloc))
  {
    return false;
  }

  // Descriptor set with uniform buffer
  std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings = {
    { // ViewProj
      0,                                          // uint32_t             binding
      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,          // VkDescriptorType     descriptorType
      1,                                          // uint32_t             descriptorCount
      VK_SHADER_STAGE_VERTEX_BIT,                 // VkShaderStageFlags   stageFlags
      nullptr                                     // const VkSampler    * pImmutableSamplers
    },
    { // Model
      1,
      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
      1,
      VK_SHADER_STAGE_VERTEX_BIT,
      nullptr
    },
    { // Light
      2,
      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      1,
      VK_SHADER_STAGE_FRAGMENT_BIT,
      nullptr
    }
  };

  if (!VulkanInterface::CreateDescriptorSetLayout(*vulkanDevice, descriptorSetLayoutBindings, descriptorSetLayout)) 
  {
    return false;
  }

  VkDescriptorPoolSize descriptorPoolSizeUB = {
      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,          // VkDescriptorType     type
      2                                           // uint32_t             descriptorCount
  };
  VkDescriptorPoolSize descriptorPoolSizeUBD = {
      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,          // VkDescriptorType     type
      1                                           // uint32_t             descriptorCount
  };

  if (!VulkanInterface::CreateDescriptorPool(*vulkanDevice, false, 1, { descriptorPoolSizeUB, descriptorPoolSizeUBD }, descriptorPool))
  {
    return false;
  }

  if (!VulkanInterface::AllocateDescriptorSets(*vulkanDevice, descriptorPool, { descriptorSetLayout }, descriptorSets))
  {
    return false;
  }

  VulkanInterface::BufferDescriptorInfo viewProjDescriptorUpdate = {
      descriptorSets[0],                          // VkDescriptorSet                      TargetDescriptorSet
      0,                                          // uint32_t                             TargetDescriptorBinding
      0,                                          // uint32_t                             TargetArrayElement
      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,          // VkDescriptorType                     TargetDescriptorType
      {                                           // std::vector<VkDescriptorBufferInfo>  BufferInfos
        {
          viewprojUBuffer,                        // VkBuffer                             buffer
          0,                                      // VkDeviceSize                         offset
          VK_WHOLE_SIZE                           // VkDeviceSize                         range
        }
      }
  };
  VulkanInterface::BufferDescriptorInfo modelDescriptorUpdate = {
      descriptorSets[0],                          // VkDescriptorSet                      TargetDescriptorSet
      1,                                          // uint32_t                             TargetDescriptorBinding
      0,                                          // uint32_t                             TargetArrayElement
      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,          // VkDescriptorType                     TargetDescriptorType
      {                                           // std::vector<VkDescriptorBufferInfo>  BufferInfos
        {
          modelUBuffer,                           // VkBuffer                             buffer
          0,                                      // VkDeviceSize                         offset
          VK_WHOLE_SIZE                           // VkDeviceSize                         range
        }
      }
  };
  VulkanInterface::BufferDescriptorInfo lightDescriptorUpdate = {
      descriptorSets[0],                          // VkDescriptorSet                      TargetDescriptorSet
      2,                                          // uint32_t                             TargetDescriptorBinding
      0,                                          // uint32_t                             TargetArrayElement
      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,          // VkDescriptorType                     TargetDescriptorType
      {                                           // std::vector<VkDescriptorBufferInfo>  BufferInfos
        {
          lightUBuffer,                           // VkBuffer                             buffer
          0,                                      // VkDeviceSize                         offset
          VK_WHOLE_SIZE                           // VkDeviceSize                         range
        }
      }
  };

  VulkanInterface::UpdateDescriptorSets(*vulkanDevice, {}, { viewProjDescriptorUpdate, modelDescriptorUpdate, lightDescriptorUpdate }, {}, {});

  std::vector<unsigned char> vertex_shader_spirv;
  if (!VulkanInterface::GetBinaryFileContents("Data/vert.spv", vertex_shader_spirv)) {
    return false;
  }

  VkShaderModule vertex_shader_module;
  if (!VulkanInterface::CreateShaderModule(*vulkanDevice, vertex_shader_spirv, vertex_shader_module)) {
    return false;
  }

  std::vector<unsigned char> fragment_shader_spirv;
  if (!VulkanInterface::GetBinaryFileContents("Data/frag.spv", fragment_shader_spirv)) {
    return false;
  }
  VkShaderModule fragment_shader_module;
  if (!VulkanInterface::CreateShaderModule(*vulkanDevice, fragment_shader_spirv, fragment_shader_module)) {
    return false;
  }

  std::vector<VulkanInterface::ShaderStageParameters> shader_stage_params = {
      {
        VK_SHADER_STAGE_VERTEX_BIT,   // VkShaderStageFlagBits        ShaderStage
        vertex_shader_module,        // VkShaderModule               ShaderModule
        "main",                       // char const                 * EntryPointName;
        nullptr                       // VkSpecializationInfo const * SpecializationInfo;
      },
      {
        VK_SHADER_STAGE_FRAGMENT_BIT, // VkShaderStageFlagBits        ShaderStage
        fragment_shader_module,      // VkShaderModule               ShaderModule
        "main",                       // char const                 * EntryPointName
        nullptr                       // VkSpecializationInfo const * SpecializationInfo
      }
  };

  std::vector<VkPipelineShaderStageCreateInfo> shader_stage_create_infos;
  SpecifyPipelineShaderStages(shader_stage_params, shader_stage_create_infos);

  std::vector<VkVertexInputBindingDescription> vertex_input_binding_descriptions = {
    {
      0,                            // uint32_t                     binding
      6 * sizeof(float),          // uint32_t                     stride
      VK_VERTEX_INPUT_RATE_VERTEX   // VkVertexInputRate            inputRate
    }
  };

  std::vector<VkVertexInputAttributeDescription> vertex_attribute_descriptions = {
    {
      0,                            // uint32_t                     location
      0,                            // uint32_t                     binding
      VK_FORMAT_R32G32B32_SFLOAT,   // VkFormat                     format
      0                             // uint32_t                     offset
    },
    {
      1,
      0,
      VK_FORMAT_R32G32B32_SFLOAT,
      3 * sizeof(float)
    }
  };

  VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info;
  VulkanInterface::SpecifyPipelineVertexInputState(vertex_input_binding_descriptions, vertex_attribute_descriptions, vertex_input_state_create_info);

  VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info;
  VulkanInterface::SpecifyPipelineInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, false, input_assembly_state_create_info);

  VulkanInterface::ViewportInfo viewport_infos = {
    {                     // std::vector<VkViewport>   Viewports
      {
        0.0f,               // float          x
        0.0f,               // float          y
        500.0f,             // float          width
        500.0f,             // float          height
        0.0f,               // float          minDepth
        1.0f                // float          maxDepth
      }
    },
    {                     // std::vector<VkRect2D>     Scissors
      {
        {                   // VkOffset2D     offset
          0,                  // int32_t        x
          0                   // int32_t        y
        },
        {                   // VkExtent2D     extent
          500,                // uint32_t       width
          500                 // uint32_t       height
        }
      }
    }
  };
  VkPipelineViewportStateCreateInfo viewport_state_create_info;
  VulkanInterface::SpecifyPipelineViewportAndScissorTestState(viewport_infos, viewport_state_create_info);

  VkPipelineRasterizationStateCreateInfo rasterization_state_create_info;
  VulkanInterface::SpecifyPipelineRasterisationState(false, false, VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, false, 0.0f, 0.0f, 0.0f, 1.0f, rasterization_state_create_info);

  VkPipelineMultisampleStateCreateInfo multisample_state_create_info;
  VulkanInterface::SpecifyPipelineMultisampleState(VK_SAMPLE_COUNT_1_BIT, false, 0.0f, nullptr, false, false, multisample_state_create_info);

  std::vector<VkPipelineColorBlendAttachmentState> attachment_blend_states = {
    {
      false,                          // VkBool32                 blendEnable
      VK_BLEND_FACTOR_ONE,            // VkBlendFactor            srcColorBlendFactor
      VK_BLEND_FACTOR_ONE,            // VkBlendFactor            dstColorBlendFactor
      VK_BLEND_OP_ADD,                // VkBlendOp                colorBlendOp
      VK_BLEND_FACTOR_ONE,            // VkBlendFactor            srcAlphaBlendFactor
      VK_BLEND_FACTOR_ONE,            // VkBlendFactor            dstAlphaBlendFactor
      VK_BLEND_OP_ADD,                // VkBlendOp                alphaBlendOp
      VK_COLOR_COMPONENT_R_BIT |      // VkColorComponentFlags    colorWriteMask
      VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT |
      VK_COLOR_COMPONENT_A_BIT
    }
  };
  VkPipelineColorBlendStateCreateInfo blend_state_create_info;
  VulkanInterface::SpecifyPipelineBlendState(false, VK_LOGIC_OP_COPY, attachment_blend_states, { 1.0f, 1.0f, 1.0f, 1.0f }, blend_state_create_info);

  std::vector<VkDynamicState> dynamic_states = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
  };
  VkPipelineDynamicStateCreateInfo dynamic_state_create_info;
  VulkanInterface::SpecifyPipelineDynamicStates(dynamic_states, dynamic_state_create_info);

  //VkPushConstantRange pushConstantRangeVertex;
  //pushConstantRangeVertex.offset = 0;
  //pushConstantRangeVertex.size = sizeof(PushConstantVertexShaderObject);
  //pushConstantRangeVertex.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  //VkPushConstantRange pushConstantRangeFragment;
  //pushConstantRangeFragment.offset = 0;
  //pushConstantRangeFragment.size = sizeof(PushConstantFragmentShaderObject);
  //pushConstantRangeFragment.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  if (!VulkanInterface::CreatePipelineLayout(*vulkanDevice
    , { descriptorSetLayout}
    , { /*pushConstantRangeFragment*/ }
    , graphicsPipelineLayout)
    ) 
  {
    return false;
  }

  VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo;
  VulkanInterface::SpecifyPipelineDepthAndStencilState(true, true, VK_COMPARE_OP_LESS_OR_EQUAL, false, 0.f, 1.f, false, {}, {}, depthStencilStateCreateInfo);

  VkGraphicsPipelineCreateInfo graphics_pipeline_create_info;
  VulkanInterface::SpecifyGraphicsPipelineCreationParameters(0
    , shader_stage_create_infos
    , vertex_input_state_create_info
    , input_assembly_state_create_info
    , nullptr
    , &viewport_state_create_info
    , rasterization_state_create_info
    , &multisample_state_create_info
    , &depthStencilStateCreateInfo
    , &blend_state_create_info
    , &dynamic_state_create_info
    , graphicsPipelineLayout
    , renderPass
    , 0
    , VK_NULL_HANDLE
    , -1
    , graphics_pipeline_create_info
  );

  std::vector<VkPipeline> graphics_pipeline;
  if (!VulkanInterface::CreateGraphicsPipelines(*vulkanDevice, { graphics_pipeline_create_info }, VK_NULL_HANDLE, graphics_pipeline)) {
    return false;
  }

  graphicsPipeline = graphics_pipeline[0];

  VulkanInterface::DestroyShaderModule(*vulkanDevice, vertex_shader_module);
  VulkanInterface::DestroyShaderModule(*vulkanDevice, fragment_shader_module);

  return true;
}

bool ComputeApp::setupFrameResources()
{  
  if (!VulkanInterface::CreateCommandPool(*vulkanDevice, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, graphicsQueueParameters.familyIndex, frameResourcesCmdPool))
  {
    return false;
  }

  if (!VulkanInterface::AllocateCommandBuffers(*vulkanDevice, frameResourcesCmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, numFrames, frameResourcesCmdBuffers))
  {
    return false;
  }

  for (uint32_t i = 0; i < numFrames; i++)
  {
    VulkanHandle(VkSemaphore) imageAcquiredSemaphore;
    VulkanInterface::InitVulkanHandle(vulkanDevice, imageAcquiredSemaphore);
    VulkanHandle(VkSemaphore) readyToPresentSemaphore;
    VulkanInterface::InitVulkanHandle(vulkanDevice, readyToPresentSemaphore);
    VulkanHandle(VkFence) drawingFinishedFence;
    VulkanInterface::InitVulkanHandle(vulkanDevice, drawingFinishedFence);
    VulkanHandle(VkImageView) depthAttachment;
    VulkanInterface::InitVulkanHandle(vulkanDevice, depthAttachment);

    depthImages.emplace_back(VkImage());

    if (!VulkanInterface::CreateSemaphore(*vulkanDevice, *imageAcquiredSemaphore))
    {
      return false;
    }
    if (!VulkanInterface::CreateSemaphore(*vulkanDevice, *readyToPresentSemaphore))
    {
      return false;
    }
    if (!VulkanInterface::CreateFence(*vulkanDevice, true, *drawingFinishedFence))
    {
      return false;
    }

    //auto cbuf = commandPools->framePools.getBuffer(i);
    

    frameResources.push_back(
      {
        frameResourcesCmdBuffers[i],
        std::move(imageAcquiredSemaphore),
        std::move(readyToPresentSemaphore),
        std::move(drawingFinishedFence),
        std::move(depthAttachment),
        VulkanHandle(VkFramebuffer)()
      }
    );

    //cbuf.first->unlock();

    VmaAllocation depthAllocation;

    if (!VulkanInterface::Create2DImageAndView(*vulkanDevice
      , allocator
      , VK_FORMAT_D16_UNORM
      , swapchain.size
      , 1, 1
      , VK_SAMPLE_COUNT_1_BIT
      , VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
      , VK_IMAGE_ASPECT_DEPTH_BIT
      , depthImages.back()
      , *frameResources[i].depthAttachment
      , VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT
      , VMA_MEMORY_USAGE_GPU_ONLY
      , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
      , VK_NULL_HANDLE
      , depthAllocation))
    {
      return false;
    }

    depthImagesAllocations.push_back(depthAllocation);
  }

  if (!VulkanInterface::CreateFramebuffersForFrameResources(*vulkanDevice, renderPass, swapchain, frameResources))
  {
    return false;
  }

  return true;
}

bool ComputeApp::setupChunkManager()
{
  chunkManager = std::make_unique<ChunkManager>(registry.get(), &allocator, &*vulkanDevice);

  return true;
}

bool ComputeApp::setupTerrainGenerator()
{
  terrainGen = std::make_unique<TerrainGenerator>();

  terrainGen->SetSeed(4422);

  return true;
}

bool ComputeApp::setupSurfaceExtractor()
{
  surfaceExtractor = std::make_unique<SurfaceExtractor>(&*vulkanDevice, &transferQueue, &transferQMutex, commandPools.get());

  return true;
}

bool ComputeApp::setupECS()
{
  registry = std::make_unique<entt::registry<>>();

  return true;
}

void ComputeApp::shutdownVulkanMemoryAllocator()
{
  if (allocator)
  {
    for (int i = 0; i < depthImages.size(); i++)
    {
      vmaDestroyImage(allocator, depthImages[i], depthImagesAllocations[i]);
    }

    vmaDestroyAllocator(allocator);
    allocator = VK_NULL_HANDLE;
  }
}

void ComputeApp::shutdownChunkManager()
{
  chunkManager->shutdown();
}

void ComputeApp::shutdownGraphicsPipeline()
{
  vmaDestroyBuffer(allocator, viewprojUBuffer, viewprojAlloc);
  vmaDestroyBuffer(allocator, modelUBuffer, modelAlloc);
  vmaDestroyBuffer(allocator, lightUBuffer, lightAlloc);
  //graphicsPipeline->cleanup();
}

void ComputeApp::shutdownImGui()
{
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();
}

void ComputeApp::cleanupVulkan()
{
  // VulkanHandle is useful for catching forgotten objects and ones with scoped/short-lifetimes, 
  // but for the final shutdown we need to be explicit
  for (auto & frameRes : frameResources)
  {
    VulkanInterface::DestroySemaphore(*vulkanDevice, *frameRes.imageAcquiredSemaphore);
    VulkanInterface::DestroySemaphore(*vulkanDevice, *frameRes.readyToPresentSemaphore);
    VulkanInterface::DestroyFence(*vulkanDevice, *frameRes.drawingFinishedFence);
    VulkanInterface::DestroyImageView(*vulkanDevice, *frameRes.depthAttachment);
    VulkanInterface::DestroyFramebuffer(*vulkanDevice, *frameRes.framebuffer);
  }
  VulkanInterface::DestroySwapchain(*vulkanDevice, *swapchain.handle);
  for (auto & imageView : swapchain.imageViews)
  {
    VulkanInterface::DestroyImageView(*vulkanDevice, *imageView);
  }  

  VulkanInterface::DestroyCommandPool(*vulkanDevice, frameResourcesCmdPool);

  VulkanInterface::DestroyDescriptorPool(*vulkanDevice, imGuiDescriptorPool);
  VulkanInterface::DestroyRenderPass(*vulkanDevice, renderPass);

  // Cleanup command buffers
  commandPools->cleanup();

  // Shutdown inline graphics pipeline  
  VulkanInterface::DestroyDescriptorSetLayout(*vulkanDevice, descriptorSetLayout);
  VulkanInterface::DestroyDescriptorPool(*vulkanDevice, descriptorPool);
  VulkanInterface::DestroyPipeline(*vulkanDevice, graphicsPipeline);
  VulkanInterface::DestroyPipelineLayout(*vulkanDevice, graphicsPipelineLayout);

  vkDestroyDevice(*vulkanDevice, nullptr); *vulkanDevice = VK_NULL_HANDLE;

#if defined(_DEBUG)
  vkDestroyDebugUtilsMessengerEXT(*vulkanInstance, callback, nullptr); callback = VK_NULL_HANDLE;
#endif

  vkDestroySurfaceKHR(*vulkanInstance, *presentationSurface, nullptr); *presentationSurface = VK_NULL_HANDLE;
  vkDestroyInstance(*vulkanInstance, nullptr); *vulkanInstance = VK_NULL_HANDLE;
  VulkanInterface::ReleaseVulkanLoaderLibrary(vulkanLibrary);
}

void ComputeApp::OnMouseEvent()
{

}

void ComputeApp::updateUser()
{
  bool moveLeft = (KeyboardState.Keys['A'].IsDown || KeyboardState.Keys[VK_LEFT].IsDown);
  bool moveRight = (KeyboardState.Keys['D'].IsDown || KeyboardState.Keys[VK_RIGHT].IsDown);
  bool moveForward = (KeyboardState.Keys['W'].IsDown || KeyboardState.Keys[VK_UP].IsDown);
  bool moveBackwards = (KeyboardState.Keys['S'].IsDown || KeyboardState.Keys[VK_DOWN].IsDown);
  bool moveUp = (KeyboardState.Keys['Q'].IsDown);
  bool moveDown = (KeyboardState.Keys['E'].IsDown);
  bool toggleMouseLock = (KeyboardState.Keys[VK_F1].IsDown);

  if (toggleMouseLock)
  {
    if (gameTime >= settingsLastChangeTimes.toggleMouseLock + buttonPressGracePeriod)
    {
      lockMouse = !lockMouse;
      settingsLastChangeTimes.toggleMouseLock = static_cast<float>(gameTime);
    }
  }

  float dt = TimerState.GetDeltaTime();
  camera.SetFrameTime(dt);
  // Handle movement controls
  auto camPos = camera.GetPosition();
  if (moveLeft || moveRight)
  {
    if (moveLeft && !moveRight)
    {
      camera.StrafeLeft();
    }
    else if (moveRight && !moveLeft)
    {
      camera.StrafeRight();
    }
  }
  if (moveForward || moveBackwards)
  {
    if (moveForward && !moveBackwards)
    {
      camera.MoveForward();
    }
    else if (moveBackwards && !moveForward)
    {
      camera.MoveBackward();
    }
  }
  if (moveUp || moveDown)
  {
    if (moveUp && !moveDown)
    {
      camera.MoveUpward();
    }
    else if (moveDown && !moveUp)
    {
      camera.MoveDownward();
    }
  }

  // Handle look controls
  if (lockMouse)
  {  
    mouseDelta = { MouseState.Position.X - swapchain.size.width / 2, MouseState.Position.Y - swapchain.size.height / 2 };
    //std::cout << mouseDelta.x << ", " << mouseDelta.y << std::endl;
    camera.Turn(-mouseDelta.x, mouseDelta.y);
  }

  //camera.SetPosition(camPos);

  camera.Update();

  if (lockMouse)
  {
    POINT pt; 
    pt.x = static_cast<LONG>(screenCentre.x);
    pt.y = static_cast<LONG>(screenCentre.y);
    ClientToScreen(static_cast<HWND>(hWnd), &pt);
    SetCursorPos(pt.x, pt.y);
  }
}

void ComputeApp::checkForNewChunks()
{
  chunkManager->despawnChunks(camera.GetPosition());
  auto chunkList = chunkManager->getChunkSpawnList(camera.GetPosition());
  for (auto & chunk : chunkList)
  {
    //std::cout << chunk.first << std::endl;
    if (chunk.second == ChunkManager::ChunkStatus::NotLoadedCached)
    {
      computeTaskflow->emplace([=]() {
        loadFromChunkCache(chunk.first);
      });
    }
    else if (chunk.second == ChunkManager::ChunkStatus::NotLoadedNotCached)
    {      
      computeTaskflow->emplace([=]() {
        //std::cout << chunk.first << std::endl;
        generateChunk(chunk.first);
      });
    }
  }
  computeTaskflow->dispatch();
}

void ComputeApp::getChunkRenderList()
{
  constexpr float screenDepth = static_cast<float>(TechnicalChunkDim * chunkViewDistance);
  camera.GetViewMatrix(view);
  proj = glm::perspective(glm::radians(80.f), static_cast<float>(swapchain.size.width) / static_cast<float>(swapchain.size.height), 0.001f, screenDepth);
  proj[1][1] *= -1; // Correct projection for vulkan

  frustum.Construct(screenDepth, proj, view);

  chunkRenderList.clear();
  chunkRenderList.reserve(343); // Revise size when frustum culling implemented
  int i = 0;
  registry->view<WorldPosition, VolumeData, ModelData, AABB>().each(
    [=, &i=i, &registry=registry, &chunkRenderList=chunkRenderList](const uint32_t entity, auto&&...)
    {
      //if (chunkIsWithinFrustum(entity) && registry->get<ModelData>(entity).indexCount > 0) // dummy check
      auto[pos, modelData] = registry->get<WorldPosition, ModelData>(entity);

      if (modelData.indexCount > 0 && chunkIsWithinFrustum(entity))
      {
        chunkRenderList.push_back(entity);

        // Calculate model matrix for chunk
        VmaAllocationInfo modelInfo;
        vmaGetAllocationInfo(allocator, modelAlloc, &modelInfo);
        char * chunkDataPtr = static_cast<char*>(modelInfo.pMappedData);

        glm::mat4 model = glm::translate(glm::mat4(1.f), pos.pos);

        PerChunkData data = {
          model
        };
        memcpy(&chunkDataPtr[i*dynamicAlignment], &data, sizeof(PerChunkData));
        i++; // Move to next aligned position
      }      
    }
  );

  vmaFlushAllocation(allocator, modelAlloc, 0, VK_WHOLE_SIZE);

  //std::cout << chunkRenderList.size() << std::endl;
}

void ComputeApp::recordChunkDrawCalls()
{
  //std::mutex drawCallVectorMutex;
  //VkCommandBufferInheritanceInfo info = {
  //  VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
  //  nullptr,
  //  renderPass,
  //  0,
  //  *(frameResources[nextFrameIndex].framebuffer),
  //  VK_FALSE,
  //  0,
  //  0
  //};
  //glm::mat4 view = glm::lookAt(camera.GetPosition(), camera.GetLookAt(), camera.GetUp());
  //glm::mat4 proj = glm::perspective(glm::radians(90.f), static_cast<float>(swapchain.size.width) / static_cast<float>(swapchain.size.height), 0.01f, static_cast<float>(TechnicalChunkDim * chunkViewDistance));
  //proj[1][1] *= -1; // Correct projection for vulkan
  //glm::mat4 vp = proj * view;
  //recordedChunkDrawCalls.clear();
  //for (auto & chunk : chunkRenderList)
  //{  
  //  graphicsTaskflow->emplace([&]()
  //  {
  //    auto cbuf = drawChunkOp(chunk, &info, vp);
  //    drawCallVectorMutex.lock();
  //    recordedChunkDrawCalls.push_back(cbuf);
  //    drawCallVectorMutex.unlock();
  //  });
  //}

  //graphicsTaskflow->dispatch().get();
}

bool ComputeApp::drawChunks()
{
  auto framePrep = [&](VkCommandBuffer commandBuffer, uint32_t imageIndex, VkFramebuffer framebuffer)
  {
    //assert(imageIndex == nextFrameIndex);

    if (chunkRenderList.size() > 0)
    {
      if (!VulkanInterface::BeginCommandBufferRecordingOp(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr))
      {
        //mutex->unlock();
        return false;
      }

      if (presentQueueParameters.familyIndex != graphicsQueueParameters.familyIndex)
      {
        VulkanInterface::ImageTransition imageTransitionBeforeDrawing = {
          swapchain.images[imageIndex],
          VK_ACCESS_MEMORY_READ_BIT,
          VK_ACCESS_MEMORY_READ_BIT,
          VK_IMAGE_LAYOUT_UNDEFINED,
          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
          presentQueueParameters.familyIndex,
          graphicsQueueParameters.familyIndex,
          VK_IMAGE_ASPECT_COLOR_BIT
        };

        VulkanInterface::SetImageMemoryBarrier(commandBuffer
          , VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
          , VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
          , { imageTransitionBeforeDrawing }
        );
      }

      // Draw
      VulkanInterface::BeginRenderPass(commandBuffer, renderPass, framebuffer
        , { {0,0}, swapchain.size } // Render Area (full frame size)
        , { {0.1f, 0.2f, 0.3f, 1.f}, {1.f, 0.f } } // Clear Color, one for our draw area, one for our depth stencil
        //, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS
        , VK_SUBPASS_CONTENTS_INLINE
      );

      //VulkanInterface::ExecuteSecondaryCommandBuffers(commandBuffer, recordedChunkDrawCalls);

      VulkanInterface::BindPipelineObject(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

      VkViewport viewport = {
        0.f,
        0.f,
        static_cast<float>(swapchain.size.width),
        static_cast<float>(swapchain.size.height),
        0.f,
        1.f
      };
      VulkanInterface::SetViewportStateDynamically(commandBuffer, 0, { viewport });

      VkRect2D scissor = {
        {
          0, 0
        },
        {
          swapchain.size.width,
          swapchain.size.height
        }
      };
      VulkanInterface::SetScissorStateDynamically(commandBuffer, 0, { scissor });      

      VmaAllocationInfo viewprojInfo;
      vmaGetAllocationInfo(allocator, viewprojAlloc, &viewprojInfo);      
      VmaAllocationInfo lightInfo;
      vmaGetAllocationInfo(allocator, lightAlloc, &lightInfo);      

      void * viewprojPtr, *modelPtr, *lightPtr;

      ViewProj viewProj = {
        view,
        proj
      };

      vmaMapMemory(allocator, viewprojAlloc, &viewprojPtr);      
      memcpy(viewprojPtr, &viewProj, sizeof(ViewProj));
      vmaUnmapMemory(allocator, viewprojAlloc);
      vmaFlushAllocation(allocator, viewprojAlloc, 0, VK_WHOLE_SIZE);

      for (int i = 0; i < chunkRenderList.size(); i++)
      {        
        ModelData modelData = registry->get<ModelData>(chunkRenderList[i]);           

        uint32_t dynamicOffset = i * static_cast<uint32_t>(dynamicAlignment);
        VulkanInterface::BindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineLayout, 0, descriptorSets, { dynamicOffset });

        VulkanInterface::BindVertexBuffers(commandBuffer, 0, { {modelData.vertexBuffer, 0} });
        VulkanInterface::BindIndexBuffer(commandBuffer, modelData.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        VulkanInterface::DrawIndexedGeometry(commandBuffer, modelData.indexCount, 1, 0, 0, 0);
      }

      //ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

      VulkanInterface::EndRenderPass(commandBuffer);

      if (presentQueueParameters.familyIndex != graphicsQueueParameters.familyIndex)
      {
        VulkanInterface::ImageTransition imageTransitionBeforePresent = {
          swapchain.images[imageIndex],
          VK_ACCESS_MEMORY_READ_BIT,
          VK_ACCESS_MEMORY_READ_BIT,
          VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
          VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
          graphicsQueueParameters.familyIndex,
          presentQueueParameters.familyIndex,
          VK_IMAGE_ASPECT_COLOR_BIT
        };

        VulkanInterface::SetImageMemoryBarrier(commandBuffer
          , VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
          , VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
          , { imageTransitionBeforePresent });
      }

      if (!VulkanInterface::EndCommandBufferRecordingOp(commandBuffer))
      {
        //mutex->unlock();
        return false;
      }

    }

    //mutex->unlock();
    return true;
  };

  //auto ret = commandPools->framePools.getBuffer(nextFrameIndex);
  //std::mutex * const mutex = ret.first;
  //VkCommandBuffer * const commandBuffer = frameResources[;

  if (chunkRenderList.size() > 0)
  {
    LightData lightData;
    lightData.lightDir = glm::vec3(-0.2f, -1.0f, -0.3f);
    lightData.viewPos = camera.GetPosition();
    lightData.lightAmbientColour = glm::vec3(0.2f, 0.2f, 0.2f);
    lightData.lightDiffuseColour = glm::vec3(0.5f, 0.5f, 0.5f);
    lightData.lightSpecularColour = glm::vec3(1.f, 1.f, 1.f);
    lightData.objectColour = glm::vec3(1.f, 0.0f, 1.0f);

    auto[mutex, transferCmdBuf] = commandPools->transferPools.getBuffer(nextFrameIndex);

    if (!VulkanInterface::UseStagingBufferToUpdateBufferWithDeviceLocalMemoryBound(
       *vulkanDevice
      , allocator
      , sizeof(LightData)
      , &lightData
      , lightUBuffer
      , 0
      , 0
      , VK_ACCESS_UNIFORM_READ_BIT
      , VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
      , VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
      , transferQueue
      , &transferQMutex
      , *transferCmdBuf
      , {}))
    {
      mutex->unlock();
      return false;
    }
    mutex->unlock();
   
    return VulkanInterface::RenderWithFrameResources(*vulkanDevice
      , graphicsQueue
      , presentQueue
      , *swapchain.handle
      , {}
      , framePrep
      //, mutex
      //, commandBuffer
      , frameResources
      , nextFrameIndex);
  }
  else
  {
    //mutex->unlock();
    return true;
  }
}

bool ComputeApp::chunkIsWithinFrustum(uint32_t const entity)
{
  auto[pos, aabb] = registry->get<WorldPosition, AABB>(entity);
  return frustum.CheckRectangle(pos.pos, { aabb.wdith, aabb.height, aabb.depth });
  //return frustum.CheckSphere(pos.pos, aabb.wdith);
}

void ComputeApp::loadFromChunkCache(EntityHandle handle)
{
  auto pos = registry->get<WorldPosition>(handle).pos;
  ChunkCacheData data;
  if (chunkManager->getChunkVolumeDataFromCache(chunkManager->chunkKey(pos), data)) // Retrieve data from cache
  {
    auto[volume, model] = registry->get<VolumeData, ModelData>(handle);
    //void * ptr;
    //vmaMapMemory(*volume.allocator, volume.volumeAllocation, &ptr);
    //memcpy(ptr, data.data(), data.size());
    //VmaAllocationInfo info;
    //vmaGetAllocationInfo(*volume.allocator, volume.volumeAllocation, &info);
    //vmaFlushAllocation(*volume.allocator, volume.volumeAllocation, 0, VK_WHOLE_SIZE);
    //vmaUnmapMemory(*volume.allocator, volume.volumeAllocation);

    volume.volume = data;

    surfaceExtractor->extractSurface(volume, model, nextFrameIndex);
  }
  else // Chunk has fallen out of the cache
  {
    generateChunk(handle);
  }
}

void ComputeApp::generateChunk(EntityHandle handle)
{
  if (!ready) return; // Catch if we're about to shutdown

  //std::cout << handle << std::endl;
  auto[volume, pos, model] = registry->get<VolumeData, WorldPosition, ModelData>(handle);

  auto chunkData = terrainGen->getChunkVolume(pos.pos);
  volume.volume = chunkData;
  /*if (!volume.init(&allocator, &vulkanPhysicalDevice, &*vulkanDevice))
  {
    abort();
  }*/

  /*void * ptr;
  vmaMapMemory(*volume.allocator, volume.volumeAllocation, &ptr);
  memcpy(ptr, chunkData.data(), chunkData.size());
  VmaAllocationInfo info;
  vmaGetAllocationInfo(*volume.allocator, volume.volumeAllocation, &info);
  vmaFlushAllocation(*volume.allocator, volume.volumeAllocation, 0, VK_WHOLE_SIZE);
  vmaUnmapMemory(*volume.allocator, volume.volumeAllocation);*/

  surfaceExtractor->extractSurface(volume, model, nextFrameIndex);
  std::cout << handle << " generated, " << model.indexCount/3 << " triangles\n";
}

VkCommandBuffer ComputeApp::drawChunkOp(EntityHandle chunk, VkCommandBufferInheritanceInfo * const inheritanceInfo, glm::mat4 vp)
{
  //WorldPosition pos = registry->get<WorldPosition>(chunk); // Should position be part of model data? Like a transform component?
  //ModelData modelData = registry->get<ModelData>(chunk);

  //auto[mutex, cmdBuf] = commandPools->graphicsPools.getBuffer(nextFrameIndex);

  ////VkCommandBuffer * cmdBuf = chunkCommandBufferStacks[nextFrameIndex].top();
  ////                           chunkCommandBufferStacks[nextFrameIndex].pop();

  //if (!VulkanInterface::BeginCommandBufferRecordingOp(*cmdBuf, VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT, inheritanceInfo))
  //{
  //  mutex->unlock();
  //  return false;
  //}

  //VulkanInterface::BindPipelineObject(*cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

  //VkViewport viewport = {
  //  0.f,
  //  0.f,
  //  static_cast<float>(swapchain.size.width),
  //  static_cast<float>(swapchain.size.height),
  //  0.f,
  //  1.f
  //};
  //VulkanInterface::SetViewportStateDynamically(*cmdBuf, 0, { viewport });

  //VkRect2D scissor = {
  //  {
  //    0, 0
  //  },
  //  {
  //    swapchain.size.width,
  //    swapchain.size.height
  //  }
  //};
  //VulkanInterface::SetScissorStateDynamically(*cmdBuf, 0, { scissor });

  //PushConstantObject push;

  //glm::mat4 model = glm::translate(glm::mat4(1.f), pos.pos);
  //push.mvp = vp * model;

  //VulkanInterface::BindVertexBuffers(*cmdBuf, 0, { {modelData.vertexBuffer, 0} });
  //VulkanInterface::BindIndexBuffer(*cmdBuf, modelData.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
  //VulkanInterface::ProvideDataToShadersThroughPushConstants(
  //   *cmdBuf
  //  , graphicsPipelineLayout
  //  , VK_SHADER_STAGE_VERTEX_BIT
  //  , 0
  //  , sizeof(PushConstantObject)
  //  , &push);

  //VulkanInterface::DrawIndexedGeometry(*cmdBuf, modelData.indexCount, 1, 0, 0, 0);

  //if (!VulkanInterface::EndCommandBufferRecordingOp(*cmdBuf))
  //{
  //  mutex->unlock();
  //  return false;
  //}

  //mutex->unlock();
  //return *cmdBuf;
  return VK_NULL_HANDLE;
}

void ComputeApp::Shutdown()
{
  if (ready)
  {
    computeTaskflow->wait_for_all();
    VulkanInterface::WaitForAllSubmittedCommandsToBeFinished(*vulkanDevice);

    // We can shutdown some systems in parallel since they don't depend on each other
    auto[vulkan, vma, imgui, chnkMngr, gpipe] = systemTaskflow->emplace(
      [&]() { cleanupVulkan(); },
      [&]() { shutdownVulkanMemoryAllocator(); },
      [&]() { shutdownImGui(); },
      [&]() { shutdownChunkManager(); },
      [&]() { shutdownGraphicsPipeline(); }
    );

    // Task dependencies
    vma.precede(vulkan);
    imgui.precede(vulkan);
    chnkMngr.precede(vulkan);
    chnkMngr.precede(vma);
    gpipe.precede(vulkan);
    gpipe.precede(vma);

    systemTaskflow->dispatch().get();
  }
}
