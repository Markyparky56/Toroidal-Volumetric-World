#include "GraphicsPipeline.hpp"

GraphicsPipeline::GraphicsPipeline( VkDevice * const logicalDevice
                                  , VkRenderPass * const renderPass
                                  , std::string vertexShaderLoc
                                  , std::string fragmentShaderLoc)
  : PipelineBase(logicalDevice)
  , renderPass(renderPass)
  , vertexShaderLoc(vertexShaderLoc)
  , fragmentShaderLoc(fragmentShaderLoc)
{
}

GraphicsPipeline::~GraphicsPipeline()
{
  if (*logicalDevice) cleanup(); 
}

void GraphicsPipeline::cleanup()
{
  if (descriptorPool) VulkanInterface::DestroyDescriptorPool(*logicalDevice, descriptorPool);
}

bool GraphicsPipeline::init()
{
  VkShaderModule vertShader, fragShader;
  VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo;
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo;
  VkPipelineViewportStateCreateInfo viewportStateCreateInfo;
  VkPipelineRasterizationStateCreateInfo rasterisationStateCreateInfo;
  VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo;
  VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo;
  VkPipelineColorBlendStateCreateInfo blendStateCreateInfo;
  VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo;
  VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo;

  if (!loadVertexShader(vertShader))
  {
    return false;
  }

  if (!loadFragmentShader(fragShader))
  {
    return false;
  }

  std::vector<VulkanInterface::ShaderStageParameters> shaderStageParams = {
    {
      VK_SHADER_STAGE_VERTEX_BIT,
      vertShader,
      "main",
      nullptr
    },
    {
      VK_SHADER_STAGE_FRAGMENT_BIT,
      fragShader,
      "main",
      nullptr
    }
  };

  std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos;
  VulkanInterface::SpecifyPipelineShaderStages(shaderStageParams, shaderStageCreateInfos);

  std::vector<VkVertexInputBindingDescription> vertexInputBindingDescriptions = {
    {
      0,
      6 * sizeof(float), // vec3 pos, vec3 normal
      VK_VERTEX_INPUT_RATE_VERTEX
    }
  };

  std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions = {
    {
      0,
      0,
      VK_FORMAT_R32G32B32_SFLOAT,
      0
    },
    {
      1,
      0,
      VK_FORMAT_R32G32B32_SFLOAT,
      3 * sizeof(float)
    }
  };

  VulkanInterface::SpecifyPipelineVertexInputState(vertexInputBindingDescriptions, vertexAttributeDescriptions, vertexInputStateCreateInfo);
  VulkanInterface::SpecifyPipelineInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, false, inputAssemblyStateCreateInfo);

  VulkanInterface::ViewportInfo viewportInfos = {
   { // Viewports
     {
       0.f,
       0.f,
       1920.f, // Dummy values, these will be set dynamically during runtime
       1080.f,
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
         1920,
         1080
       }
     }
   }
  };

  VulkanInterface::SpecifyPipelineViewportAndScissorTestState(viewportInfos, viewportStateCreateInfo);
  VulkanInterface::SpecifyPipelineRasterisationState(false, false, VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, false, 0.f, 0.f, 0.f, 1.f, rasterisationStateCreateInfo);
  VulkanInterface::SpecifyPipelineMultisampleState(VK_SAMPLE_COUNT_1_BIT, false, 0.f, nullptr, false, false, multisampleStateCreateInfo);
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

  VulkanInterface::SpecifyPipelineBlendState(false, VK_LOGIC_OP_COPY, attachmentBlendStates, { 1.f, 1.f, 1.f, 1.f }, blendStateCreateInfo);

  std::vector<VkDynamicState> dynamicStates = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
  };

  VulkanInterface::SpecifyPipelineDynamicStates(dynamicStates, dynamicStateCreateInfo);

  if (!VulkanInterface::CreatePipelineLayout(*logicalDevice, layouts, pushConstantRanges, pipelineLayout))
  {
    return false;
  }

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
    , pipelineLayout
    , *renderPass
    , 0
    , VK_NULL_HANDLE
    , -1
    , graphicsPipelineCreateInfo
  );

  std::vector<VkPipeline> pipeline;
  if (!VulkanInterface::CreateGraphicsPipelines(
      *logicalDevice
    , { graphicsPipelineCreateInfo }
    , VK_NULL_HANDLE
    , pipeline
    )
  )
  {
    return false;
  }

  handle = pipeline[0];

  return true;
}

bool GraphicsPipeline::loadVertexShader(VkShaderModule & vertexShader)
{
  std::vector<unsigned char> vertexShaderSpirv;
  if (!VulkanInterface::GetBinaryFileContents(vertexShaderLoc, vertexShaderSpirv))
  {
    return false;
  }

  if (vertexShader) VulkanInterface::DestroyShaderModule(*logicalDevice, vertexShader);
  if (!VulkanInterface::CreateShaderModule(*logicalDevice, vertexShaderSpirv, vertexShader))
  {
    return false;
  }
  return true;
}

bool GraphicsPipeline::loadFragmentShader(VkShaderModule & fragmentShader)
{
    std::vector<unsigned char> fragmentShaderSpirv;
    if (!VulkanInterface::GetBinaryFileContents(vertexShaderLoc, fragmentShaderSpirv))
    {
      return false;
    }

    if (fragmentShader) VulkanInterface::DestroyShaderModule(*logicalDevice, fragmentShader);
    if (!VulkanInterface::CreateShaderModule(*logicalDevice, fragmentShaderSpirv, fragmentShader))
    {
      return false;
    }
    return true;
}

bool GraphicsPipeline::setupDescriptorPool(std::vector<VkDescriptorPoolSize>& poolSizes)
{
  if (descriptorPool) VulkanInterface::DestroyDescriptorPool(*logicalDevice, descriptorPool);

  if (poolSizes.size() > 0)
  {
    uint32_t maxSetsCount = 0;
    for (auto & set : poolSizes)
    {
      maxSetsCount += set.descriptorCount;
    }

    if (!VulkanInterface::CreateDescriptorPool(*logicalDevice, true, maxSetsCount, poolSizes, descriptorPool))
    {
      return false;
    }
  }

  return true;
}
