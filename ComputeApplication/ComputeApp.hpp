#pragma once
#include "AppBase.hpp"
#include "GraphicsPipeline.hpp"
#include "taskflow\taskflow.hpp"
#include "vk_mem_alloc.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_vulkan.h"

class ComputeApp : public AppBase
{
public:
  bool Initialise(VulkanInterface::WindowParameters windowParameters) override;
  bool Update() override;
  bool Resize() override;

private:
  bool setupVulkanAndCreateSwapchain(VulkanInterface::WindowParameters windowParameters);
  bool setupTaskflow();
  bool initialiseVulkanMemoryAllocator();
  bool initImGui(HWND hwnd);
  bool setupCommandPoolAndBuffers();
  bool setupRenderPass();
  bool setupGraphicsPipeline();
  bool setupTerrainGenerator();

  void Shutdown() override;
  void shutdownVulkanMemoryAllocator();
  void shutdownImGui();
  void cleanupVulkan();

  // cpp-taskflow taskflows and shared executor
  std::unique_ptr<tf::Taskflow> graphicsTaskflow, computeTaskflow, systemTaskflow;
  std::shared_ptr<tf::Taskflow::Executor> tfExecutor;

  // Vulkan Memory Allocator
  VmaAllocator allocator;

  VkDescriptorPool imGuiDescriptorPool;

  std::unique_ptr<GraphicsPipeline> graphicsPipeline; // This could feasibly be a vector if we ever want a more complex render setup
                                     // But having explicitly named pipelines would probably be better

  static constexpr uint32_t chunkViewDistance = 7;
  static constexpr uint32_t maxChunks = chunkViewDistance * chunkViewDistance * chunkViewDistance;

  VkRenderPass renderPass;
  std::vector<VulkanInterface::FrameResources> frameResources;
  VkCommandPool graphicsCommandPool;
  std::vector<VkCommandBuffer> frameCommandBuffers;
  std::vector<VkCommandBuffer> chunkCommandBuffers;
  //uint32_t graphicsQueueFamily, presentQueueFamily, computeQueueFamily;
  VkQueue graphicsQueue, presentQueue;
  std::vector<VkQueue> computeQueues;
};