#pragma once
static constexpr unsigned int TrueChunkDim = 36; // 32 + 2 voxel overlap along edges
static constexpr unsigned int TechnicalChunkDim = TrueChunkDim - 4;
static constexpr unsigned int HalfChunkDim = TrueChunkDim / 2;
static constexpr unsigned int ChunkSize = TrueChunkDim * TrueChunkDim * TrueChunkDim; // ChunkDim cubed
static constexpr unsigned int ChunkMapCacheSize = 32; // Arbitrary cache size 
static constexpr unsigned int chunkSpawnDistance = 9;
static constexpr unsigned int chunkViewDistance = 7;
static constexpr unsigned int maxChunks = chunkViewDistance * chunkViewDistance * chunkViewDistance;
static constexpr unsigned int WorldDimension = 64; // In chunks
static constexpr unsigned int WorldDimensionsInVoxels = WorldDimension * TechnicalChunkDim;
static constexpr float heightMapHeightInVoxels = 32.f * 32.f; 
static constexpr float invWorldDimension = 1.f / static_cast<float>(WorldDimension);
static constexpr float invWorldDimensionInVoxels = 1.f / static_cast<float>(WorldDimensionsInVoxels);
static constexpr float invTechnicalChunkDim = 1.f / TechnicalChunkDim;
static constexpr float chunkSpawnRadius = TechnicalChunkDim * chunkSpawnDistance;
static constexpr float chunkDespawnRadius = chunkSpawnRadius * 1.5f;
static constexpr double PI = 3.141592653589793238462643383279;
