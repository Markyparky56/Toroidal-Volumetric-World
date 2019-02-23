#pragma once
#include <entt\entity\registry.hpp>
#include "components.hpp"

class ChunkFactory
{
  ChunkFactory(entt::registry<> * const registry, VmaAllocator * const allocator)
    : registry(registry)
    , allocator(allocator)
  {
  }

  ~ChunkFactory()
  {
    DestroyAllChunks();
  }

  uint32_t CreateChunk(glm::vec3 pos, float dimX, float dimY, float dimZ)
  {
    auto entity = registry->create();
    registry->assign<WorldPosition>(entity, pos);
    registry->assign<VolumeData>(entity, nullptr, nullptr, allocator);
    registry->assign<ModelData>(entity, nullptr, nullptr, nullptr, allocator);
    registry->assign<AABB>(entity, dimX, dimY, dimZ);

    return entity;
  }

  void DestroyChunk(uint32_t entityHandle)
  {
    registry->destroy(entityHandle);
  }

  void DestroyAllChunks()
  {
    registry->view<WorldPosition, VolumeData, ModelData, AABB>().each(
      [&](const auto entity, auto&&...)
      {
        registry->destroy(entity); 
      }
    );
  }

protected:
  entt::registry<> * const registry;
  VmaAllocator * const allocator;
};
