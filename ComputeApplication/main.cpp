#include "ComputeApp.hpp"

int main(int argc, char* argv[])
{
#if !defined(_DEBUG) && defined(_WIN32)  && !defined(RELEASE_MODE_VALIDATION_LAYERS)
    FreeConsole();
#endif

    bool metricsEnabled = false;
    if (argc > 1)
    {
      if (strcmp(argv[1], "-metricsLogging") == 0)
      {
        metricsEnabled = true;
      }
    }

    ComputeApp app;
    if (metricsEnabled)
    {
      if (!app.InitMetrics())
      {
        return EXIT_FAILURE;
      }
    }
    VulkanInterface::WindowFramework window("Compute Pipeline App", 0, 0, 1920, 1080, app);
    window.Render();

    return EXIT_SUCCESS;
}
