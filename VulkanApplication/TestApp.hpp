#pragma once
#include "AppBase.hpp"

class TestApp : public AppBase 
{
public:
  bool Initialise(VulkanInterface::WindowParameters windowParameters) override;
  bool Update() override;
  bool Resize() override;
private:
  bool SetupGraphicsPipeline();
  bool SetupComputePipeline();

  bool SetupGraphicsBuffers();

  void cleanupVulkan() override;

  VulkanHandle(VkCommandPool) graphicsCommandPool; // *
  VulkanHandle(VkCommandPool) computeCommandPool; // 
  VkCommandBuffer graphicsCommandBuffer; // *
  //VkCommandBuffer computeCommandBuffer;
  VulkanHandle(VkRenderPass) renderPass; // *
  //VulkanHandle(VkFramebuffer) framebuffer;

  //VulkanHandle(VkImage) image;
  //VulkanHandle(VkDeviceMemory) imageMemory;
  //VulkanHandle(VkImageView) imageView;

  VulkanHandle(VkPipeline) graphicsPipeline; // *
  //VulkanHandle(VkPipeline) computePipeline;

  VulkanHandle(VkPipelineLayout) graphicsPipelineLayout; // *
  //VulkanHandle(VkPipelineLayout) computePipelineLayout;

  //VulkanHandle(VkDescriptorSetLayout) descriptorSetLayout; // 
  //VulkanHandle(VkDescriptorPool) descriptorPool; // 
  //std::vector<VkDescriptorSet> descriptorSets;

  VulkanHandle(VkBuffer) vertexBuffer; // *
  VulkanHandle(VkDeviceMemory) bufferMemory; // *
  VulkanHandle(VkFence) drawingFence; // *
  VulkanHandle(VkSemaphore) imageAcquiredSemaphore; // *
  VulkanHandle(VkSemaphore) readyToPresentSemaphore; // *
};
