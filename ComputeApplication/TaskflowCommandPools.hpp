#pragma once
#include "VulkanInterface.hpp"
#include <vector> // For each thread
#include <array> // For each frame
#include <stack> // Pile of available command buffers
#include <mutex>

class TaskflowCommandPools
{
public:
  using cbufferPools = std::vector<std::array<VkCommandPool, 3>>;
  using cbufferPoolBuffers = std::vector<std::array<std::vector<VkCommandBuffer>, 3>>;
  using cbufferStacks = std::vector<std::array<std::stack<VkCommandBuffer*>, 3>>;
  using cbufferPoolMutexes = std::vector<std::mutex>;
  struct Pools {
  private:
    cbufferPools pools; // 3 pools per thread
    cbufferPoolBuffers buffers; // X number of buffers per pool, per frame, per thread
    cbufferStacks stacks; // Stack of pointers to each inner vector set of buffers
    cbufferPoolMutexes mutexes; // Mutexes for each thread
    VkDevice * const logicalDevice;
    uint32_t numWorkers, bufferCount;
    VkCommandBufferLevel bufferLevel;
  public:
    Pools(uint32_t numWorkers, uint32_t family, VkDevice * const logicalDevice, VkCommandBufferLevel bufferLevel, uint32_t bufferCount)
      : logicalDevice(logicalDevice)
      , numWorkers(numWorkers)
      , bufferCount(bufferCount)
      , bufferLevel(bufferLevel)
    {
      pools = cbufferPools(numWorkers);
      buffers = cbufferPoolBuffers(numWorkers);
      stacks = cbufferStacks(numWorkers);
      mutexes = cbufferPoolMutexes(numWorkers);

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
          std::vector<VkCommandBuffer> & bufferVec = buffers[thread][frame];
          if (!VulkanInterface::AllocateCommandBuffers(*logicalDevice
            , pool
            , bufferLevel
            , bufferCount
            , bufferVec))
          {
            return;
          }

          // Fill stack
          std::stack<VkCommandBuffer*> & stack = stacks[thread][frame];
          for (auto & buffer : bufferVec)
          {
            stack.push(&buffer);
          }
        }
      }
    }
    
    // Calling this function returns a mutex and a command buffer, the intention is to record the command buffer
    // then unlock the mutex. Failing to unlock the mutex will mean that set of pools won't be usable again.
    // DONT FORGET TO UNLOCK THE MUTEX
    std::pair<std::mutex * const, VkCommandBuffer * const> getBuffer(uint32_t frame)
    {
      // Find an unlocked thread
      uint32_t thread = 0;
      for (auto & mutex : mutexes)
      {
        if (mutex.try_lock())
        {
          VkCommandBuffer * cbuf = stacks[thread][frame].top();
          stacks[thread][frame].pop();
          return std::make_pair(&mutex, cbuf);
        }
        else
        {
          thread++;
        }
      }
      // If we've fallen out of the loop something is probably wrong but just return the last mutex and hope it unlocks
      VkCommandBuffer * cbuf = stacks[thread][frame].top();
      stacks[thread][frame].pop();
      return std::make_pair(&mutexes[thread], cbuf);
    }

    bool resetFramePools(uint32_t frame)
    {
      for (int thread = 0; thread < numWorkers; thread++)
      {
        // Reset pool
        VkCommandPool & pool = pools[thread][frame];
        if (!VulkanInterface::ResetCommandPool(*logicalDevice, pool, true))
        {
          return false;
        }

        // Reallocate buffers
        std::vector<VkCommandBuffer> & bufferVec = buffers[thread][frame];
        bufferVec.clear();
        bufferVec.resize(bufferCount);
        if (!VulkanInterface::AllocateCommandBuffers(
            *logicalDevice
          , pool
          , bufferLevel
          , bufferCount
          , bufferVec))
        {
          return false;
        }

        // Refill stacks
        std::stack<VkCommandBuffer*> & stack = stacks[thread][frame];
        for (auto & buffer : bufferVec)
        {
          stack.push(&buffer);
        }
      }
    }

  } framePools, graphicsPools, transferPools/*, computePools*/;

  TaskflowCommandPools(
      VkDevice * const logicalDevice
    , uint32_t graphicsQueueFamily
    , uint32_t transferQueueFamily
  /*, uint32_t computeQueueFamily*/
    , uint32_t numWorkers
  )
    : logicalDevice(logicalDevice)
    , framePools(numWorkers, graphicsQueueFamily, logicalDevice, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 3)
    , graphicsPools(numWorkers, graphicsQueueFamily, logicalDevice, VK_COMMAND_BUFFER_LEVEL_SECONDARY, 300)
    , transferPools(numWorkers, transferQueueFamily, logicalDevice, VK_COMMAND_BUFFER_LEVEL_PRIMARY, numWorkers)
  {
    
  }

protected:
  VkDevice * const logicalDevice;
  uint32_t graphicsQueueFamily, transferQueueFamily, computeQueueFamily;
};
