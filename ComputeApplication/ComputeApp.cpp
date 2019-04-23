#include "ComputeApp.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include "coordinatewrap.hpp"
#include "syncout.hpp"
#include <random>

bool ComputeApp::Initialise(VulkanInterface::WindowParameters windowParameters)
{
  // Setup some basic data
  gameTime = 0.0;
  settingsLastChangeTimes = { 0.f };
  lockMouse = false;
  reseedTerrain = false;
  camera.SetPosition({ 0.f, 32.f, 0.f });
  camera.LookAt({ 0.f, 0.f, 0.f }, { 0.f, 0.f, 1.f }, { 0.f, 1.f, 0.f });  
  nextFrameIndex = 0;

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
  bool resVMA, resCmdBufs, resRenderpass, resGpipe, resChnkMngr, resTerGen, resECS, resSurface;
  auto[vma, commandBuffers, renderpass, gpipeline, frameres, chunkmanager, terraingen, surface, ecs] = systemTaskflow->emplace(
    [&]() { resVMA = initialiseVulkanMemoryAllocator(); },
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
  ecs.precede(chunkmanager);
  commandBuffers.precede(surface);
  commandBuffers.precede(frameres);

  // Execute and wait for completion
  systemTaskflow->dispatch().get();  

  if (!resVMA || !resCmdBufs || !resRenderpass || !resGpipe || !resChnkMngr || !resTerGen || !resSurface || !resECS)
  {
    return false;
  }

  return true;
}

bool ComputeApp::InitMetrics()
{
  logging = true;
  logFile = createLogFile();
  
  if (!logFile.is_open())
  {
    return false;
  }
  else
  {
    // Write data headings to first line, csv format
    logFile << "key,heightElapsed,volumeElapsed,surfaceElapsed,timeElapsed,timeSinceRegistered" << std::endl;
    return true;
  }
}

bool ComputeApp::Update()
{
  gameTime += TimerState.GetDeltaTime();

  if (reseedTerrain)
  {
    syncout() << "Reseeding terrain, wait for device idle\n";
    vkDeviceWaitIdle(*vulkanDevice); // Wait for idle (eck)
    syncout() << "Waiting for compute taskflow to complete\n";
    computeTaskflow->wait_for_all(); // Flush compute tasks
    chunkManager->clear(); // Destroy old chunks
    syncout() << "Reseeding terrain generator" << std::endl;
    terrainGen->SetSeed(std::random_device()()); // Reseed terrain generator
    reseedTerrain = false;
    // Then continue as usual
  }

  auto[userUpdate, spawnChunks, renderList] = updateTaskflow->emplace(
    [&]() { updateUser(); },
    [&]() { checkForNewChunks(); },
    [&]() { getChunkRenderList(); }
  );

  userUpdate.precede(spawnChunks);
  userUpdate.precede(renderList);
  spawnChunks.precede(renderList);

  updateTaskflow->dispatch().get();
  {
    static float updateRefreshTimer = 0.f;
    updateRefreshTimer += TimerState.GetDeltaTime();
    if (updateRefreshTimer > 30.f)
    {
      updateTaskflow.reset(new tf::Taskflow(2));
      updateRefreshTimer = 0.f;
    }
  }
  drawChunks();

  return true;
}

bool ComputeApp::Resize()
{
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
#if defined(_DEBUG) || defined(RELEASE_MODE_VALIDATION_LAYERS)
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
    syncout() << "Checking Device " << properties.deviceName << std::endl;

    // Try to find a device which supports all our desired capabilities (graphics, compute, present);
    // Make sure we have graphics on this devie
    if (!VulkanInterface::SelectIndexOfQueueFamilyWithDesiredCapabilities(physicalDevice, VK_QUEUE_GRAPHICS_BIT, graphicsQueueParameters.familyIndex))
    {
      syncout() << "Does not support Graphics Queue" << std::endl;
      continue;
    }

    if (!VulkanInterface::SelectQueueFamilyThatSupportsPresentationToGivenSurface(physicalDevice, *presentationSurface, presentQueueParameters.familyIndex))
    {
      syncout() << "Does not support present" << std::endl;
      continue;
    }

    // Check how many concurrent threads we have available (like 4 or 8, depending on cpu)
    auto numConcurrentThreads = std::thread::hardware_concurrency();
    // We want to reserve one for rendering and allocate the rest for compute
    uint32_t numComputeThreads = numConcurrentThreads - 1;
    if (numComputeThreads < 1)
    {
      syncout() << "Number of compute threads is too low" << std::endl;
      return false; // Bail, this isn't going to end well with a single thread    
    }

    if (graphicsQueueParameters.familyIndex == computeQueueParameters.familyIndex
      && graphicsQueueParameters.familyIndex == presentQueueParameters.familyIndex)
      // Yay, probably a nvidia GPU, all queues are capable at doing everything, only possible thing to improve on this 
      // is to find if we have a dedicated transfer queue and work out how to use that properly. 
      // Will use the same queue family for all commands for now.
    {
      std::vector<float> queuePriorities = { 1.f, 1.f, 1.f }; // One for the graphics queue, one for transfer, one for the present queue

      // Check how many compute queues we can have
      std::vector<VkQueueFamilyProperties> queueFamilies;
      if (!VulkanInterface::CheckAvailableQueueFamiliesAndTheirProperties(physicalDevice, queueFamilies))
      {
        syncout() << "Failed to check available queue families" << std::endl;
        return false;
      }

      // Construct requestedQueues vector
      std::vector<VulkanInterface::QueueInfo> requestedQueues = {
        {graphicsQueueParameters.familyIndex, queuePriorities} // One graphics queue, one transfer queue
      };

      VulkanInterface::InitVulkanHandle(vulkanDevice);
      if (!VulkanInterface::CreateLogicalDevice(physicalDevice, requestedQueues, desiredDeviceExtensions, desiredLayers, &desiredDeviceFeatures, *vulkanDevice))
      {
        syncout() << "Failed to create logical device" << std::endl;
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
        vkGetDeviceQueue(*vulkanDevice, graphicsQueueParameters.familyIndex, 2, &presentQueue);
        break;
      }
    }
    else // A more involved setup... coming soon to an application near you
    {
      syncout() << "Unsupported graphics card :(" << std::endl;
      continue;
    }
  }
  // Check we actually created a device...
  if (!vulkanDevice)
  {
    syncout() << "Failed to create a vulkan device" << std::endl;
    cleanupVulkan();
    return false;
  }

  // Create that swapchain
  if (!createSwapchain( VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
                      , true
                      , VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT))
  {
    syncout() << "Failed to create swapchain" << std::endl;
    return false;
  }

  return true;
}

bool ComputeApp::setupTaskflow()
{
  tfExecutor = std::make_shared<tf::Taskflow::Executor>(std::thread::hardware_concurrency()); // maybe -1?

  updateTaskflow = std::make_unique<tf::Taskflow>(2);
  graphicsTaskflow = std::make_unique<tf::Taskflow>(std::thread::hardware_concurrency()-2);
  computeTaskflow = std::make_unique<tf::Taskflow>(std::thread::hardware_concurrency()-2);
  systemTaskflow = std::make_unique<tf::Taskflow>(std::thread::hardware_concurrency());

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

bool ComputeApp::setupCommandPoolAndBuffers()
{
  uint32_t numWorkers = static_cast<uint32_t>(graphicsTaskflow->num_workers());

  commandPools = std::make_unique<TaskflowCommandPools>(
    &*vulkanDevice
    , graphicsQueueParameters.familyIndex
    , numWorkers);

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
  // 1 for each frame index
  viewprojUBuffers.resize(3);
  modelUBuffers.resize(3);
  lightUBuffers.resize(3);
  viewprojAllocs.resize(3);
  modelAllocs.resize(3);
  lightAllocs.resize(3);

  for (int i = 0; i < 3; i++)
  {
    if (!VulkanInterface::CreateBuffer(allocator
      , sizeof(ViewProj)
      , VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
      , viewprojUBuffers[i]
      , VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT
      , VMA_MEMORY_USAGE_UNKNOWN
      , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
      , VK_NULL_HANDLE, viewprojAllocs[i]))
    {
      return false;
    }

    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(vulkanPhysicalDevice, &props);
    size_t deviceAlignment = props.limits.minUniformBufferOffsetAlignment;
    dynamicAlignment = (sizeof(PerChunkData) / deviceAlignment) * deviceAlignment + ((sizeof(PerChunkData) % deviceAlignment) > 0 ? deviceAlignment : 0);

    // Always mapped model buffer for easy copy
    if (!VulkanInterface::CreateBuffer(allocator
      , dynamicAlignment * 256
      , VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
      , modelUBuffers[i]
      , VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT
      , VMA_MEMORY_USAGE_UNKNOWN
      , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
      , VK_NULL_HANDLE, modelAllocs[i]))
    {
      return false;
    }

    if (!VulkanInterface::CreateBuffer(allocator
      , sizeof(LightData)
      , VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
      , lightUBuffers[i]
      , VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT
      , VMA_MEMORY_USAGE_UNKNOWN
      , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
      , VK_NULL_HANDLE, lightAllocs[i]))
    {
      return false;
    }
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
      2*3                                           // uint32_t             descriptorCount
  };
  VkDescriptorPoolSize descriptorPoolSizeUBD = {
      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,          // VkDescriptorType     type
      1*3                                           // uint32_t             descriptorCount
  };

  if (!VulkanInterface::CreateDescriptorPool(*vulkanDevice, false, 3, { descriptorPoolSizeUB, descriptorPoolSizeUBD }, descriptorPool))
  {
    return false;
  }

  if (!VulkanInterface::AllocateDescriptorSets(*vulkanDevice, descriptorPool, { descriptorSetLayout, descriptorSetLayout, descriptorSetLayout }, descriptorSets))
  {
    return false;
  }

  for (int i = 0; i < 3; i++)
  {
    VulkanInterface::BufferDescriptorInfo viewProjDescriptorUpdate = {
        descriptorSets[i],                          // VkDescriptorSet                      TargetDescriptorSet
        0,                                          // uint32_t                             TargetDescriptorBinding
        0,                                          // uint32_t                             TargetArrayElement
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,          // VkDescriptorType                     TargetDescriptorType
        {                                           // std::vector<VkDescriptorBufferInfo>  BufferInfos
          {
            viewprojUBuffers[0],                    // VkBuffer                             buffer
            0,                                      // VkDeviceSize                         offset
            VK_WHOLE_SIZE                           // VkDeviceSize                         range
          }
        }
    };
    VulkanInterface::BufferDescriptorInfo modelDescriptorUpdate = {
        descriptorSets[i],                          // VkDescriptorSet                      TargetDescriptorSet
        1,                                          // uint32_t                             TargetDescriptorBinding
        0,                                          // uint32_t                             TargetArrayElement
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,  // VkDescriptorType                     TargetDescriptorType
        {                                           // std::vector<VkDescriptorBufferInfo>  BufferInfos
          {
            modelUBuffers[0],                       // VkBuffer                             buffer
            0,                                      // VkDeviceSize                         offset
            VK_WHOLE_SIZE                           // VkDeviceSize                         range
          }
        }
    };
    VulkanInterface::BufferDescriptorInfo lightDescriptorUpdate = {
        descriptorSets[i],                          // VkDescriptorSet                      TargetDescriptorSet
        2,                                          // uint32_t                             TargetDescriptorBinding
        0,                                          // uint32_t                             TargetArrayElement
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,          // VkDescriptorType                     TargetDescriptorType
        {                                           // std::vector<VkDescriptorBufferInfo>  BufferInfos
          {
            lightUBuffers[0],                       // VkBuffer                             buffer
            0,                                      // VkDeviceSize                         offset
            VK_WHOLE_SIZE                           // VkDeviceSize                         range
          }
        }
    };

    VulkanInterface::UpdateDescriptorSets(*vulkanDevice, {}, { viewProjDescriptorUpdate, modelDescriptorUpdate, lightDescriptorUpdate }, {}, {});
  }  

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

  if (!VulkanInterface::CreatePipelineLayout(*vulkanDevice
    , { descriptorSetLayout, descriptorSetLayout, descriptorSetLayout }
    , { }
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
  chunkManager = std::make_unique<ChunkManager>(registry.get(), &registryMutex, &allocator, &*vulkanDevice);

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
  registry = std::make_unique<entt::DefaultRegistry>();
  registry->reserve(512);
  
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
  chunkManager->clear();
}

void ComputeApp::shutdownGraphicsPipeline()
{
  for (int i = 0; i < 3; i++)
  {
    vmaDestroyBuffer(allocator, viewprojUBuffers[i], viewprojAllocs[i]);
    vmaDestroyBuffer(allocator, modelUBuffers[i], modelAllocs[i]);
    vmaDestroyBuffer(allocator, lightUBuffers[i], lightAllocs[i]);
  }
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

#if defined(_DEBUG) || defined(RELEASE_MODE_VALIDATION_LAYERS)
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
  bool moveLeft = (KeyboardState.Keys['A'].IsDown);
  bool moveRight = (KeyboardState.Keys['D'].IsDown);
  bool moveForward = (KeyboardState.Keys['W'].IsDown);
  bool moveBackwards = (KeyboardState.Keys['S'].IsDown);
  bool moveUp = (KeyboardState.Keys['Q'].IsDown);
  bool moveDown = (KeyboardState.Keys['E'].IsDown);
  bool toggleMouseLock = (KeyboardState.Keys[VK_F1].IsDown);
  bool lookLeft = KeyboardState.Keys[VK_LEFT].IsDown;
  bool lookRight = KeyboardState.Keys[VK_RIGHT].IsDown;
  bool lookUp = KeyboardState.Keys[VK_UP].IsDown;
  bool lookDown = KeyboardState.Keys[VK_DOWN].IsDown;
  bool reseed = (KeyboardState.Keys[VK_F2].IsDown);

  if (reseed)
  {
    if (gameTime >= settingsLastChangeTimes.reseed + 5.f)
    {
      reseedTerrain = true;
      settingsLastChangeTimes.reseed = static_cast<float>(gameTime);
    }
  }

  if (toggleMouseLock)
  {
    if (gameTime >= settingsLastChangeTimes.toggleMouseLock + buttonPressGracePeriod)
    {
      lockMouse = !lockMouse;
      settingsLastChangeTimes.toggleMouseLock = static_cast<float>(gameTime);
      if (lockMouse)
        while (ShowCursor(FALSE) > 0);
      else 
        ShowCursor(TRUE);
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
    camera.Turn(-mouseDelta.x, -mouseDelta.y);
  }
  else
  {
    if (lookLeft || lookRight)
    {
      if (lookLeft && !lookRight)
      {
        camera.TurnLeft();
      }
      else if (lookRight && !lookLeft)
      {
        camera.TurnRight();
      }
    }
    if (lookUp || lookDown)
    {
      if (lookUp && !lookDown)
      {
        camera.TurnUp();
      }
      else if (lookDown && !lookUp)
      {
        camera.TurnDown();
      }
    }
  }

  glm::vec3 pos = camera.GetPosition();
  WrapCoordinates(pos);
  camera.SetPosition(pos);

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
  static float despawnTimer = 0.f;
  despawnTimer += TimerState.GetDeltaTime();
  if (despawnTimer > 1.f)
  {
    VulkanInterface::WaitUntilAllCommandsSubmittedToQueueAreFinished(graphicsQueue); // Ouch
    chunkManager->despawnChunks(camera.GetPosition());
    despawnTimer = 0.f;
  }
  auto chunkList = chunkManager->getChunkSpawnList(camera.GetPosition());
  for (auto & chunk : chunkList)
  {    
    if (chunk.second == ChunkManager::ChunkStatus::NotLoadedCached)
    {
      if (logging)
      {
        tp registered = hr_clock::now();
        computeTaskflow->emplace([=, &logFile = logFile]() {
          logEntryData data;
          data.registered = registered;         

          loadFromChunkCache(chunk.first, data);

          data.end = hr_clock::now();
          insertEntry(logFile, data);
        });
      }
      else
      {
        computeTaskflow->emplace([=]() {
          loadFromChunkCache(chunk.first);
        });
      }
    }
    else if (chunk.second == ChunkManager::ChunkStatus::NotLoadedNotCached)
    { 
      if (logging)
      {
        tp registered = hr_clock::now();
        computeTaskflow->emplace([=, &logFile = logFile]() {
          logEntryData data;
          data.registered = registered;

          generateChunk(chunk.first, data);

          data.end = hr_clock::now();
          insertEntry(logFile, data);
        });
      }
      else
      {
        computeTaskflow->emplace([=]() {
          generateChunk(chunk.first);
        });
      }
    }
  }
  computeTaskflow->dispatch();
}

void ComputeApp::getChunkRenderList()
{
  constexpr float screenDepth = static_cast<float>(TechnicalChunkDim * chunkViewDistance)*1.25f;
  camera.GetViewMatrix(view);
  proj = glm::perspective(glm::radians(90.f), static_cast<float>(swapchain.size.width) / static_cast<float>(swapchain.size.height), 0.1f, screenDepth);
  proj[1][1] *= -1; // Correct projection for vulkan

  frustum.Construct(screenDepth, proj, view);

  chunkRenderList.clear();
  chunkRenderList.reserve(256); // Revise size when frustum culling implemented
  int i = 0;
  registryMutex.lock();

  // Sort chunks by world position so if we truncate the renderlist we preserve the closest chunks
  registry->sort<WorldPosition>([&](auto const & lhs, auto const & rhs) {
    return sqrdToroidalDistance(camera.GetPosition(), lhs.pos) < sqrdToroidalDistance(camera.GetPosition(), rhs.pos);
  });

  registry->view<WorldPosition, VolumeData, ModelData, AABB>().each(
    [=, &i=i, &registry=registry, &chunkRenderList=chunkRenderList, &camera=camera](const uint32_t entity, auto&&...)
    {
      auto[pos, modelData] = registry->get<WorldPosition, ModelData>(entity);

      // Check if we need to shift chunk position by world dimension
      glm::vec3 chunkPos = pos.pos;
      CorrectChunkPosition(camera.GetPosition(), chunkPos);

      if (modelData.indexCount > 0 && chunkIsWithinFrustum(entity) && chunkRenderList.size() < 256)
      {
        chunkRenderList.push_back(entity);

        // Calculate model matrix for chunk
        VmaAllocationInfo modelInfo;
        vmaGetAllocationInfo(allocator, modelAllocs[nextFrameIndex], &modelInfo);
        char * chunkDataPtr = static_cast<char*>(modelInfo.pMappedData);


        glm::mat4 model = glm::translate(glm::mat4(1.f), chunkPos);

        PerChunkData data = {
          model
        };
        memcpy(&chunkDataPtr[i*dynamicAlignment], &data, sizeof(PerChunkData));
        i++; // Move to next aligned position
      }      
    }
  );
  registryMutex.unlock();

  vmaFlushAllocation(allocator, modelAllocs[nextFrameIndex], 0, VK_WHOLE_SIZE);

  VulkanInterface::BufferDescriptorInfo viewProjDescriptorUpdate = {
    descriptorSets[nextFrameIndex],             // VkDescriptorSet                      TargetDescriptorSet
    0,                                          // uint32_t                             TargetDescriptorBinding
    0,                                          // uint32_t                             TargetArrayElement
    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,          // VkDescriptorType                     TargetDescriptorType
    {                                           // std::vector<VkDescriptorBufferInfo>  BufferInfos
      {
        viewprojUBuffers[nextFrameIndex],       // VkBuffer                             buffer
        0,                                      // VkDeviceSize                         offset
        VK_WHOLE_SIZE                           // VkDeviceSize                         range
      }
    }
  };
  VulkanInterface::BufferDescriptorInfo modelDescriptorUpdate = {
      descriptorSets[nextFrameIndex],             // VkDescriptorSet                      TargetDescriptorSet
      1,                                          // uint32_t                             TargetDescriptorBinding
      0,                                          // uint32_t                             TargetArrayElement
      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,  // VkDescriptorType                     TargetDescriptorType
      {                                           // std::vector<VkDescriptorBufferInfo>  BufferInfos
        {
          modelUBuffers[nextFrameIndex],          // VkBuffer                             buffer
          0,                                      // VkDeviceSize                         offset
          VK_WHOLE_SIZE                           // VkDeviceSize                         range
        }
      }
  };
  VulkanInterface::BufferDescriptorInfo lightDescriptorUpdate = {
      descriptorSets[nextFrameIndex],             // VkDescriptorSet                      TargetDescriptorSet
      2,                                          // uint32_t                             TargetDescriptorBinding
      0,                                          // uint32_t                             TargetArrayElement
      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,          // VkDescriptorType                     TargetDescriptorType
      {                                           // std::vector<VkDescriptorBufferInfo>  BufferInfos
        {
          lightUBuffers[nextFrameIndex],          // VkBuffer                             buffer
          0,                                      // VkDeviceSize                         offset
          VK_WHOLE_SIZE                           // VkDeviceSize                         range
        }
      }
  };

  VulkanInterface::WaitForFences(*vulkanDevice, { *frameResources[nextFrameIndex].drawingFinishedFence }, false, std::numeric_limits<uint64_t>::max());
  VulkanInterface::UpdateDescriptorSets(*vulkanDevice, {}, { viewProjDescriptorUpdate, modelDescriptorUpdate, lightDescriptorUpdate }, {}, {});
}

bool ComputeApp::drawChunks()
{
  auto framePrep = [&](VkCommandBuffer commandBuffer, uint32_t imageIndex, VkFramebuffer framebuffer)
  {
    if (chunkRenderList.size() > 0)
    {
      if (!VulkanInterface::BeginCommandBufferRecordingOp(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr))
      {
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
        , { {0.5f, 0.5f, 0.5f, 1.f}, {1.f, 0.f } } // Clear Color, one for our draw area, one for our depth stencil
        , VK_SUBPASS_CONTENTS_INLINE
      );


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
      vmaGetAllocationInfo(allocator, viewprojAllocs[imageIndex], &viewprojInfo);
      VmaAllocationInfo lightInfo;
      vmaGetAllocationInfo(allocator, lightAllocs[imageIndex], &lightInfo);

      void * viewprojPtr;

      ViewProj viewProj = {
        view,
        proj
      };

      vmaMapMemory(allocator, viewprojAllocs[imageIndex], &viewprojPtr);      
      memcpy(viewprojPtr, &viewProj, sizeof(ViewProj));
      vmaUnmapMemory(allocator, viewprojAllocs[imageIndex]);
      vmaFlushAllocation(allocator, viewprojAllocs[imageIndex], 0, VK_WHOLE_SIZE);

      for (int i = 0; i < chunkRenderList.size(); i++)
      {        
        registryMutex.lock();
        ModelData modelData = registry->get<ModelData>(chunkRenderList[i]);           

        uint32_t dynamicOffset = i * static_cast<uint32_t>(dynamicAlignment);
        VulkanInterface::BindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineLayout, 0, { descriptorSets[imageIndex] }, { dynamicOffset });

        VulkanInterface::BindVertexBuffers(commandBuffer, 0, { {modelData.vertexBuffer, 0} });
        VulkanInterface::BindIndexBuffer(commandBuffer, modelData.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        VulkanInterface::DrawIndexedGeometry(commandBuffer, modelData.indexCount, 1, 0, 0, 0);
        registryMutex.unlock();
      }

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
        return false;
      }

    }

    return true;
  };


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

    // Technically only required once with static data, but if the lighting data were dynamic this makes sense
    if (!VulkanInterface::UseStagingBufferToUpdateBufferWithDeviceLocalMemoryBound(
       *vulkanDevice
      , allocator
      , sizeof(LightData)
      , &lightData
      , lightUBuffers[nextFrameIndex]
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

    static uint32_t frameIndex = 0;
    FrameResources & currentFrame = frameResources[frameIndex];

    if (!VulkanInterface::WaitForFences(*vulkanDevice, { *currentFrame.drawingFinishedFence }, false, std::numeric_limits<uint64_t>::max()))
    {
      return false;
    }

    if (!VulkanInterface::ResetFences(*vulkanDevice, { *currentFrame.drawingFinishedFence }))
    {
      return false;
    }

    if (!VulkanInterface::PrepareSingleFrameOfAnimation(*vulkanDevice
      , graphicsQueue
      , presentQueue
      , *swapchain.handle
      , {}
      , *currentFrame.imageAcquiredSemaphore
      , *currentFrame.readyToPresentSemaphore
      , *currentFrame.drawingFinishedFence
      , framePrep
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
  else
  {
    //mutex->unlock();
    return true;
  }
}

bool ComputeApp::chunkIsWithinFrustum(uint32_t const entity)
{
  auto[pos, aabb] = registry->get<WorldPosition, AABB>(entity);
  glm::vec3 chunkPos = pos.pos;
  CorrectChunkPosition(camera.GetPosition(), chunkPos);
  return frustum.CheckCube(chunkPos, 32) || frustum.CheckCube(chunkPos, 16); // Oversized aabb for frustum check
}

void ComputeApp::loadFromChunkCache(EntityHandle handle)
{
  registryMutex.lock();
  glm::vec3 pos;
  if (registry->valid(handle)) // Verify handle is still valid
  {
    pos = registry->get<WorldPosition>(handle).pos;
  }
  else
  {
    registryMutex.unlock();
    return;
  }
  registryMutex.unlock();
  ChunkCacheData data;
  if (chunkManager->getChunkVolumeDataFromCache(chunkManager->chunkKey(pos), data)) // Retrieve data from cache
  {
    registryMutex.lock();
    auto & volume = registry->get<VolumeData>(handle);
    volume.volume = data;
    registryMutex.unlock();

    surfaceExtractor->extractSurface(handle, registry.get(), &registryMutex, nextFrameIndex);

    registryMutex.lock();
    auto & model = registry->get<ModelData>(handle);
    syncout() << handle << " generated, " << model.indexCount / 3 << " triangles\n";
    registryMutex.unlock();
  }
  else // Chunk has fallen out of the cache
  {
    generateChunk(handle);
  }
}

void ComputeApp::generateChunk(EntityHandle handle)
{
  if (!ready) return; // Catch if we're about to shutdown
  if (!registry->valid(handle)) return; // Chunk has been unloaded
  else
  {
    registry->get<VolumeData>(handle).generating = true; // Mark volume as generating to stop it being unloaded during generation
  }

  //std::cout << handle << std::endl;
  auto pos = registry->get<WorldPosition>(handle);
  ChunkCacheData data = terrainGen->getChunkVolume(pos.pos);
  registryMutex.lock();
  {
    auto & volume = registry->get<VolumeData>(handle);
    volume.volume = data;
  }
  registryMutex.unlock();

  surfaceExtractor->extractSurface(handle, registry.get(), &registryMutex, nextFrameIndex);

  registryMutex.lock();
  {
    auto[model, volume] = registry->get<ModelData, VolumeData>(handle);
    volume.generating = false;
    syncout() << handle << " generated, " << model.indexCount / 3 << " triangles\n";
  }
  registryMutex.unlock();
}

void ComputeApp::loadFromChunkCache(EntityHandle handle, logEntryData & logData)
{
  registryMutex.lock();
  glm::vec3 pos;
  if (registry->valid(handle)) // Verify handle is still valid
  {
    pos = registry->get<WorldPosition>(handle).pos;
    logData.key = chunkManager->chunkKey(pos);
  }
  else
  {
    registryMutex.unlock();
    return;
  }
  registryMutex.unlock();
  ChunkCacheData data;
  if (chunkManager->getChunkVolumeDataFromCache(chunkManager->chunkKey(pos), data)) // Retrieve data from cache
  {
    registryMutex.lock();
    auto & volume = registry->get<VolumeData>(handle);
    volume.volume = data;
    registryMutex.unlock();

    surfaceExtractor->extractSurface(handle, registry.get(), &registryMutex, nextFrameIndex);

    registryMutex.lock();
    auto & model = registry->get<ModelData>(handle);
    syncout() << handle << " generated, " << model.indexCount / 3 << " triangles\n";
    registryMutex.unlock();

    logData.loadedFromCache = true;
  }
  else // Chunk has fallen out of the cache
  {
    generateChunk(handle, logData);
  }
}

void ComputeApp::generateChunk(EntityHandle handle, logEntryData & logData)
{ 
  if (!ready) return; // Catch if we're about to shutdown
  if (!registry->valid(handle)) return; // Chunk has been unloaded
  else
  {
    registry->get<VolumeData>(handle).generating = true; // Mark volume as generating to stop it being unloaded during generation
  }

  logData.start = hr_clock::now();
  logData.loadedFromCache = false;

  //std::cout << handle << std::endl;
  auto pos = registry->get<WorldPosition>(handle);
  logData.key = chunkManager->chunkKey(pos.pos);
  ChunkCacheData data = terrainGen->getChunkVolume(pos.pos, logData);
  registryMutex.lock();
  {
    auto & volume = registry->get<VolumeData>(handle);
    volume.volume = data;
  }
  registryMutex.unlock();

  logData.surfaceStart = hr_clock::now();
  surfaceExtractor->extractSurface(handle, registry.get(), &registryMutex, nextFrameIndex);
  logData.surfaceEnd = hr_clock::now();

  registryMutex.lock();
  {
    auto[model, volume] = registry->get<ModelData, VolumeData>(handle);
    volume.generating = false;
    syncout() << handle << " generated, " << model.indexCount / 3 << " triangles\n";
  }
  registryMutex.unlock();
}

void ComputeApp::Shutdown()
{
  if (ready)
  {
    computeTaskflow->wait_for_all();
    VulkanInterface::WaitForAllSubmittedCommandsToBeFinished(*vulkanDevice);

    // We can shutdown some systems in parallel since they don't depend on each other
    auto[vulkan, vma, chnkMngr, gpipe] = systemTaskflow->emplace(
      [&]() { cleanupVulkan(); },
      [&]() { shutdownVulkanMemoryAllocator(); },
      [&]() { shutdownChunkManager(); },
      [&]() { shutdownGraphicsPipeline(); }
    );

    tf::Task saveLogFile;
    if (logging)
    {
      systemTaskflow->emplace(
        [=, &logFile=logFile, &computeTaskflow=computeTaskflow]() {
          computeTaskflow->wait_for_all();
          logFile.close();
        }
      );
    }

    // Task dependencies
    vma.precede(vulkan);
    chnkMngr.precede(vulkan);
    chnkMngr.precede(vma);
    gpipe.precede(vulkan);
    gpipe.precede(vma);

    systemTaskflow->dispatch().get();
  }
}
