#include "ChunkFactory.hpp"

uint32_t ChunkFactory::CreateChunkEntity(glm::vec3 pos, float dimX, float dimY, float dimZ)
{
  assert(allocator != nullptr);
  auto entity = registry->create();
  auto wpos = registry->assign<WorldPosition>(entity, pos);
  //auto volume = registry->assign<VolumeData>(entity, VkBuffer(), VkBufferView(), VmaAllocation(), allocator);
  auto volume = registry->assign<VolumeData>(entity);
  auto model = registry->assign<ModelData>(entity, VkBuffer(), VkBuffer(), VmaAllocation(), VmaAllocation(), allocator, 0);
  auto aabb = registry->assign<AABB>(entity, dimX, dimY, dimZ);

  assert(model.allocator == allocator);

  return entity;
}

void ChunkFactory::DestroyChunk(uint32_t entityHandle)
{
  auto[volume, model] = registry->get<VolumeData, ModelData>(entityHandle);
  //volume.destroy();
  model.destroy();

  registry->destroy(entityHandle);
}

void ChunkFactory::DestroyAllChunks()
{
  std::cout << "Destroy All Chunks!" << std::endl;
  registry->view<WorldPosition, VolumeData, ModelData, AABB>().each(
    [&](const uint32_t entity, auto&&...)
    {
      auto[volume, model] = registry->get<VolumeData, ModelData>(entity);
      //volume.destroy();
      model.destroy();
      registry->destroy(entity);
    }
  );
}
