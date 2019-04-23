#include "TerrainGenerator.hpp"
#include <glm/common.hpp>
#include <glm/mat4x4.hpp>

std::array<Voxel, ChunkSize> TerrainGenerator::getChunkVolume(glm::vec3 chunkPos)
{
  Volume volume;
  HeightMap heightmap;

  // Normalise chunk position
  glm::vec3 normedChunkPos = chunkPos * invWorldDimensionInVoxels;
  genHeightMap(heightmap, normedChunkPos);
  genVolume(heightmap, volume, chunkPos, normedChunkPos);

  return volume;
}

std::array<Voxel, ChunkSize> TerrainGenerator::getChunkVolume(glm::vec3 chunkPos, logEntryData & data)
{
  Volume volume;
  HeightMap heightmap;  

  // Normalise chunk position
  glm::vec3 normedChunkPos = chunkPos * invWorldDimensionInVoxels;

  data.heightStart = hr_clock::now();
  genHeightMap(heightmap, normedChunkPos);
  data.heightEnd = hr_clock::now();

  data.volumeStart = hr_clock::now();
  genVolume(heightmap, volume, chunkPos, normedChunkPos);  
  data.volumeEnd = hr_clock::now();

  return volume;
}

// Calculate basic height map (this can/should be GPU compute for more complex multi biome setups)
void TerrainGenerator::genHeightMap(HeightMap& heightmap, glm::vec3 normedChunkPos)
{
  constexpr float voxelStep = invWorldDimension * invTechnicalChunkDim;
  constexpr float normedHalfChunkDim = static_cast<float>(HalfChunkDim) * invWorldDimensionInVoxels;

  uint32_t hm_p = 0;
  for (float z = normedChunkPos.z - normedHalfChunkDim; z < normedChunkPos.z + normedHalfChunkDim; z += voxelStep)
  {
    for (float x = normedChunkPos.x - normedHalfChunkDim; x < normedChunkPos.x + normedHalfChunkDim; x += voxelStep, ++hm_p)
    {
      float theta = x * 2.0f * static_cast<float>(PI);
      float phi = z * 2.0f * static_cast<float>(PI);
      float h_amp = 1.0f;
      float h_r = 64.f;
      float height = 0.0f;
      {
        {
          glm::vec4 p = glm::vec4(
            h_r * std::cos(theta),
            h_r * std::sin(theta),
            h_r * std::cos(phi),
            h_r * std::sin(phi)
          );
          height = h_amp * (1.f - glm::abs(noise.GetSimplex(p.x, p.y, p.z, p.w)));
          h_amp *= 0.8f;
          h_r *= 2.0f;
        }
        for (int i = 0; i < 1; i++)
        {
          glm::vec4 p = glm::vec4(
            h_r * std::cos(theta),
            h_r * std::sin(theta),
            h_r * std::cos(phi),
            h_r * std::sin(phi)
          );
          height -= h_amp * (1.f - glm::abs(noise.GetSimplex(p.x, p.y, p.z, p.w)));
          h_amp *= 0.4f;
          h_r *= 2.45f;
        }
      }
      // Apply terracing for some interesting terrain features
      // Via: https://gamedev.stackexchange.com/a/116222/53817
      float w = 0.2f;
      float k = glm::floor(height / w);
      float f = (height - k * w) / w;
      float s = glm::min(2.f*f, 1.f);
      height = ((k + s) * w);
      height = glm::clamp(height, -1.f, 1.f);
      heightmap[hm_p] = (height*.5f) + .5f; // ensure heightmap range is [0,1]
    }
  }
}

void TerrainGenerator::genVolume(HeightMap& heightmap, Volume& volume, glm::vec3 chunkPos, glm::vec3 normedChunkPos)
{
  glm::mat4 rot0 = glm::mat4(
    cosf(0.77f), 0.f, -sinf(0.77f), 0.f,
    0.f, 1.f, 0.f, 0.f,
    sinf(0.77f), 0.f, cosf(0.77f), 0.f,
    0.f, 0.f, 0.f, 1.f
  );
  glm::mat4 rot1 = glm::mat4(
    1.f, 0.f, 0.f, 0.f,
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, cosf(-0.23f), -sinf(-0.23f),
    0.f, 0.f, sinf(-0.23f), cosf(-0.23f)
  );
  glm::mat4 rotM = rot0 * rot1;

  constexpr float voxelStep = invWorldDimension * invTechnicalChunkDim;
  constexpr float normedHalfChunkDim = static_cast<float>(HalfChunkDim) * invWorldDimensionInVoxels;
  uint32_t vox = 0;
  uint32_t iz, iy, ix;
  iz = 0, iy = 0, ix = 0;
  for (float z = normedChunkPos.z - normedHalfChunkDim; z < normedChunkPos.z + normedHalfChunkDim; z += voxelStep, ++iz)
  {
    iy = 0;
    for (float y = chunkPos.y - HalfChunkDim; y < chunkPos.y + HalfChunkDim; y++, ++iy)
    {
      ix = 0;
      for (float x = normedChunkPos.x - normedHalfChunkDim; x < normedChunkPos.x + normedHalfChunkDim; x += voxelStep, ++vox, ++ix)
      {
        float theta = x * 2.0f * static_cast<float>(PI);
        float phi = z * 2.0f * static_cast<float>(PI);
        float t_amp = 1.0f;
        float t_r = 16.f;

        uint32_t hm_p = iz * TrueChunkDim + ix; // Calculate heightmap position

        // Encourage ground plane around 0, shift groundplane by heightmap
        float terrain = -y + (heightmap[hm_p] * heightMapHeightInVoxels);

        for (int i = 0; i < 6; i++)
        {
          glm::vec4 p = glm::vec4(
            123.456
            , -432.912
            , -198.023
            , 543.298) + glm::vec4(
              t_r * std::cos(theta),
              t_r * std::sin(theta),
              t_r * std::cos(phi),
              t_r * std::sin(phi)
            );
          p = rotM * p;
          terrain += (t_amp * noise.GetSimplex(p.x, p.y, p.z, p.w, y))*64.f;
          t_amp *= 0.6f;
          t_r *= 2.4f;
        }
        terrain = glm::clamp(terrain, -1.f, 1.f); // Clamp density range to [-1,1]
        terrain = ((terrain*.5f) + .5f); // shift range to [0,1];
        volume[vox].density = static_cast<uint16_t>(terrain * std::numeric_limits<uint16_t>::max());
      }
    }
  }
}
