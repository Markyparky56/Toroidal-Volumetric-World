#pragma once
#ifdef _WIN32
#include <Windows.h>
#endif

#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <thread>
#include <functional>
#include <stdexcept>

#include "UnrecoverableException.hpp"

namespace VulkanCookbook
{
#ifdef _WIN32
#define LIBRARY_TYPE HMODULE
#endif

  
}
