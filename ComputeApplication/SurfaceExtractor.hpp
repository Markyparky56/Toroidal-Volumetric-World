#pragma once
#include "DualMC.hpp"
#include "meshoptimizer.h"
#include "genNormals.hpp"
#include "common.hpp"
#include "components.hpp"

class SurfaceExtractor
{
public:
  SurfaceExtractor(VkDevice * const logicalDevice, VkQueue * const transferQueue, VkCommandBuffer * const transferCommandBuffer)
    : logicalDevice(logicalDevice)
    , transferQueue(transferQueue)
    , transferCommandBuffer(transferCommandBuffer)
  {}
  ~SurfaceExtractor();

  bool extractSurface(VolumeData const & volume, ModelData & modelData);

private:
  VkDevice * const logicalDevice;
  VkQueue * const transferQueue;
  VkCommandBuffer * const transferCommandBuffer;
  DualMCVoxel dmc;
};
