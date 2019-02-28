#pragma once
#include "PipelineBase.hpp"
#include <vector>

class ComputePipeline : public PipelineBase
{
public:
  ComputePipeline(VkDevice * const logicalDevice);
  ~ComputePipeline();
  void cleanup() override;

  VkDescriptorPool descriptorPool;
  std::vector<VkDescriptorSetLayout> layouts;

protected:
};