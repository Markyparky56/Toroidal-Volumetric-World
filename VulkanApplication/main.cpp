#include "TestApp.hpp"

int main()
{
#if !defined(_DEBUG) && defined(_WIN32)
  FreeConsole();
#endif
#if defined(_DEBUG)
  try
  {
#endif
    TestApp app;
    VulkanInterface::WindowFramework window("VulkanInterface App", 50, 25, 1280, 800, app);
    window.Render();
#if defined(_DEBUG)
  }
  catch (UnrecoverableRuntimeException const &e)
  {
    std::cerr << "Unrecoverable Exception!\n"
      << "What: " << e.what() << "\n"
      << e.message()
      << "\nFile: " << e.fileName()
      << "\nFunction: " << e.funcName()
      << "\nLine: " << e.lineNumber() << std::endl;
    return EXIT_FAILURE;
  }
  catch (UnrecoverableVulkanException const &e)
  {
    std::cerr << "Unrecoverable Exception!\n"
      << "What: " << e.what() << "\n"
      << "Code: " << e.code().value() << "\n"
      << e.message()
      << "\nFile: " << e.fileName()
      << "\nFunction: " << e.funcName()
      << "\nLine: " << e.lineNumber() << std::endl;
    return EXIT_FAILURE;
  }
  catch (std::runtime_error const &e)
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
#endif

  return EXIT_SUCCESS;
}
