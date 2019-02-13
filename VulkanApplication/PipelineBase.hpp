#pragma once
#include "VulkanInterface.hpp"

class PipelineBase
{
public:
  PipelineBase() {}
  virtual ~PipelineBase() = 0;

  VkPipeline handle;
  uint32_t familyIndex;
protected:
  VkPipelineLayout pipelineLayout;
};

inline PipelineBase::~PipelineBase() {}
