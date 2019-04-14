#include "ChunkFactory.hpp"
#include "syncout.hpp"
uint32_t ChunkFactory::CreateChunkEntity(glm::vec3 pos, float dimX, float dimY, float dimZ)
{
  assert(allocator != nullptr);

  registryMutex->lock();
  auto entity = registry->create();
  registry->assign<WorldPosition>(entity, pos);
  registry->assign<VolumeData>(entity, std::array<Voxel, ChunkSize>({ 0 }), false);
  registry->assign<ModelData>(entity, VkBuffer(), VkBuffer(), VmaAllocation(), VmaAllocation(), allocator, 0ui32);
  registry->assign<AABB>(entity, dimX, dimY, dimZ);
  registry->assign<Flags>(entity, false, 0ui32);
  registryMutex->unlock();

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
  registryMutex->lock();
  syncout() << "Destroy All Chunks!" << std::endl;
  registry->view<WorldPosition, VolumeData, ModelData, AABB>().each(
    [&](const uint32_t entity, auto&&...)
    {
      auto[volume, model] = registry->get<VolumeData, ModelData>(entity);
      //volume.destroy();
      model.destroy();
      registry->destroy(entity);
    }
  );
  registryMutex->unlock();
}
