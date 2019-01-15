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
  bool SetupFrameResources();

  void cleanupVulkan() override;

  VulkanHandle(VkCommandPool) graphicsCommandPool; // *
  VulkanHandle(VkCommandPool) computeCommandPool; // 
  std::vector<VkCommandBuffer>  graphicsCommandBuffers; // *
  std::vector<VkCommandBuffer> computeCommandBuffers;
  VulkanHandle(VkRenderPass) renderPass; // *
  

  std::vector<VulkanHandle(VkImage)> depthImages;
  std::vector<VulkanHandle(VkDeviceMemory)> depthImagesMemory;

  std::vector<FrameResources> frameResources;

  VulkanHandle(VkImage) image;
  VulkanHandle(VkDeviceMemory) imageMemory;
  VulkanHandle(VkImageView) imageView;

  VulkanHandle(VkPipeline) graphicsPipeline; // *
  VulkanHandle(VkPipeline) computePipeline;

  VulkanHandle(VkPipelineLayout) graphicsPipelineLayout; // *
  VulkanHandle(VkPipelineLayout) computePipelineLayout;

  VulkanHandle(VkDescriptorSetLayout) descriptorSetLayout; // 
  VulkanHandle(VkDescriptorPool) descriptorPool; // 
  std::vector<VkDescriptorSet> descriptorSets;

  VulkanHandle(VkBuffer) vertexBuffer; // *
  VulkanHandle(VkDeviceMemory) bufferMemory; // *

};
