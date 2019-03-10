#pragma once
#include "glm/vec4.hpp"
#include "glm/vec3.hpp"
#include "glm/vec2.hpp"

// Simple, dumb vertex struct, can be extended to include additional data like texture uvs, 
// materials, generic colours.
struct Vertex
{
  glm::vec3 pos;
  glm::vec3 normal;
};
