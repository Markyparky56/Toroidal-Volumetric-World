#include "ChunkManager.hpp"

ChunkManager::ChunkManager(entt::registry<> * const registry, VmaAllocator * const allocator)
  : factory(registry, allocator)
{

}

ChunkManager::~ChunkManager()
{

}
