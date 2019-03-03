#pragma once
#include "cpp-cache\fifo-cache.h"
#include "ReservedMap.hpp"

using KeyType = uint64_t;
using EntityHandle = uint32_t;
using ChunkCacheData = std::array<Voxel, ChunkSize>;

class ChunkCache
{
public:
  bool has(KeyType const key)
  {
    return cache.has(key);
  }

  void add(KeyType const key, ChunkCacheData & data)
  {
    cache.insert(key, std::move(data));
  }

  bool retrieve(KeyType const key, ChunkCacheData & data)
  {
    if (cache.try_get(key, data))
    {
      cache.erase(key);
      return true;
    }
    else
    {
      return false;
    }
    cache.erase(key);
  }

  void clear()
  {
    cache.clear();
  }

protected:
  // Chunks might get unloaded but we might need it again if the player backtracks,
  // So we use a fifo cache to store the last few unloaded chunks volume data for quick reloading
  cpp_cache::fifo_cache< KeyType
                       , ChunkCacheData
                       , ChunkMapCacheSize
                       , ReservedMap<KeyType, ChunkCacheData, ChunkMapCacheSize> > cache; 
};