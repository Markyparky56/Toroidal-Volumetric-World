#pragma once
#include "DualMC.hpp"
#include "meshoptimizer.h"
#include "genNormals.hpp"
#include "common.hpp"
#include "components.hpp"
#include <stack>
#include <mutex>

class SurfaceExtractor
{
public:
  SurfaceExtractor(VkDevice * const logicalDevice, VkQueue * const transferQueue, std::stack<VkCommandBuffer*> * const transferCommandBuffersStack, std::mutex * const stackMutex)
    : logicalDevice(logicalDevice)
    , transferQueue(transferQueue)
    , transferCommandBuffersStack(transferCommandBuffersStack)
    , stackMutex(stackMutex)
  {}
  ~SurfaceExtractor() {}

  bool extractSurface(VolumeData const & volume, ModelData & modelData);

private:
  VkDevice * const logicalDevice;
  VkQueue * const transferQueue;
  std::stack<VkCommandBuffer*> * const transferCommandBuffersStack;
  std::mutex * const stackMutex;
};
