#pragma once
#include "common.hpp"
#include <glm/vec3.hpp>
#include <glm/common.hpp>
#include <cmath>

inline void WrapCoordinates(glm::vec3 & p)
{
  constexpr float worldDim = static_cast<float>(WorldDimensionsInVoxels);
  glm::vec3 tmp = glm::abs(p);
  if (p.x < 0.f)
  {
    p.x = worldDim - tmp.x;
  }
  else if (p.x >= worldDim)
  {
    p.x = tmp.x - worldDim;
  }
  /*if (p.y < 0.f)
  {
    p.y = worldDim - tmp.y;
  }
  else if (p.y >= worldDim)
  {
    p.y = tmp.y - worldDim;
  }*/
  if (p.z < 0.f)
  {
    p.z = worldDim - tmp.z;
  }
  else if (p.z >= worldDim)
  {
    p.z = tmp.z - worldDim;
  }
}

// Clever linear distance calc on a toroidal plane from:
// https://stackoverflow.com/questions/4940636/how-can-i-calculate-the-distance-between-two-points-in-cartesian-space-while-res
// Does not wrap on the y-axis, only x and z
inline float sqrdToroidalDistance(glm::vec3 const p0, glm::vec3 const p1)
{
  glm::vec3 rawdelta = glm::abs(p1 - p0);

  glm::vec3 delta = {
    (rawdelta.x < (WorldDimensionsInVoxelsf / 2.f)) ? rawdelta.x : WorldDimensionsInVoxelsf - rawdelta.x,
    rawdelta.y,
    (rawdelta.z < (WorldDimensionsInVoxelsf / 2.f)) ? rawdelta.z : WorldDimensionsInVoxelsf - rawdelta.z,
  };

  return glm::dot(delta, delta);
}

inline float sqrdDistance(glm::vec3 const p0, glm::vec3 const p1)
{
  glm::vec3 delta = p1 - p0;

  return glm::dot(delta, delta);
}

inline void CorrectChunkPosition(glm::vec3 const playerPos, glm::vec3 & chunkPos)
{
  constexpr float halfWorldDim = WorldDimensionsInVoxelsf / 2.f;
  glm::vec3 deltas = glm::abs(playerPos - chunkPos);
  if (deltas.x > halfWorldDim)
  {
    if (playerPos.x > halfWorldDim)
    {
      chunkPos.x = WorldDimensionsInVoxelsf - chunkPos.x;
    }
    else
    {
      chunkPos.x = WorldDimensionsInVoxelsf + chunkPos.x;
    }
  }
  if (deltas.z > halfWorldDim)
  {
    if (playerPos.z < halfWorldDim)
    {
      chunkPos.z = WorldDimensionsInVoxelsf - chunkPos.z;
    }
    else
    {
      chunkPos.z = WorldDimensionsInVoxelsf + chunkPos.z;
    }
  }
}
