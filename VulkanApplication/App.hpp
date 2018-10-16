#pragma once
#include "VulkanCookbook.hpp"

class App
{
public: 
  App();
  ~App();

  void run();

private:
  LIBRARY_TYPE vulkanLibrary;
};
