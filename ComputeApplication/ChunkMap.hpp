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
  bool isChunkLoaded(uint64_t const key)
  {
    return map.count(key) == 1 ? true : false;
  }

  void loadChunk(uint64_t const key, uint32_t const entity)
  {
    map[key] = entity;
  }

  //Returns entity handle so ChunkManager can cache the volume data
  EntityHandle unloadChunk(uint64_t const key)
  {
    EntityHandle handle = map.at(key);
    map.erase(key);
    return handle;
  }

  EntityHandle get(uint64_t const key)
  {
    return map[key];
  }

  void clear()
  {
    map.clear();
  }

protected:
  std::unordered_map<KeyType, EntityHandle> map; // Map tracks loaded chunks  
};
