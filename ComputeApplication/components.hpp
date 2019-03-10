#pragma once
#include <glm\glm.hpp>
#include "vk_mem_alloc.h"
#include <array>

struct VolumeData
{
  VkBuffer volumeBuffer;
  VmaAllocation volumeAllocation;
  VmaAllocator * allocator;

  ~VolumeData()
  {
    vmaDestroyBuffer(*allocator, volumeBuffer, volumeAllocation);
  }
};

struct ModelData
{
  bool hasModelData;
  VkBuffer vertexBuffer, indexBuffer;
  VmaAllocation vbufferAllocation, ibufferAllocation;
  VmaAllocator * allocator;
  uint32_t indexCount;

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
