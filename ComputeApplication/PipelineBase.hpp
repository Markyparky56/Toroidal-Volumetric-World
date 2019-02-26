#pragma once
#include "VulkanInterface.hpp"

class PipelineBase
{
public:
  PipelineBase() {}
  virtual ~PipelineBase() = 0;
  virtual void cleanup() = 0;

  VkPipeline handle;
protected:
  VkPipelineLayout pipelineLayout;
};

inline PipelineBase::~PipelineBase() {}
