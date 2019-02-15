#include "dualmc.h"
#include "meshoptimizer.h"
#include "FastNoise.h"

#include "genNormals.hpp"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>

using tp = std::chrono::high_resolution_clock::time_point;
using hrclock = std::chrono::high_resolution_clock;

struct Volume {
  int32_t dimX, dimY, dimZ;
  int32_t bitDepth;
  std::vector<uint16_t> data;
} noiseVolume;

//template<typename vertType>
//void writeQuadOBJ(std::string const & fileName, bool const generateTris, const std::vector<vertType> vertices, const std::vector<unsigned int> quads) {
//  std::cout << "Writing OBJ file" << std::endl;
//
//  // open output file
//  std::ofstream file(fileName);
//  if (!file) {
//    std::cout << "Error opening output file" << std::endl;
//    return;
//  }
//
//  // write vertices
//  for (auto const & v : vertices) {
//    file << "v " << v.x << ' ' << v.y << ' ' << v.z << '\n';
//  }
//
//  // write quad indices
//  for (int32_t i = 0; i < quads.size(); i += 4)
//  {
//    file << "f " << quads[i] + 1 << ' ' << quads[i + 1] + 1 << ' ' << quads[i + 2] + 1 << ' ' << quads[i + 3] + 1  << ' ' << "\n";
//  }
//
//  file.close();
//}

int main()
{
  FastNoise noiseRigid(42);
  FastNoise noiseFbm(244224);
  noiseRigid.SetFractalType(FastNoise::FractalType::RigidMulti);
  noiseFbm.SetFractalType(FastNoise::FractalType::FBM);
  noiseFbm.SetFrequency(0.02f);

  constexpr unsigned int dim = 68;
  noiseVolume = {
    dim, dim, dim,
    16, 
    { {}, {} }
  };
  noiseVolume.data.resize(dim * dim * dim * 2);

  tp genNoiseStart = hrclock::now();
  int32_t p = 0;
  for (int32_t z = 0; z < noiseVolume.dimZ; ++z)
  {
    for (int32_t y = 0; y < noiseVolume.dimY; ++y)
    {
      for (int32_t x = 0; x < noiseVolume.dimX; ++x, ++p) // Increment p each step of inner-most loop
      {
        float v = noiseFbm.GetSimplexFractal(static_cast<float>(x) + 1234.5f, static_cast<float>(y) + 1234.5f, static_cast<float>(z) + 1234.5f, 8574.f, -1234.5f);
                //* noiseFbm.GetSimplexFractal(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
        //v = (v > 1.f) ? 1.f : ((v < -1.f) ? -1.f : v);

        noiseVolume.data[p] = static_cast<uint16_t>(v * std::numeric_limits<uint16_t>::max());
      }
    }
  }
  tp genNoiseEnd = hrclock::now();

  std::vector<dualmc::Vertex> vertices;
  std::vector<dualmc::Tri> triIndexes;
  //std::vector<dualmc::Quad> quadIndexes;

  tp genMeshStart = hrclock::now();
  dualmc::DualMC<uint16_t> builder;
  uint16_t iso = static_cast<uint16_t>(0.5 * std::numeric_limits<uint16_t>::max());
  builder.buildTris(noiseVolume.data.data(), noiseVolume.dimX, noiseVolume.dimY, noiseVolume.dimZ, iso, true, false, vertices, triIndexes);
  tp genMeshEnd = hrclock::now();
  //builder.build(noiseVolume.data.data(), noiseVolume.dimX, noiseVolume.dimY, noiseVolume.dimZ, iso, true, false, vertices, quadIndexes);

  tp meshOptStart = hrclock::now();
  // unpack triIndexes
  std::vector<unsigned int> unpackedIndices;
  for (auto t : triIndexes)
  {
    unpackedIndices.push_back(t.i0);
    unpackedIndices.push_back(t.i1);
    unpackedIndices.push_back(t.i2);
  }
  //for (auto t : quadIndexes)
  //{
  //  unpackedIndices.push_back(t.i0);
  //  unpackedIndices.push_back(t.i1);
  //  unpackedIndices.push_back(t.i2);
  //  unpackedIndices.push_back(t.i3);
  //}

  //writeQuadOBJ("out.quad.obj", false, vertices, unpackedIndices);


  // Mesh Optimiser magic
  size_t indexCount = unpackedIndices.size(), vertexCount;
  std::vector<dualmc::Vertex> verts;
  std::vector<unsigned int> indices;
  { // Temp remap vector
    std::vector<unsigned int> remap(indexCount);    
    vertexCount = meshopt_generateVertexRemap(&remap[0], &unpackedIndices[0], indexCount, &vertices[0], vertices.size(), sizeof(dualmc::Vertex));
    
    indices.resize(indexCount);
    meshopt_remapIndexBuffer(&indices[0], &unpackedIndices[0], indexCount, &remap[0]);

    verts.resize(vertexCount);
    meshopt_remapVertexBuffer(&verts[0], &vertices[0], vertexCount, sizeof(dualmc::Vertex), &remap[0]);
  }

  // See: https://github.com/zeux/meshoptimizer/blob/master/demo/main.cpp#L403
  // For multi-level LOD generation
  size_t targetIndexCount = static_cast<size_t>(indices.size() * 0.7f) / 3 * 3;
  float targetError = 1e-3f;
  indices.resize(meshopt_simplify(&indices[0], &indices[0], indices.size(), &verts[0].x, verts.size(), sizeof(dualmc::Vertex), targetIndexCount, targetError));

  meshopt_optimizeVertexCache(&indices[0], &indices[0], indices.size(), verts.size());

  meshopt_optimizeOverdraw(&indices[0], &indices[0], indices.size(), &verts[0].x, verts.size(), sizeof(dualmc::Vertex), 1.01f);  

  verts.resize(meshopt_optimizeVertexFetch(&verts[0], &indices[0], indices.size(), &verts[0], verts.size(), sizeof(dualmc::Vertex)));
  tp meshOptEnd = hrclock::now();

  struct Vertex
  {
    glm::vec3 pos;
    glm::vec3 normal;
  };

  tp genNormalsStart = hrclock::now();
  std::vector<Vertex> vertnorm;
  for (auto v : verts)
  {
    vertnorm.push_back({ {v.x, v.y, v.z}, {0.f, 0.f, 0.f} });
  }
  generateNormals(&vertnorm[0], vertnorm.size(), sizeof(Vertex), &indices[0], indices.size());
  tp genNormalsEnd = hrclock::now();
  
  // Write obj file
  std::cout << "Writing OBJ file" << std::endl;

  if (vertices.size() == 0 || triIndexes.size() == 0) {
    std::cout << "No ISO surface generated. Skipping OBJ generation." << std::endl;
    return 0;
  }  

  // open output file
  std::ofstream file("out.obj");
  if (!file) {
    std::cout << "Error opening output file" << std::endl;
    return 0;
  }

  std::cout << "Generating OBJ mesh with " << vertices.size() << " vertices and " << triIndexes.size() << " tris" << std::endl;

  // write vertices
  for (auto const & v : vertices) {
    file << "v " << v.x << ' ' << v.y << ' ' << v.z << '\n';
  }

  // write tri indices
  for (auto const & t : triIndexes) {
    file << "f " << (t.i0 + 1) << ' ' << (t.i1 + 1) << ' ' << (t.i2 + 1) << ' ' << "\n";
  }  

  file.close();

  // open output file
  std::ofstream file2("out.opt.obj");
  if (!file2) {
    std::cout << "Error opening output file" << std::endl;
    return 0;
  }

  std::cout << "Generating OBJ mesh with " << vertnorm.size() << " vertices and " << indices.size()/3 << " faces" << std::endl;

  // write vertices
  for (auto const & v : vertnorm) {
    file2 << "v " << v.pos.x << ' ' << v.pos.y << ' ' << v.pos.z << '\n';
  }

  // Write normals
  for (auto const & v : vertnorm) {
    file2 << "vn " << v.normal.x << ' ' << v.normal.y << ' ' << v.normal.z << '\n';
  }

  // write quad indices
  for (int32_t i = 0; i < indices.size(); i += 3)
  {
    file2 << "f " << indices[i]+1 << ' ' << indices[i+1]+1 << ' ' << indices[i+2]+1 << ' ' << "\n";
  }

  file2.close();

  float noiseVolumeTime = std::chrono::duration<float, std::chrono::milliseconds::period>(genNoiseEnd - genNoiseStart).count();
  float meshGenTime = std::chrono::duration<float, std::chrono::milliseconds::period>(genMeshEnd - genMeshStart).count();
  float meshOptTime = std::chrono::duration<float, std::chrono::milliseconds::period>(meshOptEnd - meshOptStart).count();
  float genNormalsTime = std::chrono::duration<float, std::chrono::milliseconds::period>(genNormalsEnd - genNormalsStart).count();

  std::cout << "Total Time To Generate Mesh: " << noiseVolumeTime + meshGenTime + meshOptTime + genNormalsTime << "ms\n";
  std::cout << "Noise Volume Fill Time: " << noiseVolumeTime << "ms\n";
  std::cout << "Mesh Gen Time: " << meshGenTime << "ms\n";
  std::cout << "Mesh Opt Time: " << meshOptTime << "ms\n";
  std::cout << "Normal Generation Time: " << genNormalsTime << "ms" << std::endl;

  return 0;
}