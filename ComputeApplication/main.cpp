#include "ComputeApp.hpp"

int main()
{
#if !defined(_DEBUG) && defined(_WIN32)
    FreeConsole();
#endif
    ComputeApp app;
    VulkanInterface::WindowFramework window("Compute Pipeline App", 0, 0, 1920, 1080, app);
    window.Render();

    return EXIT_SUCCESS;
}
