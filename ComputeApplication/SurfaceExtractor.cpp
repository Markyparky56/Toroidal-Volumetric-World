#include "SurfaceExtractor.hpp"
#include "VulkanInterface.hpp"
#include "vk_mem_alloc.h"
#include "entt/entity/registry.hpp"

bool SurfaceExtractor::extractSurface(uint32_t entity, entt::DefaultRegistry * registry, std::mutex * const registryMutex, uint32_t frame)
{
  DualMCVoxel dmc;
  std::vector<Vertex> generatedVerts;
  std::vector<dualmc::TriIndexType> generatedIndices;

  constexpr uint16_t iso = static_cast<uint16_t>(0.5f * std::numeric_limits<uint16_t>::max());

  //void * volumeDataPtr;
  //if (vmaMapMemory(*volume.allocator, volume.volumeAllocation, &volumeDataPtr) != VK_SUCCESS)
  //{
  //  return false;
  //}
  //VmaAllocationInfo info;
  //vmaGetAllocationInfo(*volume.allocator, volume.volumeAllocation, &info);
  //vmaInvalidateAllocation(*volume.allocator, volume.volumeAllocation, 0, VK_WHOLE_SIZE);
  //std::array<Voxel, ChunkSize> volumeData;
  //memcpy(volumeData.data(), volumeDataPtr, ChunkSize * sizeof(Voxel));
  //vmaUnmapMemory(*volume.allocator, volume.volumeAllocation);
  registryMutex->lock();
  auto & volume = registry->get<VolumeData>(entity);
  dmc.buildTris(volume.volume.data(), TrueChunkDim, TrueChunkDim, TrueChunkDim, iso, true, false, generatedVerts, generatedIndices);
  registryMutex->unlock();

  size_t indexCount = generatedIndices.size(), vertexCount;
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  if (indexCount == 0)
  {
    registryMutex->lock();
    auto & modelData = registry->get<ModelData>(entity);
    modelData.indexCount = 0;
    registryMutex->unlock();

    return false;
  }
  else
  {
    // Mesh Optimiser Remap Stage
    std::vector<uint32_t> remap(indexCount);
    vertexCount = meshopt_generateVertexRemap(&remap[0], &generatedIndices[0], indexCount, &generatedVerts[0], generatedVerts.size(), sizeof(Vertex));

    indices.resize(indexCount);
    meshopt_remapIndexBuffer(&indices[0], &generatedIndices[0], indexCount, &remap[0]);

    vertices.resize(vertexCount);
    meshopt_remapVertexBuffer(&vertices[0], &generatedVerts[0], vertexCount, sizeof(Vertex), &remap[0]);
  }

  // Optional multi-level LOD generation can happen here,
  // See: https://github.com/zeux/meshoptimizer/blob/master/demo/main.cpp#L403 
  size_t targetIndexCount = static_cast<size_t>(indices.size() * 0.7f) / 3 * 3;
  float targetError = 1e-3f;
  indices.resize(meshopt_simplify(&indices[0], &indices[0], indices.size(), &vertices[0].pos.x, vertices.size(), sizeof(Vertex), targetIndexCount, targetError));

  // Optimise vertex cache
  meshopt_optimizeVertexCache(&indices[0], &indices[0], indices.size(), vertices.size());

  // Optimise overdraw
  meshopt_optimizeOverdraw(&indices[0], &indices[0], indices.size(), &vertices[0].pos.x, vertices.size(), sizeof(Vertex), 1.01f);
  
  // Optimise vertex fetch
  vertices.resize(meshopt_optimizeVertexFetch(&vertices[0], &indices[0], indices.size(), &vertices[0], vertices.size(), sizeof(Vertex)));

  // Generate normals
  generateNormals(vertices.data(), static_cast<uint32_t>(vertices.size()), indices.data(), static_cast<uint32_t>(indices.size()));

  // Fill model data
  auto[mutex, transferCommandBuffer] = commandPools->transferPools.getBuffer(frame);

  // Vertex Buffer
  registryMutex->lock();
  auto & modelData = registry->get<ModelData>(entity);
  if (!VulkanInterface::CreateBuffer(*modelData.allocator
    , sizeof(Vertex)*vertices.size()
    , VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
    , modelData.vertexBuffer
    , VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT
    , VMA_MEMORY_USAGE_GPU_ONLY
    , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    , VK_NULL_HANDLE
    , modelData.vbufferAllocation))
  {
    // TODO: "Failed to create vertex buffer for model data"
    mutex->unlock();
    registryMutex->unlock();
    return false;
  }
  // Fill Vertex Buffer
  if (!VulkanInterface::UseStagingBufferToUpdateBufferWithDeviceLocalMemoryBound(
      *logicalDevice
    , *modelData.allocator
    , sizeof(Vertex)*vertices.size()
    , vertices.data()
    , modelData.vertexBuffer
    , 0
    , 0
    , VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT
    , VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
    , VK_PIPELINE_STAGE_VERTEX_INPUT_BIT
    , *transferQueue
    , transferQMutex
    , *transferCommandBuffer
    , {}
    ))
  {
    // TODO: "Failed to stage vertex data"
    mutex->unlock();
    registryMutex->unlock();
    return false;
  }

  // Index buffer
  if (!VulkanInterface::CreateBuffer(*modelData.allocator
    , sizeof(uint32_t)*indices.size()
    , VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
    , modelData.indexBuffer
    , VMA_ALLOCATION_CREATE_STRATEGY_BEST_FIT_BIT
    , VMA_MEMORY_USAGE_GPU_ONLY
    , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    , VK_NULL_HANDLE
    , modelData.ibufferAllocation))
  {
    // TODO: "Failed to create vertex buffer for model data"
    mutex->unlock();
    registryMutex->unlock();
    return false;
  }
  // Fill Index Buffer
  if (!VulkanInterface::UseStagingBufferToUpdateBufferWithDeviceLocalMemoryBound(
      *logicalDevice
    , *modelData.allocator
    , sizeof(uint32_t)*indices.size()
    , indices.data()
    , modelData.indexBuffer
    , 0
    , 0
    , VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT
    , VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
    , VK_PIPELINE_STAGE_VERTEX_INPUT_BIT
    , *transferQueue
    , transferQMutex
    , *transferCommandBuffer
    , {}
  ))
  {
    // TODO: "Failed to stage vertex data"
    mutex->unlock();
    registryMutex->unlock();
    return false;
  }
  modelData.indexCount = static_cast<uint32_t>(indices.size());
  mutex->unlock();
  registryMutex->unlock();

  return true;
}
