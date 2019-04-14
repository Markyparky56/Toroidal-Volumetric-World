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
  bool initImGui(HWND hwnd);
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
  void shutdownImGui();
  void cleanupVulkan();

  // App logic
  void updateUser();
  void checkForNewChunks();
  void getChunkRenderList();
  void recordChunkDrawCalls();
  bool drawChunks();

  bool chunkIsWithinFrustum(uint32_t const entity);
  void loadFromChunkCache(EntityHandle handle);
  void generateChunk(EntityHandle handle);
  void loadFromChunkCache(EntityHandle handle, logEntryData & logData);
  void generateChunk(EntityHandle handle, logEntryData & logData);
  VkCommandBuffer drawChunkOp(EntityHandle chunk, VkCommandBufferInheritanceInfo * const inheritanceInfo, glm::mat4 vp);

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

  //std::unique_ptr<GraphicsPipeline> graphicsPipeline; // This could feasibly be a vector if we ever want a more complex render setup
                                     // But having explicitly named pipelines would probably be better


  VkRenderPass renderPass;
  std::vector<VulkanInterface::FrameResources> frameResources;
  VkCommandPool frameResourcesCmdPool;
  std::vector<VkCommandBuffer> frameResourcesCmdBuffers;

  //std::vector<VkCommandPool> transferCommandPools;
  //std::vector<std::mutex> transferCommmandPoolMutexes;
  //std::vector<VkCommandBuffer> transferCommandBuffers;
  //std::mutex transferCBufVecMutex;

  //std::vector<VkCommandPool> graphicsCommandPools;
  //std::vector<std::mutex> graphicsCommandPoolMutexes;
  //std::vector<VkCommandBuffer> frameCommandBuffers;
  //std::vector<std::vector<VkCommandBuffer>> chunkCommandBuffersVecs;
  //std::vector<std::array<std::stack<VkCommandBuffer*>, numFrames>> chunkCommandBufferStacks;
  //std::vector<std::vector<VkCommandBuffer
  //std::mutex chunkCBufStackMutex;

  std::vector<VkCommandBuffer> recordedChunkDrawCalls;

  //uint32_t graphicsQueueFamily, presentQueueFamily, computeQueueFamily;
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
  //VkBuffer uniformBuffer;
  //VkDeviceMemory uniformBufferMemory;

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
  glm::ivec2 mouseDelta;
  double gameTime;
  float buttonPressGracePeriod = 0.2f;
  struct SettingsLastChangeTimes
  {
    float toggleMouseLock;
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
