#pragma once
#include "taskflow\taskflow.hpp"
#include "FastNoise.h"
//#include "ComputePipeline.hpp" // Coming soon(tm)

class TerrainGenerator
{
public:
  TerrainGenerator();
  ~TerrainGenerator();


private:
  FastNoise noise;
};
