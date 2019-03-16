#include "FrustumClass.hpp"

void Frustum::Construct(float screenDepth, glm::mat4 proj, glm::mat4 view)
{
  float zMin, r;
  glm::mat4 frustumMatrix;

  // Calculate the minimum Z distance
  zMin = -proj[4][3] / proj[3][3];
  r = screenDepth / (screenDepth - zMin);
  proj[3][3] = r;
  proj[4][3] = -r * zMin;

  // Create the frustum matrix from the view matrix and updated projection matrix
  frustumMatrix = view * proj;

  // Calculate near plane
  planes[0].x = frustumMatrix[1][4] + frustumMatrix[1][3];
  planes[0].y = frustumMatrix[2][4] + frustumMatrix[2][3];
  planes[0].z = frustumMatrix[3][4] + frustumMatrix[3][3];
  planes[0].w = frustumMatrix[4][4] + frustumMatrix[4][3];
  planes[0] = glm::normalize(planes[0]);

  // Calculate far plane
  planes[1].x = frustumMatrix[1][4] - frustumMatrix[1][3];
  planes[1].y = frustumMatrix[2][4] - frustumMatrix[2][3];
  planes[1].z = frustumMatrix[3][4] - frustumMatrix[3][3];
  planes[1].w = frustumMatrix[4][4] - frustumMatrix[4][3];
  planes[1] = glm::normalize(planes[1]);

  // Calculate left plane
  planes[2].x = frustumMatrix[1][4] + frustumMatrix[1][1];
  planes[2].y = frustumMatrix[2][4] + frustumMatrix[2][1];
  planes[2].z = frustumMatrix[3][4] + frustumMatrix[3][1];
  planes[2].w = frustumMatrix[4][4] + frustumMatrix[4][1];
  planes[2] = glm::normalize(planes[2]);

  // Calculate right plane
  planes[3].x = frustumMatrix[1][4] - frustumMatrix[1][1];
  planes[3].y = frustumMatrix[2][4] - frustumMatrix[2][1];
  planes[3].z = frustumMatrix[3][4] - frustumMatrix[3][1];
  planes[3].w = frustumMatrix[4][4] - frustumMatrix[4][1];
  planes[3] = glm::normalize(planes[3]);

  // Calculate top plane
  planes[4].x = frustumMatrix[1][4] - frustumMatrix[1][2];
  planes[4].y = frustumMatrix[2][4] - frustumMatrix[2][2];
  planes[4].z = frustumMatrix[3][4] - frustumMatrix[3][2];
  planes[4].w = frustumMatrix[4][4] - frustumMatrix[4][2];
  planes[4] = glm::normalize(planes[4]);

  // Calculate top plane
  planes[5].x = frustumMatrix[1][4] + frustumMatrix[1][2];
  planes[5].y = frustumMatrix[2][4] + frustumMatrix[2][2];
  planes[5].z = frustumMatrix[3][4] + frustumMatrix[3][2];
  planes[5].w = frustumMatrix[4][4] + frustumMatrix[4][2];
  planes[5] = glm::normalize(planes[5]);
}

bool Frustum::CheckPoint(glm::vec3 p)
{
  for (int i = 0; i < 6; ++i)
  {
    // Compute the dot product between each plane and the point
    if (PlaneDotCoord(planes[i], p) < 0.f)
    {
      return false;
    }
  }
  return true;
}

bool Frustum::CheckCbue(glm::vec3 p, float halfDim)
{
  for (int i = 0; i < 6; ++i)
  {
    if (PlaneDotCoord(planes[i], glm::vec3(p.x - halfDim, p.y - halfDim, p.z - halfDim)) >= 0.f)
    {
      continue;
    }

    if (PlaneDotCoord(planes[i], glm::vec3(p.x + halfDim, p.y - halfDim, p.z - halfDim)) >= 0.f)
    {
      continue;
    }

    if (PlaneDotCoord(planes[i], glm::vec3(p.x - halfDim, p.y + halfDim, p.z - halfDim)) >= 0.f)
    {
      continue;
    }

    if (PlaneDotCoord(planes[i], glm::vec3(p.x + halfDim, p.y + halfDim, p.z - halfDim)) >= 0.f)
    {
      continue;
    }

    if (PlaneDotCoord(planes[i], glm::vec3(p.x - halfDim, p.y - halfDim, p.z + halfDim)) >= 0.f)
    {
      continue;
    }

    if (PlaneDotCoord(planes[i], glm::vec3(p.x + halfDim, p.y - halfDim, p.z + halfDim)) >= 0.f)
    {
      continue;
    }

    if (PlaneDotCoord(planes[i], glm::vec3(p.x - halfDim, p.y + halfDim, p.z + halfDim)) >= 0.f)
    {
      continue;
    }

    if (PlaneDotCoord(planes[i], glm::vec3(p.x + halfDim, p.y + halfDim, p.z + halfDim)) >= 0.f)
    {
      continue;
    }

    return false;
  }
  return true;
}

bool Frustum::CheckSphere(glm::vec3 p, float r)
{
  for (int i = 0; i < 6; ++i)
  {
    if (PlaneDotCoord(planes[i], p) < -r)
    {
      return false;
    }
  }

  return true;
}

bool Frustum::CheckRectangle(glm::vec3 p, glm::vec3 halfDims)
{
  for (int i = 0; i < 6; ++i)
  {
    if (PlaneDotCoord(planes[i], glm::vec3(p.x - halfDims.x, p.y - halfDims.y, p.z - halfDims.z)) >= 0.f)
    {
      continue;
    }

    if (PlaneDotCoord(planes[i], glm::vec3(p.x + halfDims.x, p.y - halfDims.y, p.z - halfDims.z)) >= 0.f)
    {
      continue;
    }

    if (PlaneDotCoord(planes[i], glm::vec3(p.x - halfDims.x, p.y + halfDims.y, p.z - halfDims.z)) >= 0.f)
    {
      continue;
    }

    if (PlaneDotCoord(planes[i], glm::vec3(p.x + halfDims.x, p.y + halfDims.y, p.z - halfDims.z)) >= 0.f)
    {
      continue;
    }

    if (PlaneDotCoord(planes[i], glm::vec3(p.x - halfDims.x, p.y - halfDims.y, p.z + halfDims.z)) >= 0.f)
    {
      continue;
    }

    if (PlaneDotCoord(planes[i], glm::vec3(p.x + halfDims.x, p.y - halfDims.y, p.z + halfDims.z)) >= 0.f)
    {
      continue;
    }

    if (PlaneDotCoord(planes[i], glm::vec3(p.x - halfDims.x, p.y + halfDims.y, p.z + halfDims.z)) >= 0.f)
    {
      continue;
    }

    if (PlaneDotCoord(planes[i], glm::vec3(p.x + halfDims.x, p.y + halfDims.y, p.z + halfDims.z)) >= 0.f)
    {
      continue;
    }

    return false;
  }
  return true;
}
