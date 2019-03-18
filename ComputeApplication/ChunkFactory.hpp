#pragma once
#include <entt\entity\registry.hpp>
#include "components.hpp"

class ChunkFactory
{
public:
  ChunkFactory(entt::DefaultRegistry * const registry, std::mutex * const registryMutex, VmaAllocator * const allocator)
    : registry(registry)
    , registryMutex(registryMutex)
    , allocator(allocator)
  {
  }

  ~ChunkFactory()
  {
    assert(registry->size() == 0);
  }

  uint32_t CreateChunkEntity(glm::vec3 pos, float dimX, float dimY, float dimZ);
  void DestroyChunk(uint32_t entityHandle);
  void DestroyAllChunks();

protected:
  entt::DefaultRegistry * const registry;
  std::mutex * const registryMutex;
  VmaAllocator * const allocator;
};
