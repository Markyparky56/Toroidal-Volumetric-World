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
  std::vector<VkDescriptorSetLayout> layouts;
  std::vector<VkPushConstantRange> pushConstantRanges;

protected:
  VkRenderPass * const renderPass;

  std::string vertexShaderLoc, fragmentShaderLoc;  

};
