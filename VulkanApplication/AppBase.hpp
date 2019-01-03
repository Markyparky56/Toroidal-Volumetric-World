#pragma once
#include "VulkanInterface.hpp"

using VulkanInterface::FrameResources;
using VulkanInterface::VulkanHandle;

class AppBase
{
protected:
  struct MouseState {
    struct ButtonsState {
      bool IsPressed;
      bool WasClicked;
      bool WasReleased;
    } Buttons[2];
    struct Position {
      int X, Y;
      struct Delta {
        int X, Y;
      } Delta;
    } Position;
    struct WheelState {
      bool WasMoved;
      float Distance;
    } Wheel;

    MouseState()
      : Buttons{ { false, false, false }, { false, false, false } }
      , Position{ 0, 0, {0, 0} }
      , Wheel{ false, 0.f }
    {}
  } MouseState;

  struct TimerState {
    float GetTime() const {
      auto duration = time.time_since_epoch();
      auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
      return static_cast<float>(ms * 0.001f);
    }
    float GetDeltaTime() const {
      return dt.count();
    }
    void Update() {
      auto previous_time = time;
      time = std::chrono::high_resolution_clock::now();
      dt = std::chrono::high_resolution_clock::now() - previous_time;
    }
    TimerState() {
      Update();
    }
  private:
    std::chrono::time_point<std::chrono::high_resolution_clock> time;
    std::chrono::duration<float> dt;
  } TimerState;
public: 
  AppBase();
  ~AppBase();

  virtual bool Initialise(VulkanInterface::WindowParameters window_parameters) = 0;
  virtual bool Update() = 0;
  virtual bool Resize();
  virtual void Shutdown();
  virtual void MouseClick(size_t buttonIndex, bool state);
  virtual void MouseMove(int x, int y);
  virtual void MouseWheel(float distance);
  virtual void MouseReset();
  virtual void UpdateTime();
  virtual bool IsReady();

protected:
  bool ready;
  virtual bool initVulkan(VulkanInterface::WindowParameters windowParameters
                        , VkImageUsageFlags swapchainImageUsage
                        , bool useDepth
                        , VkImageUsageFlags depthAttachmentUsage);
  virtual bool createSwapchain( VkImageUsageFlags swapchainImageUsage
                              , bool useDepth
                              , VkImageUsageFlags depthAttacmentUsage);
  void cleanupVulkan();
  virtual void OnMouseEvent();

  std::vector<char const *> desiredLayers = {
#ifdef _DEBUG
  "VK_LAYER_LUNARG_standard_validation"
#endif
  };

  std::vector<char const *> desiredExtensions = {
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#ifdef VK_USE_PLATFORM_WIN32_KHR
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif defined VK_USE_PLATFORM_XCB_KHR
    VK_KHR_XCB_SURFACE_EXTENSION_NAME,
#elif defined VK_USE_PLATFORM_XLIB_KHR
    VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
#endif
  };

  VkPhysicalDeviceFeatures desiredDeviceFeatures;

  LIBRARY_TYPE vulkanLibrary;

  VulkanHandle(VkInstance) vulkanInstance;
  VkPhysicalDevice vulkanPhysicalDevice;
  VulkanHandle(VkDevice) vulkanDevice;
  VulkanHandle(VkSurfaceKHR) presentationSurface;
  VulkanInterface::QueueParameters graphicsQueue;
  VulkanInterface::QueueParameters computeQueue;
  VulkanInterface::QueueParameters presentQueue;
  VulkanInterface::SwapchainParameters swapchain;
  VulkanHandle(VkCommandPool) commandPool;
  std::vector<VulkanHandle(VkImage)> depthImages;
  std::vector<VulkanHandle(VkDeviceMemory)> vkDeviceMemory;
  std::vector<FrameResources> frameResources;
  static uint32_t const numFrames;
  static VkFormat const depthFormat;

  VkDebugUtilsMessengerEXT callback;

  static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
      VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity
    , VkDebugUtilsMessageTypeFlagsEXT messageType
    , const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData
    , void* pUserData) {
    std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
  }
};
                                                                                                                                                                                                        