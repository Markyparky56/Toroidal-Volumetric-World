#include "VulkanCookbook.hpp"
#include "VulkanFunctions.hpp"

namespace VulkanCookbook
{
  bool LoadVulkanLoaderLibrary(LIBRARY_TYPE &library)
  {
#if defined(_WIN32)
#include <Windows.h>
    library = LoadLibrary("vulkan-1.dll");
#elif defined(__linux)
    library = dlopen("libvulkan.so.1", RTLD_NOW);
#endif

    return (library != nullptr);
  }
}
