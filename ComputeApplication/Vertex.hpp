#pragma once
#include "glm/glm.hpp"

// Simple, dumb vertex struct, can be extended to include additional data like texture uvs, 
// materials, generic colours.
struct Vertex
{
  glm::vec3 pos;
  glm::vec3 normal;
};
