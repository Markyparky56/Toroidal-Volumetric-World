#pragma once
constexpr unsigned int ChunkDim = 36; // 32 + 2 voxel overlap along edges
constexpr unsigned int HalfChunkDim = ChunkDim / 2;
constexpr unsigned int ChunkSize = ChunkDim * ChunkDim*ChunkDim; // ChunkDim cubed
constexpr unsigned int ChunkMapCacheSize = 32; // Arbitrary cache size 