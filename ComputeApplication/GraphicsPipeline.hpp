#pragma once
#include "PipelineBase.hpp"
#include <string>
#include <vector>

class GraphicsPipeline : public PipelineBase
{
public:
  GraphicsPipeline(VkDevice * const logicalDevice, VkRenderPass * const renderPass, std::string vertexShaderLoc, std::string fragmentShaderLoc);
  ~GraphicsPipeline();
  void cleanup() override;

  bool init();
  bool loadVertexShader(VkShaderModule & vertexShader);
  bool loadFragmentShader(VkShaderModule & fragmentShader);

  bool setupDescriptorPool(std::vector<VkDescriptorPoolSize> & poolSizes);

  VkDescriptorPool descriptorPool;
  std::vector<std::vector<VkDescriptorSetLayoutBinding>> layoutBindings;
  std::vector<VkDescriptorSetLayout> layouts;
  std::vector<VkDescriptorSet> descriptorSets;
  std::vector<VkPushConstantRange> pushConstantRanges;
  VkBuffer * uniformBuffer;

protected:
  VkRenderPass * const renderPass;
  VkShaderModule vertShader = VK_NULL_HANDLE, fragShader = VK_NULL_HANDLE;

  std::string vertexShaderLoc, fragmentShaderLoc;  

};
