#pragma once
#include <unordered_map>
#include "glm/glm.hpp"
#include <array>
#include "common.hpp"
#include "voxel.hpp"
#include "cpp-cache/fifo-cache.h"
#include "ChunkFactory.hpp"
#include "ChunkCache.hpp"

class ChunkMap
{
public:
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

  ChunkStatus chunkStatus(uint64_t const key, ChunkCache & cache)
  {
    if (map.count(key))
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

  ChunkStatus chunkStatus(glm::vec3 const pos, ChunkCache & cache)
  {
    return chunkStatus(chunkKey(pos), cache);
  }

  void loadChunk(glm::vec3 pos, uint32_t entity)
  {
    KeyType key = chunkKey(pos);
    map[key] = entity;
  }

protected:
  std::unordered_map<KeyType, EntityHandle> map; // Map tracks loaded chunks  
};
