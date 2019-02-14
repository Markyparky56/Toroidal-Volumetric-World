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

  if (!SetupGraphicsPipeline())
  {
    return false;
  }
  if (!SetupGraphicsBuffers())
  {
    return false;
  }
  if (!SetupFrameResources())
  {
    return false;
  }   

  if (!SetupComputePipeline())
  {
    return false;
  }

  return true;
}

bool TestApp::Update()
{
  auto framePrep = [&](VkCommandBuffer, uint32_t imageIndex, VkFramebuffer framebuffer)
  {
    auto commandBuffer = graphicsCommandBuffers[imageIndex];

    if (!VulkanInterface::BeginCommandBufferRecordingOp(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr))
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

      VulkanInterface::SetImageMemoryBarrier(commandBuffer
        , VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        , VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        , { imageTransitionBeforeDrawing }
      );
    }

    // Draw
    VulkanInterface::BeginRenderPass(commandBuffer, *renderPass, framebuffer
      , { {0,0}, swapchain.size } // Render Area (full frame size)
      , { {0.1f, 0.2f, 0.3f, 1.f}, {1.f, 0.f } } // Clear Color, one for our draw area, one for our depth stencil
      , VK_SUBPASS_CONTENTS_INLINE
    );

    VulkanInterface::BindPipelineObject(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *graphicsPipeline);

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

    VulkanInterface::BindVertexBuffers(commandBuffer, 0, { {*vertexBuffer, 0} });

    VulkanInterface::DrawGeometry(commandBuffer, 3, 1, 0, 0);

    VulkanInterface::EndRenderPass(commandBuffer);

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

      VulkanInterface::SetImageMemoryBarrier(commandBuffer
        , VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        , VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
        , { imageTransitionBeforePresent });
    }

    if (!VulkanInterface::EndCommandBufferRecordingOp(commandBuffer))
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
  VulkanInterface::WaitForAllSubmittedCommandsToBeFinished(*vulkanDevice);
  // Although we have VulkanHandle which will call destroying functions which is useful for objects with 
  // short lifetimes, i.e. buffers and the like which get created at the device level then fall out of 
  // scope. However for cleaning up the whole application we need to to make sure they get destroyed 
  // in the right order.

  // We need to work backwards, destroying device-level objects before instance-level objects
  VulkanInterface::FreeMemoryObject(*vulkanDevice, *bufferMemory);
  VulkanInterface::DestroyBuffer(*vulkanDevice, *vertexBuffer);
  VulkanInterface::DestroyPipeline(*vulkanDevice, *graphicsPipeline);
  VulkanInterface::DestroyPipelineLayout(*vulkanDevice, *graphicsPipelineLayout);
  VulkanInterface::DestroyRenderPass(*vulkanDevice, *renderPass);
  VulkanInterface::DestroyCommandPool(*vulkanDevice, *graphicsCommandPool);

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
  for (auto & image : depthImages)
  {
    VulkanInterface::DestroyImage(*vulkanDevice, *image);
  }
  for (auto & imgMem : depthImagesMemory)
  {
    VulkanInterface::FreeMemoryObject(*vulkanDevice, *imgMem);
  }

  VulkanInterface::DestroyImageView(*vulkanDevice, *imageView);
  VulkanInterface::FreeMemoryObject(*vulkanDevice, *imageMemory);
  VulkanInterface::DestroyImage(*vulkanDevice, *image);

  VulkanInterface::DestroyCommandPool(*vulkanDevice, *computeCommandPool);
  VulkanInterface::DestroyDescriptorPool(*vulkanDevice, *descriptorPool);
  VulkanInterface::DestroyDescriptorSetLayout(*vulkanDevice, *descriptorSetLayout);
  VulkanInterface::DestroyPipeline(*vulkanDevice, *computePipeline);
  VulkanInterface::DestroyPipelineLayout(*vulkanDevice, *computePipelineLayout);

  // Cleanup device
  if (vulkanDevice) VulkanInterface::DestroyLogicalDevice(*vulkanDevice);

  // Cleanup Debug Messenger if in debug
#if defined(_DEBUG)
  if (callback) VulkanInterface::vkDestroyDebugUtilsMessengerEXT(*vulkanInstance, callback, nullptr);
#endif

  if (presentationSurface) VulkanInterface::DestroyPresentationSurface(*vulkanInstance, *presentationSurface);
  if (vulkanInstance) VulkanInterface::DestroyVulkanInstance(*vulkanInstance);
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

  if (!VulkanInterface::AllocateCommandBuffers(*vulkanDevice
    , *graphicsCommandPool
    , VK_COMMAND_BUFFER_LEVEL_PRIMARY
    , numFrames+1 // +1 for memory ops
    , graphicsCommandBuffers))
  {
    return false;
  }

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

  VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo;
  VulkanInterface::SpecifyPipelineDepthAndStencilState(true, true, VK_COMPARE_OP_LESS_OR_EQUAL, false, 0.f, 1.f, false, {}, {}, depthStencilStateCreateInfo);

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
    , &depthStencilStateCreateInfo
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
  VulkanInterface::InitVulkanHandle(vulkanDevice, computeCommandPool);
  if (!VulkanInterface::CreateCommandPool(*vulkanDevice
    , VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
    , computeQueue.familyIndex
    , *computeCommandPool))
  {
    return false;
  }

  if (!VulkanInterface::AllocateCommandBuffers(*vulkanDevice
    , *computeCommandPool
    , VK_COMMAND_BUFFER_LEVEL_PRIMARY
    , 1
    , computeCommandBuffers))
  {
    return false;
  }

  // Storage Image
  VulkanInterface::InitVulkanHandle(vulkanDevice, image);
  VulkanInterface::InitVulkanHandle(vulkanDevice, imageMemory);
  VulkanInterface::InitVulkanHandle(vulkanDevice, imageView);
  if (!VulkanInterface::Create2DImageAndView(vulkanPhysicalDevice
    , *vulkanDevice
    , swapchain.format
    , swapchain.size
    , 1, 1, VK_SAMPLE_COUNT_1_BIT
    , VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
    , VK_IMAGE_ASPECT_COLOR_BIT
    , *image, *imageMemory, *imageView)
    )
  {
    return false;
  }

  VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {
    0,
    VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
    1,
    VK_SHADER_STAGE_COMPUTE_BIT,
    nullptr
  };
  VulkanInterface::InitVulkanHandle(vulkanDevice, descriptorSetLayout);
  if (!VulkanInterface::CreateDescriptorSetLayout(*vulkanDevice, { descriptorSetLayoutBinding }, *descriptorSetLayout))
  {
    return false;
  }

  VkDescriptorPoolSize descriptorPoolSize = {
    VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
    1
  };

  VulkanInterface::InitVulkanHandle(vulkanDevice, descriptorPool);
  if (!VulkanInterface::CreateDescriptorPool(*vulkanDevice, false, 1, { descriptorPoolSize }, *descriptorPool))
  {
    return false;
  }

  if (!VulkanInterface::AllocateDescriptorSets(*vulkanDevice, *descriptorPool, { *descriptorSetLayout }, descriptorSets))
  {
    return false;
  }

  VulkanInterface::ImageDescriptorInfo imageDescriptorUpdate = {
    descriptorSets[0],
    0,
    0,
    VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
    {
      {
        VK_NULL_HANDLE,
        *imageView,
        VK_IMAGE_LAYOUT_GENERAL
      }
    }
  };

  VulkanInterface::UpdateDescriptorSets(*vulkanDevice, { imageDescriptorUpdate }, {}, {}, {});

  std::vector<unsigned char> computeShaderSpirv;
  if (!VulkanInterface::GetBinaryFileContents("Data/compute.spv", computeShaderSpirv))
  {
    return false;
  }

  VulkanHandle(VkShaderModule) computeShaderModule;
  VulkanInterface::InitVulkanHandle(vulkanDevice, computeShaderModule);
  if (!VulkanInterface::CreateShaderModule(*vulkanDevice, computeShaderSpirv, *computeShaderModule))
  {
    return false;
  }

  std::vector<VulkanInterface::ShaderStageParameters> shaderStageParams = {
    {
      VK_SHADER_STAGE_COMPUTE_BIT,
      *computeShaderModule,
      "main",
      nullptr
    }
  };

  std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos;
  VulkanInterface::SpecifyPipelineShaderStages(shaderStageParams, shaderStageCreateInfos);

  VulkanInterface::InitVulkanHandle(vulkanDevice, computePipelineLayout);
  if (!VulkanInterface::CreatePipelineLayout(*vulkanDevice, { *descriptorSetLayout }, {}, *computePipelineLayout))
  {
    return false;
  }

  VulkanInterface::InitVulkanHandle(vulkanDevice, computePipeline);
  if (!VulkanInterface::CreateComputePipeline(*vulkanDevice
    , 0
    , shaderStageCreateInfos[0]
    , *computePipelineLayout
    , VK_NULL_HANDLE
    , VK_NULL_HANDLE
    , *computePipeline))
  {
    return false;
  }

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
    , 0
    , 0
    , VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT
    , VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
    , VK_PIPELINE_STAGE_VERTEX_INPUT_BIT
    , graphicsQueue.handle
    , graphicsCommandBuffers[numFrames]
    , {}
    )
  )
  {
    return false;
  }

  return true;
}

bool TestApp::SetupFrameResources()
{
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

    depthImages.emplace_back(VulkanHandle(VkImage)());
    VulkanInterface::InitVulkanHandle(vulkanDevice, depthImages.back());
    depthImagesMemory.emplace_back(VulkanHandle(VkDeviceMemory)());
    VulkanInterface::InitVulkanHandle(vulkanDevice, depthImagesMemory.back());

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
        graphicsCommandBuffers[i],
        std::move(imageAcquiredSemaphore),
        std::move(readyToPresentSemaphore),
        std::move(drawingFinishedFence),
        std::move(depthAttachment),
        VulkanHandle(VkFramebuffer)()
      }
    );    

    if (!VulkanInterface::Create2DImageAndView(vulkanPhysicalDevice
      , *vulkanDevice
      , VK_FORMAT_D16_UNORM
      , swapchain.size
      , 1, 1
      , VK_SAMPLE_COUNT_1_BIT
      , VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
      , VK_IMAGE_ASPECT_DEPTH_BIT
      , *depthImages.back()
      , *depthImagesMemory.back()
      , *frameResources[i].depthAttachment))
    {
      return false;
    }

  }

  if (!VulkanInterface::CreateFramebuffersForFrameResources(*vulkanDevice, *renderPass, swapchain, frameResources))
  {
    return false;
  }

  return true;
}
