#pragma once
#include "AppBase.hpp"
#include "taskflow\taskflow.hpp"
#include "vk_mem_alloc.h"
#include "common.hpp"
#include "ChunkManager.hpp"
#include "Camera.hpp"
#include "TerrainGenerator.hpp"
#include "SurfaceExtractor.hpp"
#include "TaskflowCommandPools.hpp"
#include "FrustumClass.hpp"
#include "metrics.hpp"

#include <stack>
#include <array>

class ComputeApp : public AppBase
{
public:
  bool Initialise(VulkanInterface::WindowParameters windowParameters) override;
  bool InitMetrics();
  bool Update() override;
  bool Resize() override;

private:
  void OnMouseEvent() override;

  bool setupVulkanAndCreateSwapchain(VulkanInterface::WindowParameters windowParameters);
  bool setupTaskflow();
  bool initialiseVulkanMemoryAllocator();
  bool setupCommandPoolAndBuffers();
  bool setupRenderPass();
  bool setupGraphicsPipeline();
  bool setupFrameResources();
  bool setupChunkManager();
  bool setupTerrainGenerator();
  bool setupSurfaceExtractor();
  bool setupECS();

  void Shutdown() override;
  void shutdownVulkanMemoryAllocator();
  void shutdownChunkManager();
  void shutdownGraphicsPipeline();
  void cleanupVulkan();

  // App logic
  void updateUser();
  void checkForNewChunks();
  void getChunkRenderList();
  bool drawChunks();

  bool chunkIsWithinFrustum(uint32_t const entity);
  void loadFromChunkCache(EntityHandle handle);
  void generateChunk(EntityHandle handle);
  void loadFromChunkCache(EntityHandle handle, logEntryData & logData);
  void generateChunk(EntityHandle handle, logEntryData & logData);

  // Metrics
  bool logging;
  std::ofstream logFile;


  // cpp-taskflow taskflows and shared executor
  std::unique_ptr<tf::Taskflow> updateTaskflow
                              , graphicsTaskflow
                              , computeTaskflow
                              , systemTaskflow;
  std::shared_ptr<tf::Taskflow::Executor> tfExecutor;

  // Vulkan Memory Allocator
  VmaAllocator allocator;

  // ImGui gets a dedicated descriptor pool with everything
  VkDescriptorPool imGuiDescriptorPool;

  VkRenderPass renderPass;
  std::vector<VulkanInterface::FrameResources> frameResources;
  VkCommandPool frameResourcesCmdPool;
  std::vector<VkCommandBuffer> frameResourcesCmdBuffers;

  std::vector<VkCommandBuffer> recordedChunkDrawCalls;

  VkQueue graphicsQueue, presentQueue, transferQueue;
  std::vector<VkQueue> computeQueues;
  std::mutex graphicsQMutex, transferQMutex;

  std::vector<VkImage> depthImages;
  std::vector<VmaAllocation> depthImagesAllocations;

  VkDescriptorSetLayout descriptorSetLayout;
  VkDescriptorPool descriptorPool;
  std::vector<VkDescriptorSet> descriptorSets;
  std::vector<VkBuffer> viewprojUBuffers, modelUBuffers, lightUBuffers;
  std::vector<VmaAllocation> viewprojAllocs, modelAllocs, lightAllocs;

  VkPipeline graphicsPipeline;
  VkPipelineLayout graphicsPipelineLayout;

  std::unique_ptr<TaskflowCommandPools> commandPools;
  std::unique_ptr<ChunkManager> chunkManager;
  std::unique_ptr<entt::DefaultRegistry> registry;
  std::mutex registryMutex;
  std::unique_ptr<TerrainGenerator> terrainGen;
  std::unique_ptr<SurfaceExtractor> surfaceExtractor;
  Frustum frustum;

  std::vector<std::pair<EntityHandle, ChunkManager::ChunkStatus>> chunkSpawnList;
  std::vector<EntityHandle> chunkRenderList;

  uint32_t nextFrameIndex=0;
  Camera camera;
  glm::mat4 view, proj, vp;
  static constexpr float cameraSpeed = 1.f;
  glm::vec2 screenCentre;
  bool lockMouse = true;
  bool reseedTerrain = false;
  glm::ivec2 mouseDelta;
  double gameTime;
  float buttonPressGracePeriod = 0.2f;
  struct SettingsLastChangeTimes
  {
    float toggleMouseLock;
    float reseed;
  } settingsLastChangeTimes;
  struct CamRot {
    float roll, yaw, pitch;
  } camRot;
  struct PushConstantVertexShaderObject {
    glm::mat4 m;
    glm::mat4 vp;
  };
  struct ViewProj
  {
    glm::mat4 view;
    glm::mat4 proj;
  };
  struct PerChunkData {
    glm::mat4 model;
  };
  size_t dynamicAlignment;
  struct LightData {
    alignas(16) glm::vec3 lightDir;
    alignas(16) glm::vec3 viewPos;
    alignas(16) glm::vec3 lightAmbientColour;
    alignas(16) glm::vec3 lightDiffuseColour;
    alignas(16) glm::vec3 lightSpecularColour;
    alignas(16) glm::vec3 objectColour;
  };
};
