#pragma once
#include "VulkanInterface.hpp"

class PipelineBase
{
public:
  PipelineBase(VkDevice * const logicalDevice) 
    : logicalDevice(logicalDevice)
  {}
  virtual ~PipelineBase() = 0;
  virtual void cleanup() = 0;

  VkPipeline handle;
protected:
  VkPipelineLayout pipelineLayout;
  VkDevice * const logicalDevice;
};

inline PipelineBase::~PipelineBase() {}
