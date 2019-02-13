#pragma once
#include "AppBase.hpp"

class DemoApp : public AppBase
{
public:
  bool Initialise(VulkanInterface::WindowParameters windowParameters) override;
  bool Update() override;
  bool Resize() override;

private:
  bool setupGraphicsPipeline();

  void cleanupVulkan() override;
};
