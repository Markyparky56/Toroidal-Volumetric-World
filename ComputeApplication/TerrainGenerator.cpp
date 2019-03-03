#include "TerrainGenerator.hpp"

std::array<Voxel, TrueChunkDim> TerrainGenerator::getChunkVolume(glm::vec3 chunkPos)
{
  std::array<Voxel, TrueChunkDim> volume;

  for (float y = chunkPos.y - HalfChunkDim; y <= chunkPos.z + HalfChunkDim; y += 1.f)
  {
    for (float z = chunkPos.z - HalfChunkDim; z <= chunkPos.z + HalfChunkDim; z += 1.f)
    {
      for (float x = chunkPos.x - HalfChunkDim; x <= chunkPos.x + HalfChunkDim; x += 1.f)
      {
        float theta = chunkPos.x * 2.0 * static_cast<float>(PI);
        float phi = chunkPos.z * 2.0 * static_cast<float>(PI);
        float px = std::cosf(theta)
            , py = std::sinf(theta)
            , pz = std::cosf(phi)
            , pw = std::sinf(phi)
            , pv = y;
        float height = noise.GetSimplexFractal(px, py, pz, pw);
        float noise = ScaleBias(px, py, pz, pw, pv, 1.f, height);
        volume[(z*y*TrueChunkDim) + (y*TrueChunkDim) + x].density = static_cast<uint16_t>(noise * std::numeric_limits<uint16_t>::max());
      }
    }
  }

  return volume;
}

inline FN_DECIMAL TerrainGenerator::ScaleBias(float x, float y, float z, float w, float v, float scale, float bias)
{
  return noise.GetSimplexFractal(x, y, z, w, v) * scale + bias;
}
