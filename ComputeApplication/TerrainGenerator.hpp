#pragma once
#include "taskflow\taskflow.hpp"
#include "FastNoise.h"
#include <array>
#include "voxel.hpp"
#include "common.hpp"
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/common.hpp>
#include <glm/mat4x4.hpp>
#include "metrics.hpp"
//#include "ComputePipeline.hpp" // Coming soon(tm)

class TerrainGenerator
{
  using Volume = std::array<Voxel, ChunkSize>;
  using HeightMap = std::array<float, TrueChunkDim*TrueChunkDim>;

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
  std::array<Voxel, ChunkSize> getChunkVolume(glm::vec3 chunkPos, logEntryData & data);

  void genHeightMap(HeightMap & heightmap, glm::vec3 normedChunkPos);
  void genVolume(HeightMap& heightmap, Volume & volume, glm::vec3 chunkPos, glm::vec3 normedChunkPos);

private:
   FastNoise noise;
};
