#pragma once
#include "PipelineBase.hpp"
#include <vector>

class ComputePipeline : public PipelineBase
{
public:
  ComputePipeline();
  ~ComputePipeline();

  std::vector<VkDescriptorSetLayout> layouts;

protected:
};