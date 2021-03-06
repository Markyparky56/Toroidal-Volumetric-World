#pragma once
#include <glm\glm.hpp>
#include "vk_mem_alloc.h"
#include <array>
#include <atomic>
#include "common.hpp"
#include "voxel.hpp"
#include "VulkanInterface.hpp"

struct VolumeData
{
  //VkBuffer volumeBuffer;
  //VkBufferView volumeBufferView;
  //VmaAllocation volumeAllocation;
  //VmaAllocator * allocator;

  //bool init(VmaAllocator * allocatorPtr, VkPhysicalDevice * physicalDevice, VkDevice * logicalDevice)
  //{
  //  allocator = allocatorPtr;
  //  volumeBufferView = VkBufferView();
  //  return VulkanInterface::CreateUniformTexelBuffer(
  //      *physicalDevice
  //    , *logicalDevice
  //    , *allocator
  //    , VK_FORMAT_R32_SFLOAT
  //    , sizeof(std::array<Voxel, ChunkSize>)
  //    , VK_IMAGE_USAGE_STORAGE_BIT
  //    , volumeBuffer
  //    , VMA_MEMORY_USAGE_CPU_TO_GPU
  //    , volumeAllocation
  //    , volumeBufferView
  //  );
  //}

  std::array<Voxel, ChunkSize> volume;
  bool generating;

  //void destroy()
  //{
  //  //vmaDestroyBuffer(*allocator, volumeBuffer, volumeAllocation);
  //}

  //~VolumeData()
  //{
  //}
};

struct ModelData
{
  VkBuffer vertexBuffer, indexBuffer;
  VmaAllocation vbufferAllocation, ibufferAllocation;
  VmaAllocator * allocator;
  uint32_t indexCount;

  //ModelData(VkBuffer vbuf, VkBuffer ibuf, VmaAllocation vbufAlloc, VmaAllocation ibufAlloc, VmaAllocator * allocator, uint32_t idc)
  //  : vertexBuffer(vbuf)
  //  , indexBuffer(ibuf)
  //  , vbufferAllocation(vbufAlloc)
  //  , ibufferAllocation(ibufAlloc)
  //  , allocator(allocator)
  //  , indexCount(idc)
  //{
  //  std::cout << allocator << std::endl;
  //}

  void destroy()
  {
    vmaDestroyBuffer(*allocator, vertexBuffer, vbufferAllocation);
    vmaDestroyBuffer(*allocator, indexBuffer, ibufferAllocation);
  }

  ~ModelData()
  {
  }
};

struct WorldPosition
{
  glm::vec3 pos;
};

struct AABB
{
  float width, height, depth;
};

struct Flags
{
  bool needsUnloaded; // Flag set by chunk manager when outside despawn range
  // Decremented each frame to a minimum of 0, 
  // Incremented each time it is added to the draw list
  // If 0 should be safe to unload
  std::uint32_t framesQueued; 
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
