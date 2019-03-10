#pragma once
#include "AppBase.hpp"
#include "GraphicsPipeline.hpp"
#include "taskflow\taskflow.hpp"
#include "vk_mem_alloc.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_vulkan.h"
#include "common.hpp"
#include "ChunkManager.hpp"
#include "Camera.hpp"
#include "TerrainGenerator.hpp"
#include "SurfaceExtractor.hpp"

#include <stack>
#include <array>

class ComputeApp : public AppBase
{
public:
  bool Initialise(VulkanInterface::WindowParameters windowParameters) override;
  bool Update() override;
  bool Resize() override;

private:
  bool setupVulkanAndCreateSwapchain(VulkanInterface::WindowParameters windowParameters);
  bool setupTaskflow();
  bool initialiseVulkanMemoryAllocator();
  bool initImGui(HWND hwnd);
  bool setupCommandPoolAndBuffers();
  bool setupRenderPass();
  bool setupGraphicsPipeline();
  bool setupChunkManager();
  bool setupTerrainGenerator();
  bool setupSurfaceExtractor();
  bool setupECS();

  void Shutdown() override;
  void shutdownVulkanMemoryAllocator();
  void shutdownChunkManager();
  void shutdownGraphicsPipeline();
  void shutdownImGui();
  void cleanupVulkan();

  // App logic
  void updateUser();
  void checkForNewChunks();
  void getChunkRenderList();
  void recordChunkDrawCalls();
  bool drawChunks();

  bool chunkIsWithinFrustum();
  void loadFromChunkCache(EntityHandle handle);
  void generateChunk(EntityHandle handle);
  VkCommandBuffer drawChunkOp(EntityHandle chunk, VkCommandBufferInheritanceInfo * const inheritanceInfo);

  // cpp-taskflow taskflows and shared executor
  std::unique_ptr<tf::Taskflow> graphicsTaskflow, computeTaskflow, systemTaskflow;
  std::shared_ptr<tf::Taskflow::Executor> tfExecutor;

  // Vulkan Memory Allocator
  VmaAllocator allocator;

  // ImGui gets a dedicated descriptor pool with everything
  VkDescriptorPool imGuiDescriptorPool;

  //std::unique_ptr<GraphicsPipeline> graphicsPipeline; // This could feasibly be a vector if we ever want a more complex render setup
                                     // But having explicitly named pipelines would probably be better


  VkRenderPass renderPass;
  std::vector<VulkanInterface::FrameResources> frameResources;
  VkCommandPool transferCommandPool;
  std::vector<VkCommandBuffer> transferCommandBuffers;
  std::stack<VkCommandBuffer*> transferCommandBuffersStack;
  std::mutex transferCBufStackMutex;
  VkCommandPool graphicsCommandPool;
  std::vector<VkCommandBuffer> frameCommandBuffers;
  std::vector<VkCommandBuffer> chunkCommandBuffersVec;
  std::array<std::stack<VkCommandBuffer*>, numFrames> chunkCommandBufferStacks;
  std::mutex chunkCBufStackMutex;
  std::vector<VkCommandBuffer> recordedChunkDrawCalls;
  //uint32_t graphicsQueueFamily, presentQueueFamily, computeQueueFamily;
  VkQueue graphicsQueue, presentQueue;
  std::vector<VkQueue> computeQueues;

  VkDescriptorSetLayout descriptorSetLayout;
  VkDescriptorPool descriptorPool;
  std::vector<VkDescriptorSet> descriptorSets;
  VkPipeline graphicsPipeline;
  VkPipelineLayout graphicsPipelineLayout;
  VkBuffer uniformBuffer;
  VkDeviceMemory uniformBufferMemory;

  std::unique_ptr<ChunkManager> chunkManager;
  std::unique_ptr<entt::registry<>> registry;
  std::unique_ptr<TerrainGenerator> terrainGen;
  std::unique_ptr<SurfaceExtractor> surfaceExtractor;

  std::vector<std::pair<EntityHandle, ChunkManager::ChunkStatus>> chunkSpawnList;
  std::vector<EntityHandle> chunkRenderList;

  uint32_t nextFrameIndex;
  Camera camera;
  static constexpr float cameraSpeed = 1.f;
  glm::vec2 screenCentre;
  bool lockMouse = true;
  double gameTime;
  float buttonPressGracePeriod = 0.2f;
  struct SettingsLastChangeTimes
  {
    float toggleMouseLock;
  } settingsLastChangeTimes;
  struct CamRot {
    float roll, yaw, pitch;
  } camRot;
};
