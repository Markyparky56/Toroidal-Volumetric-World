#pragma once
#include "dualmc.h"
#include "voxel.hpp"
#include <type_traits>
#include <typeinfo>
#include "Vertex.hpp"

//using dualmc::Vertex;
using dualmc::VertexComponentsType;
using dualmc::TriIndexType;
using dualmc::Tri;

class DualMCVoxel : public dualmc::DualMC<Voxel>
{
  typedef uint16_t VolumeDataType;

public:
  void buildTris(
    Voxel const * data,
    int32_t const dimX, int32_t const dimY, int32_t const dimZ,
    VolumeDataType const iso,
    bool const generateManifold,
    bool const generateSoup,
    std::vector<Vertex> & vertices,
    std::vector<TriIndexType> & tris
  )
  {
    // set members
    this->dims[0] = dimX;
    this->dims[1] = dimY;
    this->dims[2] = dimZ;
    this->data = data;
    this->generateManifold = generateManifold;

    // clear vertices and quad indices
    vertices.clear();
    tris.clear();

    // generate quad soup or shared vertices quad list
    if (generateSoup) {
      //buildTriSoup(iso, vertices, tris);
    }
    else {
      buildSharedVerticesTris(iso, vertices, tris);
    }
  }

protected:
  int getDualPointCode(int32_t const cx, int32_t const cy, int32_t const cz, VolumeDataType const iso, DMCEdgeCode const edge) const {
    int cubeCode = getCellCode(cx, cy, cz, iso);

    // is manifold dual marching cubes desired?
    if (generateManifold) {
      // The Manifold Dual Marching Cubes approach from Rephael Wenger as described in
      // chapter 3.3.5 of his book "Isosurfaces: Geometry, Topology, and Algorithms"
      // is implemente here.
      // If a problematic C16 or C19 configuration shares the ambiguous face 
      // with another C16 or C19 configuration we simply invert the cube code
      // before looking up dual points. Doing this for these pairs ensures
      // manifold meshes.
      // But this removes the dualism to marching cubes.

      // check if we have a potentially problematic configuration
      uint8_t const direction = problematicConfigs[uint8_t(cubeCode)];
      // If the direction code is in {0,...,5} we have a C16 or C19 configuration.
      if (direction != 255) {
        // We have to check the neighboring cube, which shares the ambiguous
        // face. For this we decode the direction. This could also be done
        // with another lookup table.
        // copy current cube coordinates into an array.
        int32_t neighborCoords[] = { cx,cy,cz };
        // get the dimension of the non-zero coordinate axis
        unsigned int const component = direction >> 1;
        // get the sign of the direction
        int32_t delta = (direction & 1) == 1 ? 1 : -1;
        // modify the correspong cube coordinate
        neighborCoords[component] += delta;
        // have we left the volume in this direction?
        if (neighborCoords[component] >= 0 && neighborCoords[component] < (dims[component] - 1)) {
          // get the cube configuration of the relevant neighbor
          int neighborCubeCode = getCellCode(neighborCoords[0], neighborCoords[1], neighborCoords[2], iso);
          // Look up the neighbor configuration ambiguous face direction.
          // If the direction is valid we have a C16 or C19 neighbor.
          // As C16 and C19 have exactly one ambiguous face this face is
          // guaranteed to be shared for the pair.
          if (problematicConfigs[uint8_t(neighborCubeCode)] != 255) {
            // replace the cube configuration with its inverse.
            cubeCode ^= 0xff;
          }
        }
      }
    }
    for (int i = 0; i < 4; ++i)
      if (dualPointsList[cubeCode][i] & edge) {
        return dualPointsList[cubeCode][i];
      }
    return 0;
  }

  TriIndexType getSharedDualPointIndex(
      int32_t const cx, int32_t const cy, int32_t const cz,
      VolumeDataType const iso, DMCEdgeCode const edge,
      std::vector<Vertex> & vertices
    ) {
    // create a key for the dual point from its linearized cell ID and point code
    DualPointKey key;
    key.linearizedCellID = gA(cx, cy, cz);
    key.pointCode = getDualPointCode(cx, cy, cz, iso, edge);

    // have we already computed the dual point?
    auto iterator = pointToIndex.find(key);
    if (iterator != pointToIndex.end()) {
      // just return the dual point index
      return iterator->second;
    }
    else {
      // create new vertex and vertex id
      TriIndexType newVertexId = static_cast<TriIndexType>(vertices.size());
      vertices.emplace_back();
      calculateDualPoint(cx, cy, cz, iso, key.pointCode, vertices.back());
      // insert vertex ID into map and also return it
      pointToIndex[key] = newVertexId;
      return newVertexId;
    }
  }

  int getCellCode(int32_t const cx, int32_t const cy, int32_t const cz, VolumeDataType const iso) const
  {
    // determine for each cube corner if it is outside or inside
    int code = 0;
    if (data[gA(cx, cy, cz)].density >= iso)
      code |= 1;
    if (data[gA(cx + 1, cy, cz)].density >= iso)
      code |= 2;
    if (data[gA(cx, cy + 1, cz)].density >= iso)
      code |= 4;
    if (data[gA(cx + 1, cy + 1, cz)].density >= iso)
      code |= 8;
    if (data[gA(cx, cy, cz + 1)].density >= iso)
      code |= 16;
    if (data[gA(cx + 1, cy, cz + 1)].density >= iso)
      code |= 32;
    if (data[gA(cx, cy + 1, cz + 1)].density >= iso)
      code |= 64;
    if (data[gA(cx + 1, cy + 1, cz + 1)].density >= iso)
      code |= 128;
    return code;
  }

  void buildSharedVerticesTris(
      VolumeDataType const iso,
      std::vector<Vertex> & vertices,
      std::vector<TriIndexType> & tris
    ) 
  {
    int32_t const reducedX = dims[0] - 2;
    int32_t const reducedY = dims[1] - 2;
    int32_t const reducedZ = dims[2] - 2;

    TriIndexType i0, i1, i2, i3;

    pointToIndex.clear();

    // iterate voxels
    for (int32_t z = 0; z < reducedZ; ++z)
      for (int32_t y = 0; y < reducedY; ++y)
        for (int32_t x = 0; x < reducedX; ++x) {
          // construct quads for x edge
          if (z > 0 && y > 0) {
            bool const entering = data[gA(x, y, z)].density < iso && data[gA(x + 1, y, z)].density >= iso;
            bool const exiting = data[gA(x, y, z)].density >= iso && data[gA(x + 1, y, z)].density < iso;
            if (entering || exiting) {
              // get quad
              i0 = getSharedDualPointIndex(x, y, z, iso, EDGE0, vertices);
              i1 = getSharedDualPointIndex(x, y, z - 1, iso, EDGE2, vertices);
              i2 = getSharedDualPointIndex(x, y - 1, z - 1, iso, EDGE6, vertices);
              i3 = getSharedDualPointIndex(x, y - 1, z, iso, EDGE4, vertices);

              if (entering) {
                //tris.emplace_back(i0, i1, i2);
                //tris.emplace_back(i2, i3, i0);

                tris.insert(tris.end(), { i0, i1, i2, i2, i3, i0 });
              }
              else {
                //tris.emplace_back(i2, i1, i0);
                //tris.emplace_back(i0, i3, i2);

                tris.insert(tris.end(), { i2, i1, i0, i0, i3, i2 });
              }
            }
          }

          // construct quads for y edge
          if (z > 0 && x > 0) {
            bool const entering = data[gA(x, y, z)].density < iso && data[gA(x, y + 1, z)].density >= iso;
            bool const exiting = data[gA(x, y, z)].density >= iso && data[gA(x, y + 1, z)].density < iso;
            if (entering || exiting) {
              // generate quad
              i0 = getSharedDualPointIndex(x, y, z, iso, EDGE8, vertices);
              i1 = getSharedDualPointIndex(x, y, z - 1, iso, EDGE11, vertices);
              i2 = getSharedDualPointIndex(x - 1, y, z - 1, iso, EDGE10, vertices);
              i3 = getSharedDualPointIndex(x - 1, y, z, iso, EDGE9, vertices);

              if (exiting) {
                //tris.emplace_back(i0, i1, i2);
                //tris.emplace_back(i2, i3, i0);

                tris.insert(tris.end(), { i0, i1, i2, i2, i3, i0 });
              }
              else {
                //tris.emplace_back(i2, i1, i0);
                //tris.emplace_back(i0, i3, i2);

                tris.insert(tris.end(), { i2, i1, i0, i0, i3, i2 });
              }
            }
          }

          // construct quads for z edge
          if (x > 0 && y > 0) {
            bool const entering = data[gA(x, y, z)].density < iso && data[gA(x, y, z + 1)].density >= iso;
            bool const exiting = data[gA(x, y, z)].density >= iso && data[gA(x, y, z + 1)].density < iso;
            if (entering || exiting) {
              // generate quad
              i0 = getSharedDualPointIndex(x, y, z, iso, EDGE3, vertices);
              i1 = getSharedDualPointIndex(x - 1, y, z, iso, EDGE1, vertices);
              i2 = getSharedDualPointIndex(x - 1, y - 1, z, iso, EDGE5, vertices);
              i3 = getSharedDualPointIndex(x, y - 1, z, iso, EDGE7, vertices);

              if (exiting) {
                //tris.emplace_back(i0, i1, i2);
                //tris.emplace_back(i2, i3, i0);

                tris.insert(tris.end(), { i0, i1, i2, i2, i3, i0 });
              }
              else {
                //tris.emplace_back(i2, i1, i0);
                //tris.emplace_back(i0, i3, i2);

                tris.insert(tris.end(), { i2, i1, i0, i0, i3, i2 });
              }
            }
          }
        }
  }

  void calculateDualPoint(int32_t const cx, int32_t const cy, int32_t const cz,
    VolumeDataType const iso, int const pointCode, Vertex &v) const
  {
    // initialize the point with lower voxel coordinates
    v.pos.x = static_cast<VertexComponentsType>(cx);
    v.pos.y = static_cast<VertexComponentsType>(cy);
    v.pos.z = static_cast<VertexComponentsType>(cz);

    // compute the dual point as the mean of the face vertices belonging to the
    // original marching cubes face
    Vertex p;
    p.pos.x = 0;
    p.pos.y = 0;
    p.pos.z = 0;
    int points = 0;

    // sum edge intersection vertices using the point code
    if (pointCode & EDGE0) {
      p.pos.x += ((float)iso - (float)data[gA(cx, cy, cz)].density) / ((float)data[gA(cx + 1, cy, cz)].density - (float)data[gA(cx, cy, cz)].density);
      points++;
    }

    if (pointCode & EDGE1) {
      p.pos.x += 1.0f;
      p.pos.z += ((float)iso - (float)data[gA(cx + 1, cy, cz)].density) / ((float)data[gA(cx + 1, cy, cz + 1)].density - (float)data[gA(cx + 1, cy, cz)].density);
      points++;
    }

    if (pointCode & EDGE2) {
      p.pos.x += ((float)iso - (float)data[gA(cx, cy, cz + 1)].density) / ((float)data[gA(cx + 1, cy, cz + 1)].density - (float)data[gA(cx, cy, cz + 1)].density);
      p.pos.z += 1.0f;
      points++;
    }

    if (pointCode & EDGE3) {
      p.pos.z += ((float)iso - (float)data[gA(cx, cy, cz)].density) / ((float)data[gA(cx, cy, cz + 1)].density - (float)data[gA(cx, cy, cz)].density);
      points++;
    }

    if (pointCode & EDGE4) {
      p.pos.x += ((float)iso - (float)data[gA(cx, cy + 1, cz)].density) / ((float)data[gA(cx + 1, cy + 1, cz)].density - (float)data[gA(cx, cy + 1, cz)].density);
      p.pos.y += 1.0f;
      points++;
    }

    if (pointCode & EDGE5) {
      p.pos.x += 1.0f;
      p.pos.z += ((float)iso - (float)data[gA(cx + 1, cy + 1, cz)].density) / ((float)data[gA(cx + 1, cy + 1, cz + 1)].density - (float)data[gA(cx + 1, cy + 1, cz)].density);
      p.pos.y += 1.0f;
      points++;
    }

    if (pointCode & EDGE6) {
      p.pos.x += ((float)iso - (float)data[gA(cx, cy + 1, cz + 1)].density) / ((float)data[gA(cx + 1, cy + 1, cz + 1)].density - (float)data[gA(cx, cy + 1, cz + 1)].density);
      p.pos.z += 1.0f;
      p.pos.y += 1.0f;
      points++;
    }

    if (pointCode & EDGE7) {
      p.pos.z += ((float)iso - (float)data[gA(cx, cy + 1, cz)].density) / ((float)data[gA(cx, cy + 1, cz + 1)].density - (float)data[gA(cx, cy + 1, cz)].density);
      p.pos.y += 1.0f;
      points++;
    }

    if (pointCode & EDGE8) {
      p.pos.y += ((float)iso - (float)data[gA(cx, cy, cz)].density) / ((float)data[gA(cx, cy + 1, cz)].density - (float)data[gA(cx, cy, cz)].density);
      points++;
    }

    if (pointCode & EDGE9) {
      p.pos.x += 1.0f;
      p.pos.y += ((float)iso - (float)data[gA(cx + 1, cy, cz)].density) / ((float)data[gA(cx + 1, cy + 1, cz)].density - (float)data[gA(cx + 1, cy, cz)].density);
      points++;
    }

    if (pointCode & EDGE10) {
      p.pos.x += 1.0f;
      p.pos.y += ((float)iso - (float)data[gA(cx + 1, cy, cz + 1)].density) / ((float)data[gA(cx + 1, cy + 1, cz + 1)].density - (float)data[gA(cx + 1, cy, cz + 1)].density);
      p.pos.z += 1.0f;
      points++;
    }

    if (pointCode & EDGE11) {
      p.pos.z += 1.0f;
      p.pos.y += ((float)iso - (float)data[gA(cx, cy, cz + 1)].density) / ((float)data[gA(cx, cy + 1, cz + 1)].density - (float)data[gA(cx, cy, cz + 1)].density);
      points++;
    }

    // divide by number of accumulated points
    float invPoints = 1.0f / (float)points;
    p.pos.x *= invPoints;
    p.pos.y *= invPoints;
    p.pos.z *= invPoints;

    // offset point by voxel coordinates
    v.pos.x += p.pos.x;
    v.pos.y += p.pos.y;
    v.pos.z += p.pos.z;
  }
  
};
