#pragma once
#include "common.hpp"

#include "ChunkFactory.hpp"
#include "ChunkCache.hpp"
#include "ChunkMap.hpp"

class ChunkManager
{
public:
  ChunkManager(entt::registry<> * const registry, VmaAllocator * const allocator, VkDevice * const logicalDevice);
  ~ChunkManager();

  enum class ChunkStatus
  {
    NotLoadedNotCached,
    NotLoadedCached,
    Loaded
  };

  // Returns a list of <EntityHandle, ChunkStatus> pairs of chunks not yet loaded into the ChunkMap
  // These chunks may be cached, if so their volume data can be retrieved via getChunkVolumeDataFromCache
  std::vector<std::pair<EntityHandle, ChunkManager::ChunkStatus>> getChunkSpawnList(glm::vec3 const playerPos);
  bool getChunkVolumeDataFromCache(KeyType const key, ChunkCacheData & data);

  void despawnChunks(glm::vec3 const playerPos);

  // Insert a chunks handle into the chunk map
  void loadChunk(KeyType const key, EntityHandle const handle);
  
  // Remove a chunk from the chunk map, caching its volume data and destroying its entity in the registry
  void unloadChunk(KeyType const key);

private:
  entt::registry<> * const registry;
  VkDevice * const logicalDevice;
  VmaAllocator * const allocator;
  ChunkFactory factory;
  ChunkCache cache;
  ChunkMap map;  

  KeyType chunkKey(glm::vec3 const pos);
  ChunkStatus chunkStatus(uint64_t const key);
  bool pointInSpawnRange(glm::vec3 const playerPos, glm::vec3 const point);
  bool pointInDespawnRange(glm::vec3 const playerPos, glm::vec3 const point);
};
