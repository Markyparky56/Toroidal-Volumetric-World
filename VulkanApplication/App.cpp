#include "App.hpp"

App::App()
  : vulkanLibrary(nullptr)
{

}

App::~App()
{

}

void App::run()
{
  auto result = VulkanCookbook::LoadVulkanLoaderLibrary(vulkanLibrary);
  if (!result)
  {
    throw UnrecoverableRuntimeException(CreateBasicExceptionMessage("Unable to load Vulkan Library"), "VulkanCookbook::LoadVulkanLoaderLibrary");
  }
}
