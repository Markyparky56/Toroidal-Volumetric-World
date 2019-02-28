#include "ComputePipeline.hpp"

ComputePipeline::ComputePipeline(VkDevice * const logicalDevice)
  : PipelineBase(logicalDevice)
{
}

ComputePipeline::~ComputePipeline()
{
}

void ComputePipeline::cleanup()
{
}
