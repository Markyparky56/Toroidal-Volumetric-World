#pragma once
static constexpr unsigned int TrueChunkDim = 36; // 32 + 2 voxel overlap along edges
static constexpr unsigned int TechnicalChunkDim = TrueChunkDim - 4;
static constexpr unsigned int HalfChunkDim = TrueChunkDim / 2;
static constexpr unsigned int ChunkSize = TrueChunkDim * TrueChunkDim * TrueChunkDim; // ChunkDim cubed
static constexpr unsigned int ChunkMapCacheSize = 32; // Arbitrary cache size 
static constexpr unsigned int chunkSpawnDistance = 9;
static constexpr unsigned int chunkViewDistance = 7;
static constexpr unsigned int maxChunks = chunkViewDistance * chunkViewDistance * chunkViewDistance;
static constexpr float invTechnicalChunkDim = 1.f / TechnicalChunkDim;
static constexpr float chunkSpawnRadius = TechnicalChunkDim * chunkSpawnDistance;
static constexpr float chunkDespawnRadius = chunkSpawnRadius * 1.5f;
