#pragma once
#include "VulkanInterface.hpp"
#include <vector>
#include <array> // For each frame
#include <stack> // Pile of available command buffers
#include <mutex>
#include <tuple>

// Helper for getting a free command buffer for transfering data to the GPU, 
// not the best construction in the world but provided you have a dedicated 
// transfer pipeline you shouldn't have any issues even if you're sending 
// data from various threads
class TaskflowCommandPools
{
public:
  using cbufferPools = std::vector<std::array<VkCommandPool, 3>>;

  struct TransferPool
  {
  private:
    cbufferPools pools;
    std::vector<std::array<VkCommandBuffer, 3>> buffers;
    std::vector<std::mutex> mutexes;
    VkDevice * const logicalDevice;
    uint32_t numWorkers;
    std::mutex searchMutex;
  public:
    TransferPool(uint32_t numWorkers, uint32_t family, VkDevice * const logicalDevice)
      : logicalDevice(logicalDevice)
      , numWorkers(numWorkers)
    {
      pools = cbufferPools(numWorkers);
      buffers = std::vector<std::array<VkCommandBuffer, 3>>(numWorkers);
      mutexes = std::vector<std::mutex>(numWorkers);

      for (int thread = 0; thread < numWorkers; thread++)
      {
        for (int frame = 0; frame < 3; frame++)
        {
          // Setup pool
          VkCommandPool & pool = pools[thread][frame];
          if (!VulkanInterface::CreateCommandPool(*logicalDevice
            , VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
            , family
            , pool))
          {
            return;
          }

          // Allocate buffers for new pool
          VkCommandBuffer & buffer = buffers[thread][frame];
          std::vector<VkCommandBuffer> b;
          if (!VulkanInterface::AllocateCommandBuffers(*logicalDevice
            , pool
            , VK_COMMAND_BUFFER_LEVEL_PRIMARY
            , 1
            , b))
          {
            return;
          }

          buffer = b[0];
        }
      }
    }

    // DONT FORGET TO UNLOCK THE MUTEX
    std::tuple<std::mutex * const, VkCommandBuffer * const> getBuffer(uint32_t frame)
    {
      // Find an unlocked thread
      uint32_t thread = 0;
      {
        std::unique_lock<std::mutex> lock(searchMutex);
        for (auto & mutex : mutexes)
        {
          if (mutex.try_lock())
          {
            VkCommandBuffer * cbuf = &buffers[thread][frame];
            return std::make_tuple(&mutex, cbuf);
          }
          else
          {
            thread++;
          }
        }
      }      
      // If we've fallen out of the loop something is probably wrong but just return the last mutex and hope it unlocks
      mutexes[thread - 1].lock();
      VkCommandBuffer * cbuf = &buffers[thread-1][frame];
      return std::make_pair(&mutexes[thread-1], cbuf);
    }

    void cleanup()
    {
      for (auto & thread : pools)
      {
        for (auto & pool : thread)
        {
          VulkanInterface::DestroyCommandPool(*logicalDevice, pool);
        }
      }
    }
  } transferPools;

  TaskflowCommandPools(
      VkDevice * const logicalDevice
    , uint32_t transferQueueFamily
    , uint32_t numWorkers
  )
    : logicalDevice(logicalDevice)
    , transferPools(numWorkers, transferQueueFamily, logicalDevice)
  {
    
  }

  void cleanup()
  {
    transferPools.cleanup();
  }

protected:
  VkDevice * const logicalDevice;
  uint32_t transferQueueFamily;
};
