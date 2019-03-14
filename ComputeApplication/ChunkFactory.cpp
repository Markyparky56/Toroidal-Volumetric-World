#include "ChunkFactory.hpp"

uint32_t ChunkFactory::CreateChunkEntity(glm::vec3 pos, float dimX, float dimY, float dimZ)
{
  auto entity = registry->create();
  registry->assign<WorldPosition>(entity, pos);
  registry->assign<VolumeData>(entity, VkBuffer(), VkBufferView(), VmaAllocation(), allocator);
  registry->assign<ModelData>(entity, VkBuffer(), VkBuffer(), VmaAllocation(), VmaAllocation(), allocator, 0);
  registry->assign<AABB>(entity, dimX, dimY, dimZ);

  return entity;
}

void ChunkFactory::DestroyChunk(uint32_t entityHandle)
{
  registry->destroy(entityHandle);
}

void ChunkFactory::DestroyAllChunks()
{
  registry->view<WorldPosition, VolumeData, ModelData, AABB>().each(
    [&](const uint32_t entity, auto&&...)
    {
      registry->destroy(entity);
    }
  );
}
