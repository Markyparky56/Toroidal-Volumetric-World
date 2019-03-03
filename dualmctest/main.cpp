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
  constexpr double PI = 3.141592653589793238462643383279;

  constexpr unsigned int dim = 36;
  noiseVolume = {
    dim, dim, dim,
    16, 
    { {}, {} }
  };
  noiseVolume.data.resize(dim * dim * dim);

  tp genNoiseStart = hrclock::now();
  int32_t p = 0;
  glm::mat4 rot0 = glm::mat4(
    cosf(0.77f), 0.f, -cosf(0.77f), 0.f,
    0.f, 1.f, 0.f, 0.f,
    sinf(0.77f), 0.f, cos(0.77f), 0.f,
    0.f, 0.f, 0.f, 1.f
  );
  glm::mat4 rot1 = glm::mat4(
    1.f, 0.f, 0.f, 0.f,
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, cosf(-0.23f), -sinf(0.23f),
    0.f, 0.f, sinf(-0.23f), cosf(-0.23f)
  );
  //noiseFbm.SetFrequency(1.f);
  noiseFbm.SetSeed(3256);
  
  //for (int32_t z = -noiseVolume.dimZ/2; z < noiseVolume.dimZ/2; ++z)
  //{
  //  for (int32_t y = -noiseVolume.dimY/2 + 64; y < noiseVolume.dimY/2 + 64; ++y)
  //  {
  //    for (int32_t x = -noiseVolume.dimX/2 - 32; x < noiseVolume.dimX/2 - 32; ++x, ++p) // Increment p each step of inner-most loop
  //    {
  //      float theta = (static_cast<float>(x) / (32.f * 32.f)) * 2.0 * static_cast<float>(PI);
  //      float phi = (static_cast<float>(z) / (32.f * 32.f)) * 2.0 * static_cast<float>(PI);
  //      float Y = static_cast<float>(y);
  //      float h_amp = 1.0f;
  //      float h_r = 32.f;
  //      float height = 0.0f;
  //      float t_amp = 1.0f;
  //      float t_r = 32.f;
  //      float terrain = 0.0f;

  //      // height map
  //      for (int i = 0; i < 3; i++)
  //      {
  //        glm::vec4 p = glm::vec4(
  //          h_r * std::cos(theta),
  //          h_r * std::sin(theta),
  //          h_r * std::cos(phi),
  //          h_r * std::sin(phi)
  //        );
  //       // p *= rot0;
  //        //p *= rot1;
  //        height += h_amp * noiseFbm.GetSimplex(p.x, p.y, p.z, p.w);
  //        h_amp *= 0.65f;
  //        h_r *= 2.0f;
  //      }
  //      // terrain 
  //      for (int i = 0; i < 4; i++)
  //      {
  //        glm::vec4 p = glm::vec4(
  //            123.456
  //          , -432.912
  //          , -198.023
  //          , 543.298) + glm::vec4(
  //          t_r * std::cos(theta),
  //          t_r * std::sin(theta),
  //          t_r * std::cos(phi),
  //          t_r * std::sin(phi)
  //        );
  //        p = rot0 * p;
  //        p = rot1 * p;
  //        terrain += t_amp * noiseFbm.GetSimplex(p.x, p.y, p.z, p.w, Y, 0.f)*.5f;
  //        t_amp *= 0.65f;
  //        t_r *= 2.2f;
  //      }
  //      //terrain = terrain * ((height*.5f)+.5f);
  //      //float height = noiseFbm.GetSimplexFractal(px, py, pz, pw);
  //      //float noise = noiseFbm.GetSimplexFractal(px, py, pz, pw, pv) + height;
  //      //float v = noiseFbm.GetSimplexFractal(static_cast<float>(x) + 4234.5f, static_cast<float>(y) + 1234.5f, static_cast<float>(z) + 1234.5f, 8574.f, -1234.5f);
  //              //* noiseFbm.GetSimplexFractal(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
  //      //v = (v > 1.f) ? 1.f : ((v < -1.f) ? -1.f : v);
  //      //terrain = noiseFbm.GetSimplex(x, y, z, 1234.5678f, 9876.5432f);
  //      noiseVolume.data[p] = static_cast<uint16_t>(terrain * std::numeric_limits<uint16_t>::max());
  //    }
  //  }
  //}

  constexpr float voxelStep = (1.f / 64.f)*(1.f / 32.f);
  constexpr float normedHalfChunkDim = 18.f * (1.f / (64.f*32.f));
  for (float z = -normedHalfChunkDim; z < normedHalfChunkDim; z+=voxelStep)
  {
    for (float y = -18.f; y < 18.f; y += 1.f)
    {
      for (float x = -normedHalfChunkDim; x < normedHalfChunkDim; x += voxelStep, p++)
      {
        float theta = x * 2.0 * static_cast<float>(PI);
        float phi = z * 2.0 * static_cast<float>(PI);
        float t_amp = 1.0f;
        float t_r = 32.f;
        float terrain = 0.0f;
        for (int i = 0; i < 4; i++)
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
          p = rot0 * p;
          p = rot1 * p;
          terrain += t_amp * noiseFbm.GetSimplex(p.x, p.y, p.z, p.w, y, 0.f)*.5f;
          t_amp *= 0.65f;
          t_r *= 2.2f;
        }
        noiseVolume.data[p] = static_cast<uint16_t>(terrain * std::numeric_limits<uint16_t>::max());
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
  if (unpackedIndices.size() > 0)
  {
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
    size_t targetIndexCount = static_cast<size_t>(indices.size() * 1.0f) / 3 * 3;
    float targetError = 1e-3f;
    indices.resize(meshopt_simplify(&indices[0], &indices[0], indices.size(), &verts[0].x, verts.size(), sizeof(dualmc::Vertex), targetIndexCount, targetError));

    meshopt_optimizeVertexCache(&indices[0], &indices[0], indices.size(), verts.size());

    meshopt_optimizeOverdraw(&indices[0], &indices[0], indices.size(), &verts[0].x, verts.size(), sizeof(dualmc::Vertex), 1.01f);

    verts.resize(meshopt_optimizeVertexFetch(&verts[0], &indices[0], indices.size(), &verts[0], verts.size(), sizeof(dualmc::Vertex)));
  }
  tp meshOptEnd = hrclock::now();

  struct Vertex
  {
    glm::vec3 pos;
    glm::vec3 normal;
  };

  tp genNormalsStart = hrclock::now();
  std::vector<Vertex> vertnorm;
  if (verts.size() > 0)
  {
    for (auto v : verts)
    {
      vertnorm.push_back({ {v.x, v.y, v.z}, {0.f, 0.f, 0.f} });
    }
    generateNormals(&vertnorm[0], vertnorm.size(), sizeof(Vertex), &indices[0], indices.size());
  }
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