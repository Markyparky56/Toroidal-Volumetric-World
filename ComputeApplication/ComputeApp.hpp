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
  bool setupGraphicsPipeline();

  void shutdownVulkanMemoryAllocator();
  void shutdownImGui();
  void cleanupVulkan();

  // cpp-taskflow taskflows and shared executor
  std::unique_ptr<tf::Taskflow> graphicsTaskflow, computeTaskflow, systemTaskflow;
  std::shared_ptr<tf::Taskflow::Executor> tfExecutor;

  // Vulkan Memory Allocator
  VmaAllocator allocator;

  VulkanHandle(VkDescriptorPool) imGuiDescriptorPool;

  GraphicsPipeline graphicsPipeline; // This could feasibly be a vector if we ever want a more complex render setup
                                     // But having explicitly named pipelines would probably be better

  VulkanHandle(VkRenderPass) renderPass;
  VkQueue graphicsQueue, presentQueue;
  std::vector<VkQueue> computeQueues;
};