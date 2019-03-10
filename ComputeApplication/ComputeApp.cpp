#include "ComputeApp.hpp"

//#include "chunkDirectionalLight.vert.spv.h"
//#include "chunkDirectionalLight.frag.spv.h"

bool ComputeApp::Initialise(VulkanInterface::WindowParameters windowParameters)
{
  // Setup some basic data
  gameTime = 0.0;
  settingsLastChangeTimes = { 0.f };
  lockMouse = true;
  ShowCursor(!lockMouse);
  camera.SetPosition({ 0.f, 0.f, 0.f });
  camera.SetUp({ 0.f, 1.f, 0.f });
  camRot = { 0.f, 0.f, 0.f };

  hWnd = windowParameters.HWnd;

  if (!setupVulkanAndCreateSwapchain(windowParameters))
  {
    return false;
  }

  screenCentre = { static_cast<float>(swapchain.size.width), static_cast<float>(swapchain.size.height) };

  if (!setupTaskflow())
  {
    return false;
  }

  // Prepare setup tasks
  bool resVMA, resImGui, resCmdBufs, resRenderpass, resGpipe, resChnkMngr, resTerGen, resECS;
  auto[vma, /*imgui,*/ commandBuffers, renderpass, gpipeline, chunkmanager, terraingen, ecs] = systemTaskflow->emplace(
    [&]() { resVMA = initialiseVulkanMemoryAllocator(); },
    //[&]() { resImGui = initImGui(windowParameters.HWnd); },
    [&]() { resCmdBufs = setupCommandPoolAndBuffers(); },
    [&]() { resRenderpass = setupRenderPass(); },
    [&]() { 
      if (!resRenderpass) { resGpipe = false; return; }
      resGpipe = setupGraphicsPipeline(); },
    [&]() { 
      if (!resECS && !resVMA) { resChnkMngr = false; return; }
      resChnkMngr = setupChunkManager(); },
    [&]() { resTerGen = setupTerrainGenerator(); },
    [&]() { resECS = setupECS(); }
  );

  // Task dependencies
  vma.precede(gpipeline);
  vma.precede(chunkmanager);
  renderpass.precede(gpipeline);
  //renderpass.precede(imgui);
  ecs.precede(chunkmanager);

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

  if (!resVMA /*|| !resImGui*/ || !resCmdBufs || !resRenderpass || !resGpipe || !resChnkMngr || !resTerGen || !resECS)
  {
    return false;
  }

  return true;
}

bool ComputeApp::Update()
{
  gameTime += TimerState.GetDeltaTime();

  auto[userUpdate, spawnChunks, renderList, recordDrawCalls, draw] = systemTaskflow->emplace(
    [&]() { updateUser(); },
    [&]() { checkForNewChunks(); },
    [&]() { getChunkRenderList(); },
    [&]() { recordChunkDrawCalls(); },
    [&]() { drawChunks(); }
  );

  userUpdate.precede(spawnChunks);
  userUpdate.precede(renderList);
  renderList.precede(recordDrawCalls);
  recordDrawCalls.precede(draw);

  return false;
}

bool ComputeApp::Resize()
{
  //ImGui_ImplVulkan_InvalidateDeviceObjects();
  //if (!createSwapchain(
  //  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
  //  , true
  //  , VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
  //  )
  //{
  //  return false;
  //}

  if (!VulkanInterface::CreateFramebuffersForFrameResources(
    *vulkanDevice
    , renderPass
    , swapchain
    , frameResources)
    )
  {
    return false;
  }
  //ImGui_ImplVulkan_CreateDeviceObjects();
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
        // Retrieve "present queue" handle
        vkGetDeviceQueue(*vulkanDevice, presentQueueParameters.familyIndex, 0, &presentQueue);
        // Retrieve compute queue handles
        computeQueues.resize(numComputeThreads, VK_NULL_HANDLE);
        for (uint32_t i = 1; i < numComputeThreads; i++)
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

  graphicsTaskflow = std::make_unique<tf::Taskflow>(tfExecutor);
  computeTaskflow = std::make_unique<tf::Taskflow>(tfExecutor);
  systemTaskflow = std::make_unique<tf::Taskflow>(tfExecutor);

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

//bool ComputeApp::initImGui(HWND hwnd)
//{
//  IMGUI_CHECKVERSION();
//  ImGui::CreateContext();
//
//  ImGui::StyleColorsDark();
//
//  if (!ImGui_ImplWin32_Init(hwnd))
//  {
//    return false;
//  }
//  
//  std::vector<VkDescriptorPoolSize> poolSizes = 
//  {
//    { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
//    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
//    { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
//    { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
//    { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
//    { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
//    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
//    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
//    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
//    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
//    { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
//  };
//  if (!VulkanInterface::CreateDescriptorPool(*vulkanDevice, true, 1000 * static_cast<uint32_t>(poolSizes.size()), poolSizes, imGuiDescriptorPool))
//  {
//    return false;
//  }  
//
//  ImGui_ImplVulkan_InitInfo initInfo = { 0 };
//  initInfo.Instance = *vulkanInstance;
//  initInfo.PhysicalDevice = vulkanPhysicalDevice;
//  initInfo.Device = *vulkanDevice;
//  initInfo.QueueFamily = graphicsQueueParameters.familyIndex;
//  initInfo.Queue = graphicsQueue;
//  initInfo.PipelineCache = VK_NULL_HANDLE;
//  initInfo.DescriptorPool = imGuiDescriptorPool;
//  initInfo.Allocator = VK_NULL_HANDLE;
//  initInfo.CheckVkResultFn = [](VkResult err) { 
//    if (err == VK_SUCCESS) return; 
//    else { 
//      std::cout << "ImGui Error (Non-success return value), Code: " << err << std::endl; 
//#if defined(_DEBUG)
//      if (err < 0) 
//        abort(); 
//#endif
//    }
//  };
//
//  if (!ImGui_ImplVulkan_Init(&initInfo, renderPass))
//  {
//    return false;
//  }
//
//  return true;
//}

bool ComputeApp::setupCommandPoolAndBuffers()
{
  // Create Graphics Command Pool
  if (!VulkanInterface::CreateCommandPool(*vulkanDevice
    , VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
    , graphicsQueueParameters.familyIndex
    , graphicsCommandPool))
  {
    return false;
  }

  // Allocate primary command buffers for frame resources
  if (!VulkanInterface::AllocateCommandBuffers(*vulkanDevice
    , graphicsCommandPool
    , VK_COMMAND_BUFFER_LEVEL_PRIMARY
    , numFrames
    , frameCommandBuffers))
  {
    return false;
  }

  // Allocate secondary command buffers for chunk models
  if (!VulkanInterface::AllocateCommandBuffers(*vulkanDevice
    , graphicsCommandPool
    , VK_COMMAND_BUFFER_LEVEL_SECONDARY
    , maxChunks*numFrames
    , chunkCommandBuffersVec))
  {
    return false;
  }

  // Setup command buffer stacks
  for (uint32_t i = 0; i < numFrames; i++)
  {
    for (uint32_t buffer = maxChunks*i; buffer < maxChunks; buffer++)
    {
      chunkCommandBufferStacks[i].push(&chunkCommandBuffersVec[buffer]);
    }
  }

  // TODO: Compute command pool and buffers

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
  // Uniform buffer
  if (!VulkanInterface::CreateUniformBuffer(vulkanPhysicalDevice
    , *vulkanDevice
    , 16 * sizeof(float)
    , VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
    , uniformBuffer
    , uniformBufferMemory)) 
  {
    return false;
  }
  //graphicsPipeline->uniformBuffer = &uniformBuffer;
  //graphicsPipeline->init();

  // Descriptor set with uniform buffer
  VkDescriptorSetLayoutBinding descriptor_set_layout_binding = {
    0,                                          // uint32_t             binding
    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,          // VkDescriptorType     descriptorType
    1,                                          // uint32_t             descriptorCount
    VK_SHADER_STAGE_VERTEX_BIT,                 // VkShaderStageFlags   stageFlags
    nullptr                                     // const VkSampler    * pImmutableSamplers
  };

  if (!VulkanInterface::CreateDescriptorSetLayout(*vulkanDevice, { descriptor_set_layout_binding }, descriptorSetLayout)) {
    return false;
  }

  VkDescriptorPoolSize descriptor_pool_size = {
      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,          // VkDescriptorType     type
      1                                           // uint32_t             descriptorCount
  };

  if (!VulkanInterface::CreateDescriptorPool(*vulkanDevice, false, 1, { descriptor_pool_size }, descriptorPool))
  {
    return false;
  }

  if (!VulkanInterface::AllocateDescriptorSets(*vulkanDevice, descriptorPool, { descriptorSetLayout }, descriptorSets))
  {
    return false;
  }

  //VulkanInterface::BufferDescriptorInfo buffer_descriptor_update = {
  //    descriptorSets[0],                          // VkDescriptorSet                      TargetDescriptorSet
  //    0,                                          // uint32_t                             TargetDescriptorBinding
  //    0,                                          // uint32_t                             TargetArrayElement
  //    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,          // VkDescriptorType                     TargetDescriptorType
  //    {                                           // std::vector<VkDescriptorBufferInfo>  BufferInfos
  //      {
  //        uniformBuffer,                            // VkBuffer                             buffer
  //        0,                                        // VkDeviceSize                         offset
  //        VK_WHOLE_SIZE                             // VkDeviceSize                         range
  //      }
  //    }
  //};

  //VulkanInterface::UpdateDescriptorSets(*vulkanDevice, {}, { buffer_descriptor_update }, {}, {});

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

  if (!VulkanInterface::CreatePipelineLayout(*vulkanDevice, { descriptorSetLayout }, {}, graphicsPipelineLayout)) {
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

bool ComputeApp::setupECS()
{
  registry = std::make_unique<entt::registry<>>();

  return true;
}

void ComputeApp::shutdownVulkanMemoryAllocator()
{
  if (allocator)
  {
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
  //graphicsPipeline->cleanup();
}

//void ComputeApp::shutdownImGui()
//{
//  ImGui_ImplVulkan_Shutdown();
//  ImGui_ImplWin32_Shutdown();
//  ImGui::DestroyContext();
//}

void ComputeApp::cleanupVulkan()
{
  if (vulkanDevice) VulkanInterface::WaitForAllSubmittedCommandsToBeFinished(*vulkanDevice);

  // VulkanHandle is useful for catching forgotten objects and ones with scoped/short-lifetimes, 
  // but for the final shutdown we need to be explicit
  VulkanInterface::DestroyDescriptorPool(*vulkanDevice, imGuiDescriptorPool);
  VulkanInterface::DestroyRenderPass(*vulkanDevice, renderPass);

  if (vulkanDevice) vkDestroyDevice(*vulkanDevice, nullptr);

#if defined(_DEBUG)
  if (callback) vkDestroyDebugUtilsMessengerEXT(*vulkanInstance, callback, nullptr);
#endif

  if (presentationSurface) vkDestroySurfaceKHR(*vulkanInstance, *presentationSurface, nullptr);
  if (vulkanInstance) vkDestroyInstance(*vulkanInstance, nullptr);
  if (vulkanLibrary) VulkanInterface::ReleaseVulkanLoaderLibrary(vulkanLibrary);
}

void ComputeApp::updateUser()
{
  bool moveLeft = (KeyboardState.Keys['A'].IsDown || KeyboardState.Keys[VK_LEFT].IsDown);
  bool moveRight = (KeyboardState.Keys['D'].IsDown || KeyboardState.Keys[VK_RIGHT].IsDown);
  bool moveForward = (KeyboardState.Keys['W'].IsDown || KeyboardState.Keys[VK_UP].IsDown);
  bool moveBackwards = (KeyboardState.Keys['S'].IsDown || KeyboardState.Keys[VK_DOWN].IsDown);
  bool moveUp = (KeyboardState.Keys[VK_SPACE].IsDown);
  bool moveDown = (KeyboardState.Keys[VK_CONTROL].IsDown);
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
  // Handle movement controls
  auto camPos = camera.GetPosition();
  if (moveLeft || moveRight)
  {
    if (moveLeft && !moveRight)
    {
      camPos -= (camera.GetRight() * dt * cameraSpeed);
    }
    else if (moveRight && !moveLeft)
    {
      camPos += (camera.GetRight() * dt * cameraSpeed);
    }
  }
  if (moveForward || moveBackwards)
  {
    if (moveForward && !moveBackwards)
    {
      camPos += (camera.GetForward() * dt * cameraSpeed);
    }
    else if (moveBackwards && !moveForward)
    {
      camPos -= (camera.GetForward() * dt * cameraSpeed);
    }
  }
  if (moveUp || moveDown)
  {
    if (moveUp && !moveDown)
    {
      camPos += (camera.GetUp() * dt * cameraSpeed);
    }
    else if (moveDown && !moveUp)
    {
      camPos -= (camera.GetUp() * dt * cameraSpeed);
    }
  }

  // Handle look controls
  glm::vec2 dir = screenCentre - glm::vec2(MouseState.Position.Delta.X, MouseState.Position.Delta.Y);
  dir = glm::normalize(dir);  
  camRot.pitch += dir.y * (90.f*dt);
  camRot.yaw += dir.x * (90.f*dt);

  camera.SetPosition(camPos);
  camera.SetPitch(camRot.pitch);
  camera.SetYaw(camRot.yaw);
  camera.SetRoll(camRot.roll);
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
    if (chunk.second == ChunkManager::ChunkStatus::NotLoadedCached)
    {

    }
  }
}

void ComputeApp::getChunkRenderList()
{
  chunkRenderList.reserve(343); // Revise size when frustum culling implemented
  registry->view<WorldPosition, VolumeData, ModelData, AABB>().each(
    [&](const uint32_t entity, auto&&...)
    {
      if (chunkIsWithinFrustum()) // dummy check
      {
        chunkRenderList.push_back(entity);
      }
    }
  );
}

void ComputeApp::recordChunkDrawCalls()
{
}

void ComputeApp::drawChunks()
{
}

bool ComputeApp::chunkIsWithinFrustum()
{
  return true;
}

void ComputeApp::Shutdown()
{
  if (ready)
  {
    // We can shutdown some systems in parallel since they don't depend on each other
    auto[vulkan, vma, /*imgui,*/ chnkMngr, gpipe] = systemTaskflow->emplace(
      [&]() { cleanupVulkan(); },
      [&]() { shutdownVulkanMemoryAllocator(); },
      //[&]() { shutdownImGui(); },
      [&]() { shutdownChunkManager(); },
      [&]() { shutdownGraphicsPipeline(); }
    );

    // Task dependencies
    vma.precede(vulkan);
    //imgui.precede(vulkan);
    chnkMngr.precede(vulkan);
    chnkMngr.precede(vma);
    gpipe.precede(vulkan);

    systemTaskflow->dispatch().get();
  }
}
