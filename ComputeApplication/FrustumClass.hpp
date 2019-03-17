#pragma once
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"

// Based on: http://www.rastertek.com/dx11tut16.html

class Frustum
{
public:
  void Construct(float screenDepth, glm::mat4 proj, glm::mat4 view);

  bool CheckPoint(glm::vec3 p);
  bool CheckCube(glm::vec3 p, float halfDim);
  bool CheckSphere(glm::vec3 p, float r);
  bool CheckRectangle(glm::vec3 p, glm::vec3 halfDims);

protected:
  float PlaneDotCoord(glm::vec4 a, glm::vec3 b)
  {
    return glm::dot(a, glm::vec4(b, 1.f));
  }
  glm::vec4 planes[6];
};
