#pragma once
#include "VulkanInterface.hpp"

class PipelineBase
{
public:
  PipelineBase() {}
  virtual ~PipelineBase() = 0;

  VkPipeline handle;
protected:
  VkPipelineLayout pipelineLayout;
};

inline PipelineBase::~PipelineBase() {}
