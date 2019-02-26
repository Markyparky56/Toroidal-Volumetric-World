#pragma once
#include <glm\glm.hpp>
#include "vk_mem_alloc.h"
#include <array>

struct VolumeData
{
  VkBuffer volumeBuffer;
  VmaAllocation volumeAllocation;
  VmaAllocator * allocator;

  //VolumeData(VkBuffer vbuf, VmaAllocation vbufAlloc, VmaAllocator * allocator)
  //  : volumeBuffer(vbuf)
  //  , volumeAllocation(vbufAlloc)
  //  , allocator(allocator)
  //{}

  ~VolumeData()
  {
    vmaDestroyBuffer(*allocator, volumeBuffer, volumeAllocation);
  }
};

struct ModelData
{
  VkBuffer vertexBuffer, indexBuffer;
  VmaAllocation vbufferAllocation, ibufferAllocation;
  VmaAllocator * allocator;

  //ModelData(VkBuffer vbuf, VkBuffer ibuf, VmaAllocation vbufAlloc, VmaAllocation ibufAlloc, VmaAllocator * allocator)
  //  : vertexBuffer(vbuf)
  //  , indexBuffer(ibuf)
  //  , vbufferAllocation(vbufAlloc)
  //  , ibufferAllocation(ibufAlloc)
  //  , allocator(allocator)
  //{}

  ~ModelData()
  {
    vmaDestroyBuffer(*allocator, vertexBuffer, vbufferAllocation);
    vmaDestroyBuffer(*allocator, indexBuffer, ibufferAllocation);
  }
};

struct WorldPosition
{
  glm::vec3 pos;
};

struct AABB
{
  float wdith, height, depth;
};

//struct Frustum
//{
//  std::array<glm::vec4, 6> planes;
//};
//
//struct Controller
//{
//
//};
