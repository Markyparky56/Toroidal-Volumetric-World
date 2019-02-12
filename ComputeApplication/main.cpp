#include "ComputeApp.hpp"

int main()
{
#if !defined(_DEBUG) && defined(_WIN32)
    FreeConsole();
#endif
    ComputeApp app;
    VulkanInterface::WindowFramework window("Compute Pipeline App", 50, 25, 1280, 800, app);
    window.Render();

    return EXIT_SUCCESS;
}
