#include "TerrainGenerator.hpp"

std::array<Voxel, TrueChunkDim> TerrainGenerator::getChunkVolume(glm::vec3 chunkPos)
{
  std::array<Voxel, TrueChunkDim> volume;

  //for (float y = chunkPos.y - HalfChunkDim; y <= chunkPos.z + HalfChunkDim; y += 1.f)
  //{
  //  for (float z = chunkPos.z - HalfChunkDim; z <= chunkPos.z + HalfChunkDim; z += 1.f)
  //  {
  //    for (float x = chunkPos.x - HalfChunkDim; x <= chunkPos.x + HalfChunkDim; x += 1.f)
  //    {
  //      float theta = chunkPos.x * 2.0 * static_cast<float>(PI);
  //      float phi = chunkPos.z * 2.0 * static_cast<float>(PI);
  //      float px = std::cosf(theta)
  //          , py = std::sinf(theta)
  //          , pz = std::cosf(phi)
  //          , pw = std::sinf(phi)
  //          , pv = y;
  //      float height = noise.GetSimplexFractal(px, py, pz, pw);
  //      float noise = ScaleBias(px, py, pz, pw, pv, 1.f, height);
  //      volume[(z*y*TrueChunkDim) + (y*TrueChunkDim) + x].density = static_cast<uint16_t>(noise * std::numeric_limits<uint16_t>::max());
  //    }
  //  }
  //}

  // Normalise chunk position
  glm::vec3 normedChunkPos = chunkPos * invWorldDimensionInVoxels;
  constexpr float voxelStep = invWorldDimension * invTechnicalChunkDim;
  constexpr float normedHalfChunkDim = static_cast<float>(HalfChunkDim) * invWorldDimensionInVoxels;
  std::array<float, TrueChunkDim*TrueChunkDim> heightmap;
  // Calculate height map (this can/should be GPU compute)
  uint32_t hm_p = 0;
  for (float z = normedChunkPos.z - normedHalfChunkDim; z <= normedChunkPos.z + normedHalfChunkDim; z += voxelStep)
  {
    for (float x = normedChunkPos.x - normedHalfChunkDim; x <= normedChunkPos.x + normedHalfChunkDim; x += voxelStep, hm_p++)
    {
      float theta = x * 2.0 * static_cast<float>(PI);
      float phi = z * 2.0 * static_cast<float>(PI);
      float h_amp = 1.0f;
      float h_r = 32.f;
      float height = 0.0f;      
      for (int i = 0; i < 3; i++)
      {
        glm::vec4 p = glm::vec4(
          h_r * std::cos(theta),
          h_r * std::sin(theta),
          h_r * std::cos(phi),
          h_r * std::sin(phi)
        );
        height += h_amp * noise.GetSimplex(p.x, p.y, p.z, p.w);
        h_amp *= 0.65f;
        h_r *= 2.0f;
      }
      heightmap[hm_p] = height;
    }
  }

  return volume;
}

inline FN_DECIMAL TerrainGenerator::ScaleBias(float x, float y, float z, float w, float v, float scale, float bias)
{
  return noise.GetSimplexFractal(x, y, z, w, v) * scale + bias;
}
