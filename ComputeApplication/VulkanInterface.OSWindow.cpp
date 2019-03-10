#include "VulkanInterface.OSWindow.hpp"
#include "AppBase.hpp"

// ImGUI WndProcHandler Extern
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace VulkanInterface
{
#ifdef VK_USE_PLATFORM_WIN32_KHR
  enum class UserMessage {
    Resize = WM_USER + 1,
    Quit,
    Click,
    Move,
    Wheel,
    KeyDown,
    KeyUp
  };
  // Little helper macro to save on writing static casts everywhere
#define e2UINT(in) static_cast<UINT>(in)

  LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
  {
    //if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
    //{
    //  return true;
    //}

    switch (message)
    {
    case WM_LBUTTONDOWN: PostMessage(hWnd, e2UINT(UserMessage::Click), 0, 1); break;
    case WM_LBUTTONUP: PostMessage(hWnd, e2UINT(UserMessage::Click), 0, 0); break;
    case WM_RBUTTONDOWN: PostMessage(hWnd, e2UINT(UserMessage::Click), 1, 1); break;
    case WM_RBUTTONUP: PostMessage(hWnd, e2UINT(UserMessage::Click), 1, 0); break;
    case WM_MOUSEMOVE: PostMessage(hWnd, e2UINT(UserMessage::Move), LOWORD(lParam), HIWORD(lParam)); break;
    case RI_MOUSE_WHEEL: PostMessage(hWnd, e2UINT(UserMessage::Wheel), HIWORD(wParam), 0); break;
    case WM_SIZE: 
    case WM_EXITSIZEMOVE: PostMessage(hWnd, e2UINT(UserMessage::Resize), wParam, lParam); break;
    case WM_KEYDOWN: 
      if (VK_ESCAPE == wParam) {
        PostMessage(hWnd, e2UINT(UserMessage::Quit), wParam, lParam); break;
      }
      else {
        PostMessage(hWnd, e2UINT(UserMessage::KeyDown), wParam, lParam); break;
      }
    case WM_KEYUP: PostMessage(hWnd, e2UINT(UserMessage::KeyUp), wParam, lParam); break;
    case WM_CLOSE: PostMessage(hWnd, e2UINT(UserMessage::Quit), wParam, lParam); break;
    default:
      return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
  }

  WindowFramework::WindowFramework(const char * title, int x, int y, int width, int height, AppBase & app)
    : app(app)
    , created(false)
    , windowParameters()
  {
    windowParameters.HInstance = GetModuleHandle(nullptr);
    WNDCLASSEX windowClass = {
      sizeof(WNDCLASSEX),
      CS_HREDRAW | CS_VREDRAW,
      WindowProcedure,
      0,
      0,
      windowParameters.HInstance,
      nullptr,
      LoadCursor(nullptr, IDC_ARROW),
      reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1),
      nullptr,
      "VulkanInterface",
      nullptr
    };

    if (!RegisterClassEx(&windowClass))
    {
      return;
    }

    windowParameters.HWnd = CreateWindow("VulkanInterface", title, (WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU), x, y, width, height, nullptr, nullptr, windowParameters.HInstance, nullptr);
    if (!windowParameters.HWnd)
    {
      return;
    }

    created = true;
  }

  WindowFramework::~WindowFramework()
  {
    if (windowParameters.HWnd)
    {
      DestroyWindow(windowParameters.HWnd);
    }

    if (windowParameters.HInstance)
    {
      UnregisterClass("VulkanInterface", windowParameters.HInstance);
    }
  }

  void WindowFramework::Render()
  {
    if (created && app.Initialise(windowParameters))
    {
      ShowWindow(windowParameters.HWnd, SW_SHOWNORMAL);
      UpdateWindow(windowParameters.HWnd);

      MSG msg;
      bool running = true;

      while (running)
      {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
          switch (static_cast<UserMessage>(msg.message))
          {
          case UserMessage::Click:
            app.MouseClick(static_cast<size_t>(msg.wParam), msg.lParam > 0);
            break;
          case UserMessage::Move:
            app.MouseMove(static_cast<int>(msg.wParam), static_cast<int>(msg.lParam));
            break;
          case UserMessage::Wheel:
            app.MouseWheel(static_cast<short>(msg.wParam) * 0.002f);
            break;
          case UserMessage::KeyDown:
            app.KeyDown(static_cast<unsigned int>(msg.wParam));
            break;
          case UserMessage::KeyUp:
            app.KeyUp(static_cast<unsigned int>(msg.wParam));
            break;
          case UserMessage::Resize:
            if (!app.Resize())
            {
              running = false;
            }
            break;
          case UserMessage::Quit:
            running = false;
            break;
          }
          TranslateMessage(&msg);
          DispatchMessage(&msg);
        }
        else
        {
          if (app.IsReady())
          {
            app.UpdateTime();
            if (!app.Update())
            {
#if defined(_DEBUG)
              //abort();
              break;
#endif
            }
            app.MouseReset(); // Check abort above
          }
        }
      }
    }
   
    app.Shutdown();
  }

  // Add other platform specific window creation here
#elif VK_USER_PLATFORM_XLIB_KHR
#elif VK_USER_PLATFORM_XCB_KHR
#endif
}
