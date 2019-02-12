#pragma once
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_vulkan.h"
#ifdef _WIN32
#include <Windows.h>
#endif

// Forward declare AppBase
class AppBase;

namespace VulkanInterface
{
#ifdef _WIN32
#define LIBRARY_TYPE HMODULE
#endif

  // Windows are handled differently depending on platform
  struct WindowParameters {
#ifdef VK_USE_PLATFORM_WIN32_KHR
    HINSTANCE HInstance;
    HWND HWnd;
#elif defined VK_USE_PLATFORM_XLIB_KHR
    Display * Dpy;
    Window Window;
#elif defined VK_USE_PLATFORM_XCB_KHR
    xcb_connection_t * Connection;
    xcb_window_t Window;
#endif
  };

  class WindowFramework 
  {
  public: 
    WindowFramework(const char * title, int x, int y, int width, int height, AppBase & app);
    virtual ~WindowFramework();
    virtual void Render() final;

  private:
    WindowParameters windowParameters;
    AppBase &app;
    bool created;
  };
}
