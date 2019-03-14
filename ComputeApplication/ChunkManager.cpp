#include "ChunkManager.hpp"
#include "VulkanInterface.hpp"
#include "VulkanInterface.Functions.hpp"
#include "vk_mem_alloc.h"

ChunkManager::ChunkManager(entt::registry<> * const registry, VmaAllocator * const allocator, VkDevice * const logicalDevice)
  : factory(registry, allocator)
  , registry(registry)
  , logicalDevice(logicalDevice)
  , allocator(allocator)
{

}

ChunkManager::~ChunkManager()
{

}

std::vector<std::pair<EntityHandle, ChunkManager::ChunkStatus>> ChunkManager::getChunkSpawnList(glm::vec3 const playerPos)
{
  // Find the closest chunk from which to base our search off using double precision
  glm::vec3 offsetPlayerPos = {
        static_cast<float>(static_cast<double>(playerPos.x) - std::fmod(static_cast<double>(playerPos.x), static_cast<double>(TechnicalChunkDim))),
        static_cast<float>(static_cast<double>(playerPos.y) - std::fmod(static_cast<double>(playerPos.y), static_cast<double>(TechnicalChunkDim))),
        static_cast<float>(static_cast<double>(playerPos.z) - std::fmod(static_cast<double>(playerPos.z), static_cast<double>(TechnicalChunkDim)))
  };

  //uint32_t chunkRadius = TechnicalChunkDim * std::ceilf(chunkSpawnRadius * invTechnicalChunkDim);
  constexpr uint32_t chunkRadius = (TechnicalChunkDim * chunkSpawnDistance)/2;
  constexpr float chunkRadiusf = static_cast<float>(chunkRadius);
  std::vector<std::pair<EntityHandle, ChunkManager::ChunkStatus>> chunkList;

  for (float z = offsetPlayerPos.z - chunkRadiusf; z < offsetPlayerPos.z + chunkRadiusf; z += static_cast<float>(TechnicalChunkDim))
  {
    for (float y = offsetPlayerPos.y - chunkRadiusf; y < offsetPlayerPos.y + chunkRadiusf; y += static_cast<float>(TechnicalChunkDim))
    {
      for (float x = offsetPlayerPos.x - chunkRadiusf; x < offsetPlayerPos.x + chunkRadiusf; x += static_cast<float>(TechnicalChunkDim))
      {
        glm::vec3 chunkPos = { x,y,z };
        KeyType key = chunkKey(chunkPos);
        if (pointInSpawnRange(offsetPlayerPos, chunkPos))
        {
          ChunkStatus status = chunkStatus(key);
          if (status == ChunkStatus::NotLoadedNotCached || status == ChunkStatus::NotLoadedCached)
          {
            EntityHandle handle = factory.CreateChunkEntity(chunkPos, TechnicalChunkDim, TechnicalChunkDim, TechnicalChunkDim);
            map.loadChunk(key, handle);
            chunkList.push_back(std::make_pair(handle, status));
          }
          // else status == ChunkStatus::Loaded, requires no action
        }
      }
    }
  }

  return chunkList;
}

bool ChunkManager::getChunkVolumeDataFromCache(KeyType const key, ChunkCacheData & data)
{
  return cache.retrieve(key, data);
}

void ChunkManager::despawnChunks(glm::vec3 const playerPos)
{
  // Find the closest chunk from which to base our search off using double precision
  glm::vec3 offsetPlayerPos = {
        static_cast<float>(static_cast<double>(playerPos.x) - std::fmod(static_cast<double>(playerPos.x), static_cast<double>(TechnicalChunkDim))),
        static_cast<float>(static_cast<double>(playerPos.y) - std::fmod(static_cast<double>(playerPos.y), static_cast<double>(TechnicalChunkDim))),
        static_cast<float>(static_cast<double>(playerPos.z) - std::fmod(static_cast<double>(playerPos.z), static_cast<double>(TechnicalChunkDim)))
  };

  uint32_t chunkRadius = TechnicalChunkDim * std::ceilf(chunkDespawnRadius * invTechnicalChunkDim);
 // std::vector<std::pair<EntityHandle, ChunkManager::ChunkStatus>> chunkList;

  for (float z = offsetPlayerPos.z - chunkRadius; z < offsetPlayerPos.z + chunkRadius; z += TechnicalChunkDim)
  {
    for (float y = offsetPlayerPos.y - chunkRadius; y < offsetPlayerPos.y + chunkRadius; y += TechnicalChunkDim)
    {
      for (float x = offsetPlayerPos.x - chunkRadius; x < offsetPlayerPos.x + chunkRadius; x += TechnicalChunkDim)
      {
        glm::vec3 chunkPos = { x,y,z };
        KeyType key = chunkKey(chunkPos);
        if (pointInDespawnRange(offsetPlayerPos, chunkPos))
        {
          ChunkStatus status = chunkStatus(key);
          if (status == ChunkStatus::Loaded)
          {
            unloadChunk(key);
          }
        }
      }
    }
  }
}

void ChunkManager::loadChunk(KeyType const key, EntityHandle const handle)
{
  map.loadChunk(key, handle);
}

void ChunkManager::unloadChunk(KeyType const key)
{
  EntityHandle handle = map.unloadChunk(key);
  ChunkCacheData data;
  VolumeData & volume = registry->get<VolumeData>(handle);
  void * ptr;
  VkResult result = vmaMapMemory(*allocator, volume.volumeAllocation, &ptr);
  if (result == VK_SUCCESS)
  {
    VmaAllocationInfo info;
    vmaGetAllocationInfo(*volume.allocator, volume.volumeAllocation, &info);
    vmaInvalidateAllocation(*volume.allocator, volume.volumeAllocation, 0, info.size);
    memcpy(data.data(), ptr, sizeof(ChunkCacheData));
    vmaUnmapMemory(*allocator, volume.volumeAllocation);
    cache.add(key, data);
    factory.DestroyChunk(handle);
  }
  else
  {
    // PANIC
  }
}

void ChunkManager::shutdown()
{
  factory.DestroyAllChunks();
  cache.clear();
}

KeyType ChunkManager::chunkKey(glm::vec3 const pos)
{
  KeyType x = static_cast<KeyType>(pos.x)
        , y = static_cast<KeyType>(pos.y)
        , z = static_cast<KeyType>(pos.z);

  KeyType key = ((x & 0x1FFFFF) << 43) | ((y & 0x1FFFFF) << 22) | (z & 0x3FFFFF);

  return key;
}

ChunkManager::ChunkStatus ChunkManager::chunkStatus(uint64_t const key)
{
  if (map.isChunkLoaded(key))
  {
    return ChunkStatus::Loaded;
  }
  else if (cache.has(key))
  {
    return ChunkStatus::NotLoadedCached;
  }
  else
  {
    return ChunkStatus::NotLoadedNotCached;
  }
}

bool ChunkManager::pointInSpawnRange(glm::vec3 const playerPos, glm::vec3 const point)
{
  bool result = (point.x - playerPos.x)*(point.x - playerPos.x) + (point.y - playerPos.y)*(point.y - playerPos.y) + (point.z - playerPos.z)*(point.z - playerPos.z) <= chunkSpawnRadius * chunkSpawnRadius;
  return result;
}

bool ChunkManager::pointInDespawnRange(glm::vec3 const playerPos, glm::vec3 const point)
{
  bool result = (point.x - playerPos.x)*(point.x - playerPos.x) + (point.y - playerPos.y)*(point.y - playerPos.y) + (point.z - playerPos.z)*(point.z - playerPos.z) > chunkDespawnRadius * chunkDespawnRadius;
  return result;
}
