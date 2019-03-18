#pragma once
#include "taskflow\taskflow.hpp"
#include "FastNoise.h"
#include <array>
#include "voxel.hpp"
#include "common.hpp"
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
//#include "ComputePipeline.hpp" // Coming soon(tm)

class TerrainGenerator
{
public:
  TerrainGenerator()
  {
    noise.SetSeed(4422);
    noise.SetFractalType(FastNoise::FractalType::FBM);
  }
  ~TerrainGenerator() {}

  void SetSeed(int seed)
  {
    noise.SetSeed(seed);
  }

  std::array<Voxel, ChunkSize> getChunkVolume(glm::vec3 chunkPos);

private:
   FastNoise noise;

   inline FN_DECIMAL ScaleBias(float x, float y, float z, float w, float v, float scale, float bias);

};
