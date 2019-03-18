#pragma once
#include "DualMC.hpp"
#include "meshoptimizer.h"
#include "genNormals.hpp"
#include "common.hpp"
#include "components.hpp"
#include "TaskflowCommandPools.hpp"
#include <stack>
#include <mutex>
#include "entt\entity\registry.hpp"

class SurfaceExtractor
{
public:
  SurfaceExtractor(VkDevice * const logicalDevice, VkQueue * const transferQueue, std::mutex * const transferQMutex, TaskflowCommandPools * const commandPools)
    : logicalDevice(logicalDevice)
    , transferQueue(transferQueue)
    , transferQMutex(transferQMutex)
    , commandPools(commandPools)
  {}
  ~SurfaceExtractor() {}

  // TODO: consider whether frame is required, compute should be frame independent
  bool extractSurface(uint32_t entity, entt::DefaultRegistry * registry, std::mutex * const registryMutex, uint32_t frame);

private:
  VkDevice * const logicalDevice;
  VkQueue * const transferQueue;
  std::mutex * const transferQMutex;
  TaskflowCommandPools * const commandPools;
};
