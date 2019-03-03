#pragma once
#include <cstdint>

// Voxel Memory Structure
// Could be more complicated, i.e. materials and blending, data packing
// Demo just needs density for meshing
struct Voxel
{
  uint16_t density;
};
