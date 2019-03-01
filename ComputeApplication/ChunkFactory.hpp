#pragma once
#include <entt\entity\registry.hpp>
#include "components.hpp"

class ChunkFactory
{
public:
  ChunkFactory(entt::registry<> * const registry, VmaAllocator * const allocator)
    : registry(registry)
    , allocator(allocator)
  {
  }

  ~ChunkFactory()
  {
    DestroyAllChunks();
  }

  uint32_t CreateChunkEntity(glm::vec3 pos, float dimX, float dimY, float dimZ);
  void DestroyChunk(uint32_t entityHandle);
  void DestroyAllChunks();

protected:
  entt::registry<> * const registry;
  VmaAllocator * const allocator;
};
