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
  initVulkan();
}

void App::initVulkan()
{
  // Load Library
  try
  {
    VulkanInterface::LoadVulkanLoaderLibrary(vulkanLibrary);
  }
  catch (std::runtime_error const &e)
  {
    throw UnrecoverableRuntimeException(CreateBasicExceptionMessage("Unable to load Vulkan Library"), e);
  }

  // Get vkGetInstanceProcAddr function so we can start loading functions
  try
  {
    VulkanInterface::LoadVulkanFunctionGetter(vulkanLibrary);
  }
  catch (std::runtime_error const &e)
  {
    throw UnrecoverableRuntimeException(CreateBasicExceptionMessage("Unable to load vkGetInstanceProcAddr function"), e);
  }

  // Load the global vulkan functions so we can create a Vulkan Instance
  try
  {
    VulkanInterface::LoadGlobalVulkanFunctions();
  }
  catch (std::runtime_error const &e)
  {
    throw UnrecoverableRuntimeException(CreateBasicExceptionMessage("Unable to load Global-Level Function"), e);
  } 

  // Create a Vulkan Instance tailored to our needs, with required extensions (and if validation layers if in debug)
  try
  {
    VulkanInterface::CreateVulkanInstance(desiredExtensions, desiredLayers, "VulkanApplication", vulkanInstance);
  }
  catch (std::runtime_error const &e)
  {
    throw UnrecoverableRuntimeException(CreateBasicExceptionMessage("Unable to create Vulkan Instance"), e);
  }

  // If we're in debug we can attach our debugCallback function now
#if defined(_DEBUG)
  try
  {
    VulkanInterface::SetupDebugCallback(vulkanInstance, debugCallback, callback);
  }
  catch (std::runtime_error const &e)
  {
    // Not really unrecoverable, but if this is failing in debug something is probably wrong
    throw UnrecoverableRuntimeException(CreateBasicExceptionMessage("Unable to attach debug callback to vulkan instance"), e);
  }
#endif
}

void App::cleanup()
{
#if defined(_DEBUG)
  VulkanInterface::DestroyDebugUtilsMessengerEXT(vulkanInstance, callback, nullptr);
#endif

  VulkanInterface::vkDestroyInstance(vulkanInstance, nullptr);
}
