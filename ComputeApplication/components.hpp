#pragma once
#include <glm\glm.hpp>
#include "vk_mem_alloc.h"
#include <array>
#include "common.hpp"
#include "voxel.hpp"
#include "VulkanInterface.hpp"

struct VolumeData
{
  VkBuffer volumeBuffer;
  VkBufferView volumeBufferView;
  VmaAllocation volumeAllocation;
  VmaAllocator * allocator;

  bool init(VmaAllocator * allocatorPtr, VkPhysicalDevice * physicalDevice, VkDevice * logicalDevice)
  {
    allocator = allocatorPtr;
    volumeBufferView = VkBufferView();
    return VulkanInterface::CreateUniformTexelBuffer(
        *physicalDevice
      , *logicalDevice
      , *allocator
      , VK_FORMAT_R32_SFLOAT
      , sizeof(std::array<Voxel, ChunkSize>)
      , VK_IMAGE_USAGE_STORAGE_BIT
      , volumeBuffer
      , VMA_MEMORY_USAGE_CPU_TO_GPU
      , volumeAllocation
      , volumeBufferView
    );
  }

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
  uint32_t indexCount;

  ModelData(VkBuffer vbuf, VkBuffer ibuf, VmaAllocation vbufAlloc, VmaAllocation ibufAlloc, VmaAllocator * allocator, uint32_t idc)
    : vertexBuffer(vbuf)
    , indexBuffer(ibuf)
    , vbufferAllocation(vbufAlloc)
    , ibufferAllocation(ibufAlloc)
    , allocator(allocator)
    , indexCount(idc)
  {}

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
