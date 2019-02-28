#pragma once
#include "common.hpp"

#include "ChunkFactory.hpp"
#include "ChunkCache.hpp"
#include "ChunkMap.hpp"

class ChunkManager
{
public:
  ChunkManager(entt::registry<> * const registry, VmaAllocator * const allocator);
  ~ChunkManager();

private:
  ChunkFactory factory;
  ChunkCache cache;
  ChunkMap map;

  enum class ChunkStatus
  {
    NotLoadedNotCached,
    NotLoadedCached,
    Loaded
  };

  KeyType chunkKey(glm::vec3 pos)
  {
    KeyType x = static_cast<KeyType>(pos.x)
      , y = static_cast<KeyType>(pos.y)
      , z = static_cast<KeyType>(pos.z);

    KeyType key = ((x & 0x1FFFFF) << 43) | ((y & 0x1FFFFF) << 22) | (z & 0x3FFFFF);

    return key;
  }

  ChunkStatus chunkStatus(uint64_t const key)
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
};
