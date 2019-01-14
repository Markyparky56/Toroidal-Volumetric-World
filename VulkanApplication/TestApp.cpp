#include "TestApp.hpp"

bool TestApp::Initialise(VulkanInterface::WindowParameters windowParameters)
{
  if (!initVulkan(windowParameters
    , VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
    , true
    , VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT))
  {
    return false;
  }

  if (!VulkanInterface::CreateFramebuffersForFrameResources(*vulkanDevice, *renderPass, swapchain, frameResources))
  {
    return false;
  }

  // Drawing Synchronisation
  VulkanInterface::InitVulkanHandle(vulkanDevice, drawingFence);
  if (!VulkanInterface::CreateFence(*vulkanDevice, true, *drawingFence))
  {
    return false;
  }

  VulkanInterface::InitVulkanHandle(vulkanDevice, imageAcquiredSemaphore);
  if (!VulkanInterface::CreateSemaphore(*vulkanDevice, *imageAcquiredSemaphore))
  {
    return false;
  }

  VulkanInterface::InitVulkanHandle(vulkanDevice, readyToPresentSemaphore);
  if (!VulkanInterface::CreateSemaphore(*vulkanDevice, *readyToPresentSemaphore))
  {
    return false;
  }

  if (!SetupGraphicsPipeline())
  {
    return false;
  }
  if (!SetupGraphicsBuffers())
  {
    return false;
  }

  //SetupComputePipeline();

  return true;
}

bool TestApp::Update()
{
  //if (!VulkanInterface::WaitForFences(*vulkanDevice, { *drawingFence }, false, 5000000000))
  //{
  //  return false;
  //}

  //if (!VulkanInterface::ResetFences(*vulkanDevice, { *drawingFence }))
  //{
  //  return false;
  //}

  //uint32_t imageIndex;
  //if (!VulkanInterface::AcquireSwapchainImage(*vulkanDevice, *swapchain.handle, *imageAcquiredSemaphore, VK_NULL_HANDLE, imageIndex))
  //{
  //  return false;
  //}

  auto framePrep = [&](VkCommandBuffer, uint32_t imageIndex, VkFramebuffer framebuffer)
  {
    if (!VulkanInterface::BeginCommandBufferRecordingOp(graphicsCommandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr))
    {
      return false;
    }

    if (presentQueue.familyIndex != graphicsQueue.familyIndex)
    {
      VulkanInterface::ImageTransition imageTransitionBeforeDrawing = {
        swapchain.images[imageIndex],
        VK_ACCESS_MEMORY_READ_BIT,
        VK_ACCESS_MEMORY_READ_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        presentQueue.familyIndex,
        graphicsQueue.familyIndex,
        VK_IMAGE_ASPECT_COLOR_BIT
      };

      VulkanInterface::SetImageMemoryBarrier(graphicsCommandBuffer
        , VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        , VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        , { imageTransitionBeforeDrawing }
      );
    }

    // Draw
    VulkanInterface::BeginRenderPass(graphicsCommandBuffer, *renderPass, framebuffer
      , { {0,0}, swapchain.size } // Render Area (full frame size)
      , { {0.1f, 0.2f, 0.3f, 1.f} } // Clear Color
      , VK_SUBPASS_CONTENTS_INLINE
    );

    VulkanInterface::BindPipelineObject(graphicsCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *graphicsPipeline);

    VkViewport viewport = {
      0.f,
      0.f,
      static_cast<float>(swapchain.size.width),
      static_cast<float>(swapchain.size.height),
      0.f,
      1.f
    };
    VulkanInterface::SetViewportStateDynamically(graphicsCommandBuffer, 0, { viewport });

    VkRect2D scissor = {
      {
        0, 0
      },
      {
        swapchain.size.width,
        swapchain.size.height
      }
    };
    VulkanInterface::SetScissorStateDynamically(graphicsCommandBuffer, 0, { scissor });

    VulkanInterface::BindVertexBuffers(graphicsCommandBuffer, 0, { {*vertexBuffer, 0} });

    VulkanInterface::DrawGeometry(graphicsCommandBuffer, 3, 1, 0, 0);

    VulkanInterface::EndRenderPass(graphicsCommandBuffer);

    if (presentQueue.familyIndex != graphicsQueue.familyIndex)
    {
      VulkanInterface::ImageTransition imageTransitionBeforePresent = {
        swapchain.images[imageIndex],
        VK_ACCESS_MEMORY_READ_BIT,
        VK_ACCESS_MEMORY_READ_BIT,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        graphicsQueue.familyIndex,
        presentQueue.familyIndex,
        VK_IMAGE_ASPECT_COLOR_BIT
      };

      VulkanInterface::SetImageMemoryBarrier(graphicsCommandBuffer
        , VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        , VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
        , { imageTransitionBeforePresent });
    }

    if (!VulkanInterface::EndCommandBufferRecordingOp(graphicsCommandBuffer))
    {
      return false;
    }
    return true;
  };  

  return VulkanInterface::RenderWithFrameResources( *vulkanDevice
                                                  , graphicsQueue.handle
                                                  , presentQueue.handle
                                                  , *swapchain.handle
                                                  , swapchain.size
                                                  , swapchain.imageViewsRaw
                                                  , *renderPass
                                                  , {}
                                                  , framePrep
                                                  , frameResources);

  //VulkanInterface::WaitSemaphoreInfo waitSemaphoreInfo = {
  //    *imageAcquiredSemaphore
  //  , VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
  //};

  //if (!VulkanInterface::SubmitCommandBuffersToQueue(graphicsQueue.handle
  //  , { waitSemaphoreInfo }
  //  , { graphicsCommandBuffer }
  //  , { *readyToPresentSemaphore }
  //  , *drawingFence))
  //{
  //  return false;
  //}

  //VulkanInterface::PresentInfo presentInfo = {
  //   *swapchain.handle
  //  , imageIndex
  //};

  //if (!VulkanInterface::PresentImage(presentQueue.handle, { *readyToPresentSemaphore }, { presentInfo }))
  //{
  //  return false;
  //}

  //return true;
}

bool TestApp::Resize()
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
    , *renderPass
    , swapchain
    , frameResources)
  )
  {
    return false;
  }

  return true;
}

void TestApp::cleanupVulkan()
{
  // Although we have VulkanHandle which will call destroying functions which is useful for objects with 
  // short lifetimes, i.e. buffers and the like which get created at the device level then fall out of 
  // scope. However for cleaning up the whole application we need to to make sure they get destroyed 
  // in the right order.

  // We need to work backwards, destroying device-level objects before instance-level objects
  if (bufferMemory) VulkanInterface::vkFreeMemory(*vulkanDevice, *bufferMemory, nullptr);
  if (vertexBuffer) VulkanInterface::vkDestroyBuffer(*vulkanDevice, *vertexBuffer, nullptr);
  if (graphicsPipeline) VulkanInterface::vkDestroyPipeline(*vulkanDevice, *graphicsPipeline, nullptr);
  if (graphicsPipelineLayout) VulkanInterface::vkDestroyPipelineLayout(*vulkanDevice, *graphicsPipelineLayout, nullptr);
  if (renderPass) VulkanInterface::vkDestroyRenderPass(*vulkanDevice, *renderPass, nullptr);
  if (graphicsCommandPool) VulkanInterface::vkDestroyCommandPool(*vulkanDevice, *graphicsCommandPool, nullptr);

  if (drawingFence) VulkanInterface::vkDestroyFence(*vulkanDevice, *drawingFence, nullptr);
  if (imageAcquiredSemaphore) VulkanInterface::vkDestroySemaphore(*vulkanDevice, *imageAcquiredSemaphore, nullptr);
  if (readyToPresentSemaphore) VulkanInterface::vkDestroySemaphore(*vulkanDevice, *readyToPresentSemaphore, nullptr);
  
  for (auto & frameRes : frameResources)
  {
    VulkanInterface::vkDestroySemaphore(*vulkanDevice, *frameRes.imageAcquiredSemaphore, nullptr);
    VulkanInterface::vkDestroySemaphore(*vulkanDevice, *frameRes.readyToPresentSemaphore, nullptr);
    VulkanInterface::vkDestroyFence(*vulkanDevice, *frameRes.drawingFinishedFence, nullptr);
    VulkanInterface::vkDestroyImageView(*vulkanDevice, *frameRes.depthAttachment, nullptr);
    VulkanInterface::vkDestroyFramebuffer(*vulkanDevice, *frameRes.framebuffer, nullptr);
  }
  if (swapchain.handle) VulkanInterface::vkDestroySwapchainKHR(*vulkanDevice, *swapchain.handle, nullptr);
  for (auto & imageView : swapchain.imageViews)
  {
    VulkanInterface::vkDestroyImageView(*vulkanDevice, *imageView, nullptr);
  }

  for (auto & image : depthImages)
  {
    VulkanInterface::vkDestroyImage(*vulkanDevice, *image, nullptr);
  }
  for (auto & imgMem : depthImagesMemory)
  {
    VulkanInterface::vkFreeMemory(*vulkanDevice, *imgMem, nullptr);
  }

  // Cleanup device
  if (vulkanDevice) VulkanInterface::vkDestroyDevice(*vulkanDevice, nullptr);

  // Cleanup Debug Messenger if in debug
#if defined(_DEBUG)
  if (callback) VulkanInterface::vkDestroyDebugUtilsMessengerEXT(*vulkanInstance, callback, nullptr);
#endif

  if (presentationSurface) VulkanInterface::vkDestroySurfaceKHR(*vulkanInstance, *presentationSurface, nullptr);
  if (vulkanInstance) VulkanInterface::vkDestroyInstance(*vulkanInstance, nullptr);
  if (vulkanLibrary) VulkanInterface::ReleaseVulkanLoaderLibrary(vulkanLibrary);
}

bool TestApp::SetupGraphicsPipeline()
{
  // Command Buffer Creation, Graphics
  VulkanInterface::InitVulkanHandle(vulkanDevice, graphicsCommandPool);
  if (!VulkanInterface::CreateCommandPool( *vulkanDevice
                                          , VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
                                          , graphicsQueue.familyIndex
                                          , *graphicsCommandPool))
  {
    return false;
  }

  std::vector<VkCommandBuffer> graphicsCommandBuffers;
  if (!VulkanInterface::AllocateCommandBuffers( *vulkanDevice
                                              , *graphicsCommandPool
                                              , VK_COMMAND_BUFFER_LEVEL_PRIMARY
                                              , 1
                                              , graphicsCommandBuffers))
  {
    return false;
  }

  graphicsCommandBuffer = graphicsCommandBuffers[0];

  // Render Pass
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
    }
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
      nullptr,
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

  VulkanInterface::InitVulkanHandle(vulkanDevice, renderPass);
  if (!VulkanInterface::CreateRenderPass(*vulkanDevice, attachmentDescriptions, subpassParameters, subpassDependecies, *renderPass))
  {
    return false;
  }

  // Compile Vertex Shader
  std::vector<unsigned char> vertexShaderSpirv;
  if (!VulkanInterface::GetBinaryFileContents("Data/vert.spv", vertexShaderSpirv))
  {
    return false;
  }

  VulkanHandle(VkShaderModule) vertexShaderModule;
  VulkanInterface::InitVulkanHandle(vulkanDevice, vertexShaderModule);
  if (!VulkanInterface::CreateShaderModule(*vulkanDevice, vertexShaderSpirv, *vertexShaderModule))
  {
    return false;
  }

  // Compile Fragment Shader
  std::vector<unsigned char> fragmentShaderSpirv;
  if (!VulkanInterface::GetBinaryFileContents("Data/frag.spv", fragmentShaderSpirv))
  {
    return false;
  }

  VulkanHandle(VkShaderModule) fragmentShaderModule;
  VulkanInterface::InitVulkanHandle(vulkanDevice, fragmentShaderModule);
  if (!VulkanInterface::CreateShaderModule(*vulkanDevice, fragmentShaderSpirv, *fragmentShaderModule))
  {
    return false;
  }

  std::vector<VulkanInterface::ShaderStageParameters> shaderStageParams = {
    {
      VK_SHADER_STAGE_VERTEX_BIT,
      *vertexShaderModule,
      "main",
      nullptr
    },
    {
      VK_SHADER_STAGE_FRAGMENT_BIT,
      *fragmentShaderModule,
      "main",
      nullptr
    }
  };

  std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos;
  VulkanInterface::SpecifyPipelineShaderStages(shaderStageParams, shaderStageCreateInfos);

  std::vector<VkVertexInputBindingDescription> vertexInputBindingDescriptions = {
    {
      0,
      3 * sizeof(float),
      VK_VERTEX_INPUT_RATE_VERTEX
    }
  };

  std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions = {
    {
      0,
      0,
      VK_FORMAT_R32G32B32_SFLOAT,
      0
    }
  };

  VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo;
  VulkanInterface::SpecifyPipelineVertexInputState(vertexInputBindingDescriptions, vertexAttributeDescriptions, vertexInputStateCreateInfo);

  VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo;
  VulkanInterface::SpecifyPipelineInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, false, inputAssemblyStateCreateInfo);

  VulkanInterface::ViewportInfo viewportInfos = {
    { // Viewports
      {
        0.f,
        0.f,
        500.f,
        500.f,
        0.f,
        1.f
      }
    },
    { // Scissors
      {
        {
          0,
          0
        },
        {
          500,
          500
        }
      }
    }
  };

  VkPipelineViewportStateCreateInfo viewportStateCreateInfo;
  VulkanInterface::SpecifyPipelineViewportAndScissorTestState(viewportInfos, viewportStateCreateInfo);

  VkPipelineRasterizationStateCreateInfo rasterisationStateCreateInfo;
  VulkanInterface::SpecifyPipelineRasterisationState(false, false, VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, false, 0.f, 0.f, 0.f, 1.f, rasterisationStateCreateInfo);

  VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo;
  VulkanInterface::SpecifyPipelineMultisampleState(VK_SAMPLE_COUNT_1_BIT, false, 0.f, nullptr, false, false, multisampleStateCreateInfo);

  std::vector<VkPipelineColorBlendAttachmentState> attachmentBlendStates = {
    {
      false,
      VK_BLEND_FACTOR_ONE,
      VK_BLEND_FACTOR_ONE,
      VK_BLEND_OP_ADD,
      VK_BLEND_FACTOR_ONE,
      VK_BLEND_FACTOR_ONE,
      VK_BLEND_OP_ADD,
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    }
  };

  VkPipelineColorBlendStateCreateInfo blendStateCreateInfo;
  VulkanInterface::SpecifyPipelineBlendState(false, VK_LOGIC_OP_COPY, attachmentBlendStates, { 1.f, 1.f, 1.f, 1.f }, blendStateCreateInfo);

  std::vector<VkDynamicState> dynamicStates = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
  };

  VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo;
  VulkanInterface::SpecifyPipelineDynamicStates(dynamicStates, dynamicStateCreateInfo);

  VulkanInterface::InitVulkanHandle(vulkanDevice, graphicsPipelineLayout);
  if (!VulkanInterface::CreatePipelineLayout(*vulkanDevice, {}, {}, *graphicsPipelineLayout))
  {
    return false;
  }

  VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo;
  VulkanInterface::SpecifyGraphicsPipelineCreationParameters(
      0
    , shaderStageCreateInfos
    , vertexInputStateCreateInfo
    , inputAssemblyStateCreateInfo
    , nullptr
    , &viewportStateCreateInfo
    , rasterisationStateCreateInfo
    , &multisampleStateCreateInfo
    , nullptr
    , &blendStateCreateInfo
    , &dynamicStateCreateInfo
    , *graphicsPipelineLayout
    , *renderPass
    , 0
    , VK_NULL_HANDLE
    , -1
    , graphicsPipelineCreateInfo
  );

  std::vector<VkPipeline> pipeline;
  if (!VulkanInterface::CreateGraphicsPipelines(
      *vulkanDevice
    , { graphicsPipelineCreateInfo }
    , VK_NULL_HANDLE
    , pipeline
    )
  )
  {
    return false;
  }

  VulkanInterface::InitVulkanHandle(vulkanDevice, graphicsPipeline);
  *graphicsPipeline = pipeline[0];

  return true;
}

bool TestApp::SetupComputePipeline()
{
  // Command Buffer Creation, Compute
  //VulkanInterface::InitVulkanHandle(vulkanDevice, computeCommandPool);
  //if (!VulkanInterface::CreateCommandPool(*vulkanDevice
  //  , VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
  //  , graphicsQueue.familyIndex
  //  , *computeCommandPool))
  //{
  //  return false;
  //}

  //std::vector<VkCommandBuffer> computeCommandBuffers;
  //if (!VulkanInterface::AllocateCommandBuffers(*vulkanDevice
  //  , *computeCommandPool
  //  , VK_COMMAND_BUFFER_LEVEL_PRIMARY
  //  , 1
  //  , computeCommandBuffers))
  //{
  //  return false;
  //}

  //computeCommandBuffer = computeCommandBuffers[0];

  return true;
}

bool TestApp::SetupGraphicsBuffers()
{
  std::vector<float> vertices = {
     0.f  , -0.75f, 0.f,
    -0.75f,  0.75f, 0.f,
     0.75f,  0.75f, 0.f
  };

  // Vertex Buffer
  VulkanInterface::InitVulkanHandle(vulkanDevice, vertexBuffer);
  if (!VulkanInterface::CreateBuffer(*vulkanDevice
    , sizeof(vertices[0]) * vertices.size()
    , VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
    , *vertexBuffer))
  {
    return false;
  }

  // BufferMemory
  VulkanInterface::InitVulkanHandle(vulkanDevice, bufferMemory);
  if (!VulkanInterface::AllocateAndBindMemoryObjectToBuffer(vulkanPhysicalDevice, *vulkanDevice, *vertexBuffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, *bufferMemory))
  {
    return false;
  }

  if (!VulkanInterface::UseStagingBufferToUpdateBufferWithDeviceLocalMemoryBound(
    vulkanPhysicalDevice
    , *vulkanDevice
    , sizeof(vertices[0]) * vertices.size()
    , &vertices[0]
    , *vertexBuffer
    , 0, 0,
    VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT
    , VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
    , VK_PIPELINE_STAGE_VERTEX_INPUT_BIT
    , graphicsQueue.handle
    , graphicsCommandBuffer
    , {}
    )
  )
  {
    return false;
  }

  return true;
}
