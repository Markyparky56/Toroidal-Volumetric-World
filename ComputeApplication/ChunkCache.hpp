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

  void add(KeyType const key, ChunkCacheData && data)
  {
    cache.insert(key, std::move(data));
  }

  ChunkCacheData retrieve(KeyType const key)
  {
    ChunkCacheData data = cache.get(key);
    //cache.try_get(key, data);
    cache.erase(key);
    return data;
  }

protected:
  cpp_cache::fifo_cache<KeyType, ChunkCacheData, ChunkMapCacheSize, ReservedMap<KeyType, ChunkCacheData, ChunkMapCacheSize>> cache; // Chunks might get unloaded but we might need it again if the player backtracks
};