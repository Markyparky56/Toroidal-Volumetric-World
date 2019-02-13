#pragma once
#include "PipelineBase.hpp"
#include <vector>

class GraphicsPipeline : public PipelineBase
{
public:
  GraphicsPipeline();
  ~GraphicsPipeline();

  std::vector<VkDescriptorSetLayout> layouts;

  VkRenderPass renderPass;
 std::vector<VulkanInterface::FrameResources> frameResources;
protected:

};