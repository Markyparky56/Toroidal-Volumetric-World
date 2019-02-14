#pragma once
#include "glm\glm.hpp"

// Assume vertex structure of float3 pxyz + float3 nxyz
void generateNormals(
    void * vertices, const unsigned int vertexCount, const int vertexSize
  , const unsigned int * indices, const unsigned int indexCount)
{
/*#define PX(i) (i*vertexSize + 0)
#define PY(i) (i*vertexSize + 1)
#define PZ(i) (i*vertexSize + 2)
#define NX(i) (i*vertexSize + 3)
#define NY(i) (i*vertexSize + 4)
#define NZ(i) (i*vertexSize + 5)*/

  const int stride = vertexSize / 4;

#define vert(i) (i*stride)
#define norm(i) (i*stride + 3)

  float * vertexArray = reinterpret_cast<float*>(vertices);

  // First, 0 all normals
  for (unsigned int i = 0; i < vertexCount; i++)
  {
    glm::vec3 * normal = reinterpret_cast<glm::vec3*>(&vertexArray[norm(i)]);
    *normal = { 0.f,0.f,0.f };
  }

  // Then, calculate all face normals and sum for each vertex
  for (unsigned int i = 0; i < indexCount; i+=3)
  {
    const int ia = indices[i+0];
    const int ib = indices[i+1];
    const int ic = indices[i+2];

    const glm::vec3 * va = reinterpret_cast<glm::vec3*>(&vertexArray[vert(ia)]);
    const glm::vec3 * vb = reinterpret_cast<glm::vec3*>(&vertexArray[vert(ib)]);
    const glm::vec3 * vc = reinterpret_cast<glm::vec3*>(&vertexArray[vert(ic)]);

    /*{ vertexArray[PX(ia)] - vertexArray[PX(ib)]
                         , vertexArray[PY(ia)] - vertexArray[PY(ib)]
                         , vertexArray[PZ(ia)] - vertexArray[PZ(ib)] };
    v1 =
    { vertexArray[PX(ic)] - vertexArray[PX(ib)]
                         , vertexArray[PY(ic)] - vertexArray[PY(ib)]
                         , vertexArray[PZ(ic)] - vertexArray[PZ(ib)] };*/
    const glm::vec3 v1 = *va - *vb;
    const glm::vec3 v2 = *vc - *vb;
    const glm::vec3 normal = glm::cross(v1, v2);

    glm::vec3 * na = reinterpret_cast<glm::vec3*>(&vertexArray[norm(ia)])
            , * nb = reinterpret_cast<glm::vec3*>(&vertexArray[norm(ib)])
            , * nc = reinterpret_cast<glm::vec3*>(&vertexArray[norm(ic)]);

    *na += normal;
    *nb += normal;
    *nc += normal;

    /*vertexArray[NX(ia)] += normal.x;
    vertexArray[NY(ia)] += normal.y;
    vertexArray[NZ(ia)] += normal.z;
    vertexArray[NX(ib)] += normal.x;
    vertexArray[NY(ib)] += normal.y;
    vertexArray[NZ(ib)] += normal.z;
    vertexArray[NX(ic)] += normal.x;
    vertexArray[NY(ic)] += normal.y;
    vertexArray[NZ(ic)] += normal.z;*/
  }

  for (unsigned int i = 0; i < vertexCount; i++)
  {
    glm::vec3 * normal = reinterpret_cast<glm::vec3*>(&vertexArray[norm(i)]);

    assert(!(normal->x == 0.f && normal->y == 0.f && normal->z == 0.f));

    *normal = glm::normalize(*normal);
  }
}
