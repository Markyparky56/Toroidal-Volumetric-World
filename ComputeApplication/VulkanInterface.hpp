#pragma once
#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include <array>
#include <string>
#include <thread>
#include <functional>
#include <stdexcept>

#include "taskflow\taskflow.hpp"
#include "VulkanInterface.Functions.hpp"
#include "VulkanInterface.OSWindow.hpp"
#include "VulkanInterface.VulkanHandle.hpp"
#include "vk_mem_alloc.h"

#undef CreateSemaphore

namespace VulkanInterface
{
  struct QueueInfo {
    uint32_t familyIndex;
    std::vector<float> priorities;
  };

  struct QueueParameters {
    VkQueue handle;
    uint32_t familyIndex;
  };

  struct SwapchainParameters {
    VulkanHandle(VkSwapchainKHR) handle;
    VkFormat format;
    VkExtent2D size;
    std::vector<VkImage> images;
    std::vector<VulkanHandle(VkImageView)> imageViews;
    std::vector<VkImageView> imageViewsRaw;
  };

  struct FrameResources {
    VkCommandBuffer commandBuffer;
    VulkanHandle(VkSemaphore) imageAcquiredSemaphore;
    VulkanHandle(VkSemaphore) readyToPresentSemaphore;
    VulkanHandle(VkFence) drawingFinishedFence;
    VulkanHandle(VkImageView) depthAttachment;
    VulkanHandle(VkFramebuffer) framebuffer;
  };

  struct WaitSemaphoreInfo {
    VkSemaphore Semaphore;
    VkPipelineStageFlags WaitingStage;
  };

  struct PresentInfo {
    VkSwapchainKHR swapchain;
    uint32_t imageIndex;
  };

  struct BufferTransition {
    VkBuffer buffer;
    VkAccessFlags currentAccess;
    VkAccessFlags newAccess;
    uint32_t currentQueueFamily;
    uint32_t newQueueFamily;
  };

  struct ImageTransition {
    VkImage image;
    VkAccessFlags currentAccess;
    VkAccessFlags newAccess;
    VkImageLayout currentLayout;
    VkImageLayout newLayout;
    uint32_t currentQueueFamily;
    uint32_t newQueueFamily;
    VkImageAspectFlags aspect;
  };

  struct ImageDescriptorInfo {
    VkDescriptorSet targetDescriptorSet;
    uint32_t targetDescriptorBinding;
    uint32_t targetArrayElement;
    VkDescriptorType targetDescriptorType;
    std::vector<VkDescriptorImageInfo> imageInfos;
  };

  struct BufferDescriptorInfo {
    VkDescriptorSet targetDescriptorSet;
    uint32_t targetDescriptorBinding;
    uint32_t targetArrayElement;
    VkDescriptorType targetDescriptorType;
    std::vector<VkDescriptorBufferInfo> bufferInfos;
  };

  struct TexelBufferDescriptorInfo {
    VkDescriptorSet targetDescriptorSet;
    uint32_t targetDescriptorBinding;
    uint32_t targetArrayElement;
    VkDescriptorType targetDescriptorType;
    std::vector<VkBufferView> texelBufferViews;
  };

  struct CopyDescriptorInfo {
    VkDescriptorSet targetDescriptorSet;
    uint32_t targetDescriptorBinding;
    uint32_t targetArrayElement;
    VkDescriptorSet sourceDescriptorSet;
    uint32_t sourceDescriptorBinding;
    uint32_t sourceArrayElement;
    uint32_t descriptorCount;
  };

  struct SubpassParameters {
    VkPipelineBindPoint pipelineType;
    std::vector<VkAttachmentReference> inputAttachments;
    std::vector<VkAttachmentReference> colourAttachments;
    std::vector<VkAttachmentReference> resolveAttachments;
    VkAttachmentReference const * depthStencilAttachment;
    std::vector<uint32_t> preserveAttachments;
  };

  struct ShaderStageParameters {
    VkShaderStageFlagBits shaderStage;
    VkShaderModule shaderModule;
    char const * entryPointName;
    VkSpecializationInfo const * specialisationInfo;
  };

  struct ViewportInfo {
    std::vector<VkViewport> viewports;
    std::vector<VkRect2D> scisscors;
  };

  struct VertexBufferParameters {
    VkBuffer buffer;
    VkDeviceSize memoryOffset;
  };

  struct CommandBufferRecordingParameters {
    VkCommandBuffer commandBuffer;
    std::function<bool(VkCommandBuffer)> recordingFunction;
  };

  bool SetupDebugCallback(VkInstance instance
                        , PFN_vkDebugUtilsMessengerCallbackEXT debugCallbackFunc
                        , VkDebugUtilsMessengerEXT & callback);

  bool LoadVulkanLoaderLibrary(LIBRARY_TYPE & library);
  void ReleaseVulkanLoaderLibrary(LIBRARY_TYPE & library);
  bool LoadVulkanFunctionGetter(LIBRARY_TYPE & library);
  bool LoadGlobalVulkanFunctions();
  bool LoadInstanceLevelVulkanFunctions(VkInstance instance
                                      , std::vector<char const *> const & enabledExtensions);
  bool LoadDeviceLevelVulkanFunctions(VkDevice device
                                    , std::vector<char const *> const & enabledExtensions);

  bool GetAvailableLayerSupport(std::vector<VkLayerProperties> & availableLayers);
  bool IsLayerSupported( std::vector<VkLayerProperties> const & availableLayers
                       , char const * const layer);
  bool GetAvailableInstanceExtensions(std::vector<VkExtensionProperties> & availableExtensions);
  bool IsExtensionSupported( std::vector<VkExtensionProperties> const & availableExtensions
                           , char const * const extension);

  bool CreateVulkanInstance(std::vector<char const *> const & desiredExtensions
                          , std::vector<char const *> const & desiredLayers
                          , char const * const applicationName
                          , VkInstance & instance);  

  bool EnumerateAvailablePhysicalDevices(VkInstance instance
    , std::vector<VkPhysicalDevice> & availableDevices);
  bool CheckAvailableDeviceExtensions(VkPhysicalDevice physicalDevice
                                    , std::vector<VkExtensionProperties> &availableExtensions);
  void GetFeaturesAndPropertiesOfPhysicalDevice(VkPhysicalDevice physicalDevice
                                              , VkPhysicalDeviceFeatures &deviceFeatures
                                              , VkPhysicalDeviceProperties &deviceProperties);
  bool CheckAvailableQueueFamiliesAndTheirProperties( VkPhysicalDevice physicalDevice
                                                    , std::vector<VkQueueFamilyProperties> &queueFamilies);
  bool SelectIndexOfQueueFamilyWithDesiredCapabilities( VkPhysicalDevice physicalDevice
                                                      , VkQueueFlags desiredCapabilities
                                                      , uint32_t &queueFamilyIndex);
  bool SelectQueueFamilyThatSupportsPresentationToGivenSurface( VkPhysicalDevice physicalDevice
                                                              , VkSurfaceKHR presentationSurface
                                                              , uint32_t & queueFamilyIndex);

  bool CreateLogicalDevice( VkPhysicalDevice physicalDevice
                          , std::vector<QueueInfo> queueInfos
                          , std::vector<char const *> const & desiredExtensions
                          , std::vector<char const *> const & desiredLayers
                          , VkPhysicalDeviceFeatures * desiredFeatures
                          , VkDevice & logicalDevice);

  bool CreatePresentationSurface( VkInstance instance
                                , WindowParameters windowParameters
                                , VkSurfaceKHR & presentationSurface);
  bool SelectDesiredPresentationMode( VkPhysicalDevice physicalDevice
                                    , VkSurfaceKHR presentationSurface
                                    , VkPresentModeKHR desiredPresentMode
                                    , VkPresentModeKHR & presentMode);
  bool GetCapabilitiesOfPresentationSurface(VkPhysicalDevice physicalDevice
                                          , VkSurfaceKHR presentationSurface
                                          , VkSurfaceCapabilitiesKHR & surfaceCapabilities);
  bool SelectNumberOfSwapchainImages( VkSurfaceCapabilitiesKHR const & surfaceCapabilities
                                    , uint32_t & numberOfImages);
  bool ChooseSizeOfSwapchainImages( VkSurfaceCapabilitiesKHR const & surfaceCapabilities
                                  , VkExtent2D & sizeOfImages);
  bool SelectDesiredUsageScenariosOfSwapchainImages(VkSurfaceCapabilitiesKHR const & surfaceCapabilities
                                                  , VkImageUsageFlags desiredUsages
                                                  , VkImageUsageFlags & imageUsage);
  bool SelectTransformationOfSwapchainImages( VkSurfaceCapabilitiesKHR const & surfaceCapabilities
                                            , VkSurfaceTransformFlagBitsKHR desiredTransform
                                            , VkSurfaceTransformFlagBitsKHR & surfaceTransform);
  bool SelectFormatOfSwapchainImages( VkPhysicalDevice physicalDevice
                                    , VkSurfaceKHR presentationSurface
                                    , VkSurfaceFormatKHR desiredSurfaceFormat
                                    , VkFormat & imageFormat
                                    , VkColorSpaceKHR & imageColorSpace);
  bool CreateSwapchain( VkDevice logicalDevice
                      , VkSurfaceKHR presentationSurface
                      , uint32_t imageCount
                      , VkSurfaceFormatKHR surfaceFormat
                      , VkExtent2D imageSize
                      , VkImageUsageFlags imageUsage
                      , VkSurfaceTransformFlagBitsKHR surfaceTransform
                      , VkPresentModeKHR presentMode
                      , VkSwapchainKHR & oldSwapchain
                      , VkSwapchainKHR & swapchain);
  // RGBA 8-bit channels, Mailbox Present Mode
  bool CreateStandardSwapchain( VkPhysicalDevice physicalDevice
                              , VkSurfaceKHR presentationSurface
                              , VkDevice logicalDevice
                              , VkImageUsageFlags swapchainImageUsage
                              , VkExtent2D & imageSize
                              , VkFormat & imageFormat
                              , VkSwapchainKHR & oldSwapchain
                              , VkSwapchainKHR & swapchain
                              , std::vector<VkImage> & swapchainImages);
  bool GetHandlesOfSwapchainImages( VkDevice logicalDevice
                                  , VkSwapchainKHR swapchain
                                  , std::vector<VkImage> & swapchainImages);
  bool AcquireSwapchainImage( VkDevice logicalDevice
                            , VkSwapchainKHR swapchain
                            , VkSemaphore semaphore
                            , VkFence fence
                            , uint32_t & imageIndex);
  bool PresentImage(VkQueue queue
                  , std::vector<VkSemaphore> renderingSemaphores
                  , std::vector<PresentInfo> imagesToPresent);


  bool CreateCommandPool( VkDevice logicalDevice
                        , VkCommandPoolCreateFlags parameters
                        , uint32_t queueFamily
                        , VkCommandPool & commandPool);
  bool AllocateCommandBuffers(VkDevice logicalDevice
                            , VkCommandPool commandPool
                            , VkCommandBufferLevel level
                            , uint32_t count
                            , std::vector<VkCommandBuffer> & commandBuffers);
  bool BeginCommandBufferRecordingOp( VkCommandBuffer commandBuffer
                                    , VkCommandBufferUsageFlags usage
                                    , VkCommandBufferInheritanceInfo * secondaryCommandBufferInfo);
  bool EndCommandBufferRecordingOp(VkCommandBuffer commandBuffer);
  bool ResetCommandBuffer(VkCommandBuffer commandBuffer
                        , bool releaseResources);
  bool ResetCommandPool(VkDevice logicalDevice
                      , VkCommandPool commandPool
                      , bool releaseResources);
  bool CreateSemaphore( VkDevice logicalDevice
                      , VkSemaphore & semaphore);
  bool CreateFence( VkDevice logicalDevice
                  , bool signaled
                  , VkFence & fence);
  bool WaitForFences( VkDevice logicalDevice
                    , std::vector<VkFence> const & fences
                    , VkBool32 waitForAll
                    , uint64_t timeout);
  bool ResetFences( VkDevice logicalDevice
                  , std::vector<VkFence> const & fences);
  bool SubmitCommandBuffersToQueue( VkQueue queue
                                  , std::vector<WaitSemaphoreInfo> waitSemaphoreInfos
                                  , std::vector<VkCommandBuffer> commandBuffers
                                  , std::vector<VkSemaphore> signalSemaphores
                                  , VkFence fence);
  bool SynchroniseTwoCommandBuffers(VkQueue firstQueue
                                  , std::vector<WaitSemaphoreInfo> firstWaitSemaphoreInfos
                                  , std::vector<VkCommandBuffer> firstCommandBuffers
                                  , std::vector<WaitSemaphoreInfo> synchronisingSemaphores
                                  , VkQueue secondQueue
                                  , std::vector<VkCommandBuffer> secondCommandBuffers
                                  , std::vector<VkSemaphore> secondSignalSemaphores
                                  , VkFence secondFence);
  bool CheckIfProcesingOfSubmittedCommandBuffersHasFinished(VkDevice logicalDevice
                                                          , VkQueue queue
                                                          , std::vector<WaitSemaphoreInfo> waitSemaphoreInfos
                                                          , std::vector<VkCommandBuffer> commandBuffers
                                                          , std::vector<VkSemaphore> signalSemaphore
                                                          , VkFence fence
                                                          , uint64_t timeout);
  bool WaitUntilAllCommandsSubmittedToQueueAreFinished(VkQueue queue);
  bool WaitForAllSubmittedCommandsToBeFinished(VkDevice logicalDevice);
  void FreeCommandBuffers(VkDevice logicalDevice
                        , VkCommandPool commandPool
                        , std::vector<VkCommandBuffer> & commandBuffers);
  
  bool CreateBuffer(VkDevice logicalDevice
                  , VkDeviceSize size
                  , VkBufferUsageFlags usage
                  , VkBuffer & buffer);
  bool AllocateAndBindMemoryObjectToBuffer( VkPhysicalDevice physicalDevice
                                          , VkDevice logicalDevice
                                          , VkBuffer buffer
                                          , VkMemoryPropertyFlagBits memoryProperties
                                          , VkDeviceMemory & memoryObject);
  void SetBufferMemoryBarrier(VkCommandBuffer commandBuffer
                            , VkPipelineStageFlags generatingStages
                            , VkPipelineStageFlags consumingStages
                            , std::vector<BufferTransition> bufferTransitions);
  bool CreateBufferView(VkDevice logicalDevice
                      , VkBuffer buffer
                      , VkFormat format
                      , VkDeviceSize memoryOffset
                      , VkDeviceSize memoryRange
                      , VkBufferView & bufferView);
  bool CreateImage( VkDevice logicalDevice
                  , VkImageType type
                  , VkFormat format
                  , VkExtent3D size
                  , uint32_t numMipmaps
                  , uint32_t numLayers
                  , VkSampleCountFlagBits samples
                  , VkImageUsageFlags usageScenarios
                  , bool cubemap
                  , VkImage & image);
  bool AllocateAndBindMemoryObjectToImage(VkPhysicalDevice physicalDevice
                                        , VkDevice logicalDevice
                                        , VkImage image
                                        , VkMemoryPropertyFlagBits memoryProperties
                                        , VkDeviceMemory & memoryObject);
  void SetImageMemoryBarrier( VkCommandBuffer commandBuffer
                            , VkPipelineStageFlags generatingStages
                            , VkPipelineStageFlags consumingStages
                            , std::vector<ImageTransition> imageTransitions);
  bool CreateImageView( VkDevice logicalDevice
                      , VkImage image
                      , VkImageViewType viewType
                      , VkFormat format
                      , VkImageAspectFlags aspect
                      , VkImageView & imageView);
  bool Create2DImageAndView(VkPhysicalDevice physicalDevice
                          , VkDevice logicalDevice
                          , VkFormat format
                          , VkExtent2D size
                          , uint32_t numMipmaps
                          , uint32_t numLayers
                          , VkSampleCountFlagBits samples
                          , VkImageUsageFlags usage
                          , VkImageAspectFlags aspect
                          , VkImage & image
                          , VkDeviceMemory & memoryObject
                          , VkImageView & imageView);
  bool CreateLayered2DImageWithCubemapView( VkPhysicalDevice physicalDevice
                                          , VkDevice logicalDevice
                                          , uint32_t size
                                          , uint32_t numMipmaps
                                          , VkImageUsageFlags usage
                                          , VkImageAspectFlags aspect
                                          , VkImage & image
                                          , VkDeviceMemory & memoryObject
                                          , VkImageView & imageView);
  bool MapUpdateAndUnmapHostVisibleMemory(VkDevice logicalDevice
                                        , VkDeviceMemory memoryObject
                                        , VkDeviceSize offset
                                        , VkDeviceSize dataSize
                                        , void * data
                                        , bool unmap
                                        , void ** pointer);
  void CopyDataBetweenBuffers(VkCommandBuffer commandBuffer
                            , VkBuffer sourceBuffer
                            , VkBuffer destinationBuffer
                            , std::vector<VkBufferCopy> regions);
  void CopyDataFromBufferToImage( VkCommandBuffer commandBuffer
                                , VkBuffer sourceBuffer
                                , VkImage destinationImage
                                , VkImageLayout imageLayout
                                , std::vector<VkBufferImageCopy> regions);
  void CopyDataFromImageToBuffer( VkCommandBuffer commandBuffer
                                , VkImage sourceImage
                                , VkImageLayout imageLayout
                                , VkBuffer destinationBuffer
                                , std::vector<VkBufferImageCopy> regions);
  bool UseStagingBufferToUpdateBufferWithDeviceLocalMemoryBound(
                                  VkPhysicalDevice physicalDevice
                                , VkDevice logicalDevice
                                , VkDeviceSize dataSize
                                , void * data
                                , VkBuffer destinationBuffer
                                , VkDeviceSize destinationOffset
                                , VkAccessFlags destinationBufferCurrentAccess
                                , VkAccessFlags destinationBufferNewAccess
                                , VkPipelineStageFlags destinationBufferGeneratingStages
                                , VkPipelineStageFlags destinationBufferConsumingStages
                                , VkQueue queue
                                , VkCommandBuffer commandBuffer
                                , std::vector<VkSemaphore> signalSemaphores);
  bool UseStagingBufferToUpdateImageWithDeviceLocalMemoryBound(
                                  VkPhysicalDevice physicalDevice
                                , VkDevice logicalDevice
                                , VkDeviceSize dataSize
                                , void * data
                                , VkImage destinationImage
                                , VkImageSubresourceLayers destinationImageSubresource
                                , VkOffset3D destinationImageOffset
                                , VkExtent3D destinationImageSize
                                , VkImageLayout destinationImageCurrentLayout
                                , VkImageLayout destinationImageNewLayout
                                , VkAccessFlags destinationImageCurrentAccess
                                , VkAccessFlags destinationImageNewAccess
                                , VkImageAspectFlags destinationImageAspect
                                , VkPipelineStageFlags destinationImageGeneratingStages
                                , VkPipelineStageFlags destinationImageConsumingStages
                                , VkQueue queue
                                , VkCommandBuffer commandBuffer
                                , std::vector<VkSemaphore> signalSemaphores);

  bool CreateSampler( VkDevice logicalDevice
                    , VkFilter magFilter
                    , VkFilter minFilter
                    , VkSamplerMipmapMode mipmapMode
                    , VkSamplerAddressMode uAddressMode
                    , VkSamplerAddressMode vAddressMode
                    , VkSamplerAddressMode wAddressMode
                    , float lodBias
                    , bool anisotropyEnable
                    , float maxAnisotropy
                    , bool compareEnable
                    , VkCompareOp compareOperator
                    , float minLod
                    , float maxLod
                    , VkBorderColor borderColor
                    , bool unnormalisedCoords
                    , VkSampler & sampler);

  bool CreateSampledImage(VkPhysicalDevice physicalDevice
                        , VkDevice logicalDevice
                        , VkImageType type
                        , VkFormat format
                        , VkExtent3D size
                        , uint32_t numMipmaps
                        , uint32_t numLayers
                        , VkImageUsageFlags usage
                        , VkImageViewType viewType
                        , VkImageAspectFlags aspect
                        , bool linearFiltering
                        , VkImage & sampledImage
                        , VkDeviceMemory & memoryObject
                        , VkImageView & sampledImageView);

  bool CreateCombinedImageSampler(VkPhysicalDevice physicalDevice
                                , VkDevice logicalDevice
                                , VkImageType type
                                , VkFormat format
                                , VkExtent3D size
                                , uint32_t numMipmaps
                                , uint32_t numLayers
                                , VkImageUsageFlags usage
                                , VkImageViewType viewType
                                , VkImageAspectFlags aspect
                                , VkFilter magFilter
                                , VkFilter minFilter
                                , VkSamplerMipmapMode mipmapMode
                                , VkSamplerAddressMode uAddressMode
                                , VkSamplerAddressMode vAddressMode
                                , VkSamplerAddressMode wAddressMode
                                , float lodBias
                                , bool anistropyEnable
                                , float maxAnisotropy
                                , bool compareEnable
                                , VkCompareOp compareOperator
                                , float minLod
                                , float maxLod
                                , VkBorderColor borderColor
                                , bool unnormalisedCoords
                                , VkSampler & sampler
                                , VkImage & sampledImage
                                , VkDeviceMemory & memoryObject
                                , VkImageView & sampledImageView);

  bool CreateStorageImage(VkPhysicalDevice physicalDevice
                        , VkDevice logicalDevice
                        , VkImageType type
                        , VkFormat format
                        , VkExtent3D size
                        , uint32_t numMipmaps
                        , uint32_t numLayers
                        , VkImageUsageFlags usage
                        , VkImageViewType viewType
                        , VkImageAspectFlags aspect
                        , bool atomicOperations
                        , VkImage & storageImage
                        , VkDeviceMemory & memoryObject
                        , VkImageView & storageImagesView);

  bool CreateUniformTexelBuffer(VkPhysicalDevice physicalDevice
                              , VkDevice logicalDevice
                              , VkFormat format
                              , VkDeviceSize size
                              , VkImageUsageFlags usage
                              , VkBuffer & uniformTexelBuffer
                              , VkDeviceMemory & memoryObject
                              , VkBufferView & uniformTexelBufferView);

  bool CreateStorageTexelBuffer(VkPhysicalDevice physicalDevice
                              , VkDevice logicalDevice
                              , VkFormat format
                              , VkDeviceSize size
                              , VkBufferUsageFlags usage
                              , bool atomicOperations
                              , VkBuffer & storageTexelBuffer
                              , VkDeviceMemory & memoryObject
                              , VkBufferView & storageTexelBufferView);

  bool CreateUniformBuffer( VkPhysicalDevice physicalDevice
                          , VkDevice logicalDevice
                          , VkDeviceSize size
                          , VkBufferUsageFlags usage
                          , VkBuffer & uniformBuffer
                          , VkDeviceMemory & memoryObject);

  bool CreateStorageBuffer( VkPhysicalDevice physicalDevice
                          , VkDevice logicalDevice
                          , VkDeviceSize size
                          , VkBufferUsageFlags usage
                          , VkBuffer & storageBuffer
                          , VkDeviceMemory & memoryObject);

  bool CreateInputAttachment( VkPhysicalDevice physicalDevice
                            , VkDevice logicalDevice
                            , VkImageType type
                            , VkFormat format
                            , VkExtent3D size
                            , VkImageUsageFlags usage
                            , VkImageViewType viewType
                            , VkImageAspectFlags aspect
                            , VkImage & inputAttachment
                            , VkDeviceMemory & memoryObject
                            , VkImageView & inputAttachmentImageView);

  bool CreateDescriptorSetLayout( VkDevice logicalDevice
                                , std::vector<VkDescriptorSetLayoutBinding> const & bindings
                                , VkDescriptorSetLayout & descriptorSetLayout);

  bool CreateDescriptorPool(VkDevice logicalDevice
                          , bool freeIndividualSets
                          , uint32_t maxSetsCount
                          , std::vector<VkDescriptorPoolSize> const & descriptorTypes
                          , VkDescriptorPool & descriptorPool);

  bool AllocateDescriptorSets(VkDevice logicalDevice
                            , VkDescriptorPool descriptorPool
                            , std::vector<VkDescriptorSetLayout> const & descriptorSetLayouts
                            , std::vector<VkDescriptorSet> & descriptorSets);

  void UpdateDescriptorSets(VkDevice logicalDevice
                          , std::vector<ImageDescriptorInfo> const & imageDescriptorInfos
                          , std::vector<BufferDescriptorInfo> const & bufferDescriptorInfos
                          , std::vector<TexelBufferDescriptorInfo> const & texelBufferDescriptorInfos
                          , std::vector<CopyDescriptorInfo> const & copyDescriptorInfos);

  void BindDescriptorSets(VkCommandBuffer commandBuffer
                        , VkPipelineBindPoint pipelineType
                        , VkPipelineLayout pipelineLayout
                        , uint32_t indexForFirstSet
                        , std::vector<VkDescriptorSet> const & descriptorSets
                        , std::vector<uint32_t> const & dynamicOffsets);

  bool CreateDescriptorsWithTextureAndUniformBuffer(VkPhysicalDevice physicalDevice
                                                  , VkDevice logicalDevice
                                                  , VkExtent3D sampledImageSize
                                                  , uint32_t uniformBufferSize
                                                  , VkSampler & sampler
                                                  , VkImage & sampledImage
                                                  , VkDeviceMemory & sampledImageMemoryObject
                                                  , VkImageView & sampledImageView
                                                  , VkBuffer & uniformBuffer
                                                  , VkDeviceMemory & uniformBufferMemoryObject
                                                  , VkDescriptorSetLayout & descriptorSetLayout
                                                  , VkDescriptorPool & descriptorPool
                                                  , std::vector<VkDescriptorSet> & descriptorSets);

  bool FreeDescriptorSets(VkDevice logicalDevice
                        , VkDescriptorPool descriptorPool
                        , std::vector<VkDescriptorSet> & descriptorSets);

  bool ResetDescriptorPool( VkDevice logicalDevice
                          , VkDescriptorPool descriptorPool);

  void SpecifySubpassDescriptions(std::vector<SubpassParameters> const & subpassParameters
                                , std::vector<VkSubpassDescription> & subpassDescriptions);

  bool CreateRenderPass(VkDevice logicalDevice
                      , std::vector<VkAttachmentDescription> const & attachmentDescriptions
                      , std::vector<SubpassParameters> const & subpassParameters
                      , std::vector<VkSubpassDependency> const & subpassDependencies
                      , VkRenderPass & renderPass);

  bool CreateFramebuffer( VkDevice logicalDevice
                        , VkRenderPass renderPass
                        , std::vector<VkImageView> const & attachments
                        , uint32_t width
                        , uint32_t height
                        , uint32_t layers
                        , VkFramebuffer & framebuffer);

  void BeginRenderPass( VkCommandBuffer commandBuffer
                      , VkRenderPass renderPass
                      , VkFramebuffer framebuffer
                      , VkRect2D renderArea
                      , std::vector<VkClearValue> const & clearValues
                      , VkSubpassContents subpassContents);

  void ProgressToNextSubpass( VkCommandBuffer commandBuffer
                            , VkSubpassContents subpassContents);

  void EndRenderPass(VkCommandBuffer commandBuffer);

  bool CreateShaderModule(VkDevice logicalDevice
                        , std::vector<unsigned char> const & sourceCode
                        , VkShaderModule & shaderModule);

  bool CreateShaderModule(VkDevice logicalDevice
                        , std::vector<uint32_t> const & sourceCode
                        , VkShaderModule & shaderModule);

  void SpecifyPipelineShaderStages( std::vector<ShaderStageParameters> const & shaderStageParams
                                  , std::vector<VkPipelineShaderStageCreateInfo> & shaderStageCreateInfos);

  void SpecifyPipelineVertexInputState( std::vector<VkVertexInputBindingDescription> const & bindingDescriptions
                                      , std::vector<VkVertexInputAttributeDescription> const & attributeDescriptions
                                      , VkPipelineVertexInputStateCreateInfo & vertexInputStateCreateInfo);

  void SpecifyPipelineInputAssemblyState( VkPrimitiveTopology topology
                                        , bool primitiveRestartEnable
                                        , VkPipelineInputAssemblyStateCreateInfo & inputAssemblyStateCreateInfo);

  void SpecifyPipelineTessellationState(uint32_t patchControlPointsCount
                                      , VkPipelineTessellationStateCreateInfo & tesselationStateCreateInfo);

  void SpecifyPipelineViewportAndScissorTestState(ViewportInfo const & viewportInfos
                                                , VkPipelineViewportStateCreateInfo & viewportStateCreateInfo);

  void SpecifyPipelineRasterisationState( bool depthClampEnable
                                        , bool rasteriserDiscardEnable
                                        , VkPolygonMode polygonMode
                                        , VkCullModeFlags cullingMode
                                        , VkFrontFace frontFace
                                        , bool depthBiasEnable
                                        , float depthBiasConstantFactor
                                        , float depthBiasClamp
                                        , float depthBiasSlopeFactor
                                        , float lineWidth
                                        , VkPipelineRasterizationStateCreateInfo & rasterisationStateCreateInfo);

  void SpecifyPipelineMultisampleState( VkSampleCountFlagBits sampleCount
                                      , bool perSampleShadingEnable
                                      , float minSampleShading
                                      , VkSampleMask const * sampleMask
                                      , bool alphaToCoverageEnable
                                      , bool alphaToOneEnable
                                      , VkPipelineMultisampleStateCreateInfo & multisampleStateCreateInfo);

  void SpecifyPipelineDepthAndStencilState( bool depthTestEnable
                                          , bool depthWriteEnable
                                          , VkCompareOp depthCompareOp
                                          , bool depthBoundsTestEnable
                                          , float minDepthBounds
                                          , float maxDepthBounds
                                          , bool stencilTestEnable
                                          , VkStencilOpState frontStencilTestParameters
                                          , VkStencilOpState backStencilTestParameters
                                          , VkPipelineDepthStencilStateCreateInfo & depthAndStencilStateCreateInfo);

  void SpecifyPipelineBlendState( bool logicOpEnable
                                , VkLogicOp logicOp
                                , std::vector<VkPipelineColorBlendAttachmentState> const & attachmentBlendStates
                                , std::array<float, 4> const & blendConstants
                                , VkPipelineColorBlendStateCreateInfo & blendStateCreateInfo);

  void SpecifyPipelineDynamicStates(std::vector<VkDynamicState> const & dynamicStates
                                  , VkPipelineDynamicStateCreateInfo & dynamicStateCreateInfo);

  bool CreatePipelineLayout(VkDevice logicalDevice
                          , std::vector<VkDescriptorSetLayout> const & descriptorSetLayouts
                          , std::vector<VkPushConstantRange> const & pushConstantRanges
                          , VkPipelineLayout & pipelineLayout);

  void SpecifyGraphicsPipelineCreationParameters( VkPipelineCreateFlags additionalOptions
                                                , std::vector<VkPipelineShaderStageCreateInfo> const & shaderStageCreateInfos
                                                , VkPipelineVertexInputStateCreateInfo const & vertexInputStateCreateInfo
                                                , VkPipelineInputAssemblyStateCreateInfo const & inputAssemblyStateCreateInfo
                                                , VkPipelineTessellationStateCreateInfo const * tessellationStateCreateInfo
                                                , VkPipelineViewportStateCreateInfo const * viewportStateCreateInfo
                                                , VkPipelineRasterizationStateCreateInfo const & rasterisationStateCreateInfo
                                                , VkPipelineMultisampleStateCreateInfo const * multisampleStateCreateInfo
                                                , VkPipelineDepthStencilStateCreateInfo const * depthAndStencilStateCreateInfo
                                                , VkPipelineColorBlendStateCreateInfo const * blendStateCreateInfo
                                                , VkPipelineDynamicStateCreateInfo const * dynamicStateCreateInfo
                                                , VkPipelineLayout pipelineLayout
                                                , VkRenderPass renderPass
                                                , uint32_t subpass
                                                , VkPipeline basePipelineHandle
                                                , int32_t basePipelineIndex
                                                , VkGraphicsPipelineCreateInfo & graphicsPipelineCreateInfo);

  bool CreatePipelineCacheObject( VkDevice logicalDevice
                                , std::vector<unsigned char> const & cacheData
                                , VkPipelineCache & pipelineCache);

  bool RetrieveDataFromPipelineCache( VkDevice logicalDevice
                                    , VkPipelineCache pipelineCache
                                    , std::vector<unsigned char> & pipelineCacheData);

  bool MergeMultiplePipelineCacheObjects( VkDevice logicalDevice
                                        , VkPipelineCache targetPipelineCache
                                        , std::vector<VkPipelineCache> const & sourcePipelineCaches);

  bool CreateGraphicsPipelines( VkDevice logicalDevice
                              , std::vector<VkGraphicsPipelineCreateInfo> const & graphicsPipelineCreateInfos
                              , VkPipelineCache pipelineCache
                              , std::vector<VkPipeline> & graphicsPipelines);

  bool CreateComputePipeline( VkDevice logicalDevice
                            , VkPipelineCreateFlags additionalOptions
                            , VkPipelineShaderStageCreateInfo const & computeShaderStage
                            , VkPipelineLayout pipelineLayout
                            , VkPipeline basePipelineHandle
                            , VkPipelineCache pipelineCache
                            , VkPipeline & computePipeline);

  void BindPipelineObject(VkCommandBuffer commandBuffer
                        , VkPipelineBindPoint pipelineType
                        , VkPipeline pipeline);

  void ClearColorImage( VkCommandBuffer commandBuffer
                      , VkImage image
                      , VkImageLayout imageLayout
                      , std::vector<VkImageSubresourceRange> const & imageSubresourceRanges
                      , VkClearColorValue & clearColor);

  void ClearDepthStencilImage(VkCommandBuffer commandBuffer
                            , VkImage image
                            , VkImageLayout imageLayout
                            , std::vector<VkImageSubresourceRange> const & imageSubresourceRanges
                            , VkClearDepthStencilValue & clearValue);

  void ClearRenderPassAttachments(VkCommandBuffer commandBuffer
                                , std::vector<VkClearAttachment> const & attachments
                                , std::vector<VkClearRect> const & rects);

  void BindVertexBuffers( VkCommandBuffer commandBuffer
                        , uint32_t firstBinding
                        , std::vector<VertexBufferParameters> const & buffersParameters);

  void BindIndexBuffer( VkCommandBuffer commandBuffer
                      , VkBuffer buffer
                      , VkDeviceSize memoryOffset
                      , VkIndexType indexType);

  void ProvideDataToShadersThroughPushConstants(VkCommandBuffer commandBuffer
                                              , VkPipelineLayout pipelineLayout
                                              , VkShaderStageFlags pipelineStages
                                              , uint32_t offset
                                              , uint32_t size
                                              , void * data);

  void SetViewportStateDynamically( VkCommandBuffer commandBuffer
                                  , uint32_t firstViewport
                                  , std::vector<VkViewport> const & viewports);

  void SetScissorStateDynamically(VkCommandBuffer commandBuffer
                                , uint32_t firstScissor
                                , std::vector<VkRect2D> const & scissors);

  void SetLineWidthStateDynamically(VkCommandBuffer commandBuffer
                                  , float lineWidth);

  void SetDepthBiasStateDynamically(VkCommandBuffer commandBuffer
                                  , float constantFactor
                                  , float clampValue
                                  , float slopeFactor);

  void SetBlendConstantsStateDynamically( VkCommandBuffer commandBuffer
                                        , std::array<float, 4> const & blendConstants);

  void DrawGeometry(VkCommandBuffer commandBuffer
                  , uint32_t vertexCount
                  , uint32_t instanceCount
                  , uint32_t firstVertex
                  , uint32_t firstInstance);

  void DrawIndexedGeometry( VkCommandBuffer commandBuffer
                          , uint32_t indexCount
                          , uint32_t instanceCount
                          , uint32_t firstIndex
                          , uint32_t vertexOffset
                          , uint32_t firstInstance);

  void DispatchComputeWork( VkCommandBuffer commandBuffer
                          , uint32_t xSize
                          , uint32_t ySize
                          , uint32_t zSize);

  bool GetBinaryFileContents( std::string const & filename
                            , std::vector<unsigned char> & contents);

  bool CreateFramebuffersForFrameResources( VkDevice logicalDevice
                                          , VkRenderPass renderPass
                                          , SwapchainParameters & swapchain
                                          , std::vector<FrameResources> & frameResources);

  bool PrepareSingleFrameOfAnimation( VkDevice logicalDevice
                                    , VkQueue graphicsQueue
                                    , VkQueue presentQueue
                                    , VkSwapchainKHR swapchain
                                    , std::vector<WaitSemaphoreInfo> const & waitInfos
                                    , VkSemaphore imageAcquiredSemaphore
                                    , VkSemaphore readyToPresentSemaphore
                                    , VkFence finishedDrawingFence
                                    , std::function<bool(VkCommandBuffer, uint32_t, VkFramebuffer)> recordCommandBuffer
                                    , VkCommandBuffer commandBuffer
                                    , VulkanHandle(VkFramebuffer) & framebuffer);

  bool RenderWithFrameResources( VkDevice logicalDevice
                               , VkQueue graphicsQueue
                               , VkQueue presentQueue
                               , VkSwapchainKHR swapchain
                               , std::vector<WaitSemaphoreInfo> const & waitInfos
                               , std::function<bool(VkCommandBuffer, uint32_t, VkFramebuffer)> recordCommandBuffer
                               , std::vector<FrameResources> & frameResources
                               , uint32_t & nextFrameIndex);

  void ExecuteSecondaryCommandBuffers( VkCommandBuffer commandBuffer
                                     , std::vector<VkCommandBuffer> const & secondaryCommandBuffers);

  bool RecordAndsubmitCommandBuffersConcurrently( std::vector<CommandBufferRecordingParameters> const & recordingOperations
                                                , VkQueue queue
                                                , std::vector<WaitSemaphoreInfo> waitSemaphoreInfos
                                                , std::vector<VkSemaphore> signalSemaphores
                                                , VkFence fence
                                                , tf::Taskflow * const taskflow);

  // Destroy Handlers, set objects to VK_NULL_HANDLE after destruction

  void DestroyLogicalDevice(VkDevice & logicalDevice);

  void DestroyVulkanInstance(VkInstance & instance);

  void DestroySwapchain(VkDevice logicalDevice
    , VkSwapchainKHR & swapchain);

  void DestroyPresentationSurface(VkInstance instance
    , VkSurfaceKHR & presentationSurface);

  void DestroyCommandPool(VkDevice logicalDevice
    , VkCommandPool & commandPool);

  void DestroySemaphore(VkDevice logicalDevice
    , VkSemaphore & semaphore);

  void DestroyFence(VkDevice logicalDevice
    , VkFence & fence);

  void DestroyBuffer(VkDevice logicalDevice
    , VkBuffer & buffer);

  void FreeMemoryObject(VkDevice logicalDevice
    , VkDeviceMemory & memoryObject);

  void DestroyBufferView(VkDevice logicalDevice
    , VkBufferView & bufferView);

  void DestroyImage(VkDevice logicalDevice
    , VkImage & image);

  void DestroyImageView(VkDevice logicalDevice
    , VkImageView & imageView);

  void DestroySampler(VkDevice logicalDevice
    , VkSampler & sampler);

  void DestroyDescriptorSetLayout(VkDevice logicalDevice
    , VkDescriptorSetLayout & descriptorSetLayout);

  void DestroyDescriptorPool(VkDevice logicalDevice
    , VkDescriptorPool & descriptorPool);

  void DestroyFramebuffer(VkDevice logicalDevice
    , VkFramebuffer & framebuffer);

  void DestroyRenderPass(VkDevice logicalDevice
    , VkRenderPass & renderPass);

  void DestroyPipeline(VkDevice logicalDevice
    , VkPipeline & pipeline);

  void DestroyPipelineCache(VkDevice logicalDevice
    , VkPipelineCache & pipelineCache);

  void DestroyPipelineLayout(VkDevice logicalDevice
    , VkPipelineLayout & pipelineLayout);

  void DestroyShaderModule(VkDevice logicalDevice
    , VkShaderModule & shaderModule);

  // VulkanMemoryAllocator Specific Functions
  bool CreateBuffer(VmaAllocator allocator
    , VkDeviceSize size
    , VkBufferUsageFlags bufferUsage
    , VkBuffer & buffer
    , VmaAllocationCreateFlags allocationFlags
    , VmaMemoryUsage memUsage
    , VkMemoryPropertyFlags memoryProperties
    , VmaPool pool
    , VmaAllocation & allocation);

  bool CreateImage(VmaAllocator allocator
    , VkImageType type
    , VkFormat format
    , VkExtent3D size
    , uint32_t numMipmaps
    , uint32_t numLayers
    , VkSampleCountFlagBits samples
    , VkImageUsageFlags usageScenarios
    , bool cubemap
    , VkImage & image
    , VmaAllocationCreateFlags allocationFlags
    , VmaMemoryUsage memUsage
    , VkMemoryPropertyFlags memoryProperties
    , VmaPool pool
    , VmaAllocation & allocation);

  bool Create2DImageAndView(VkDevice logicalDevice
    , VmaAllocator allocator
    , VkFormat format
    , VkExtent2D size
    , uint32_t numMipmaps
    , uint32_t numLayers
    , VkSampleCountFlagBits samples
    , VkImageUsageFlags usage
    , VkImageAspectFlags aspect
    , VkImage & image
    , VkImageView & imageView
    , VmaAllocationCreateFlags allocationFlags
    , VmaMemoryUsage memUsage
    , VkMemoryPropertyFlags memoryProperties
    , VmaPool pool
    , VmaAllocation & allocation);

  bool CreateLayered2DImageWithCubemapView(VkDevice logicalDevice
    , VmaAllocator allocator
    , uint32_t size
    , uint32_t numMipmaps
    , VkImageUsageFlags usage
    , VkImageAspectFlags aspect
    , VkImage & image
    , VkImageView & imageView
    , VmaAllocationCreateFlags allocationFlags
    , VmaMemoryUsage memUsage
    , VkMemoryPropertyFlags memoryProperties
    , VmaPool pool
    , VmaAllocation & allocation);

  bool MapUpdateAndUnmapHostVisibleMemory(VmaAllocator allocator
    , VmaAllocation allocation
    , VkDeviceSize dataSize
    , void * data
    , bool unmap
    , void ** pointer);

  bool UseStagingBufferToUpdateBufferWithDeviceLocalMemoryBound(
      VkDevice logicalDevice
    , VmaAllocator allocator
    , VkDeviceSize dataSize
    , void * data
    , VkBuffer destinationBuffer
    , VkDeviceSize destinationOffset
    , VkAccessFlags destinationBufferCurrentAccess
    , VkAccessFlags destinationBufferNewAccess
    , VkPipelineStageFlags destinationBufferGeneratingStages
    , VkPipelineStageFlags destinationBufferConsumingStages
    , VkQueue queue
    , VkCommandBuffer commandBuffer
    , std::vector<VkSemaphore> signalSemaphores);

  bool UseStagingBufferToUpdateImageWithDeviceLocalMemoryBound(   
      VkDevice logicalDevice
    , VmaAllocator allocator
    , VkDeviceSize dataSize
    , void * data
    , VkImage destinationImage
    , VkImageSubresourceLayers destinationImageSubresource
    , VkOffset3D destinationImageOffset
    , VkExtent3D destinationImageSize
    , VkImageLayout destinationImageCurrentLayout
    , VkImageLayout destinationImageNewLayout
    , VkAccessFlags destinationImageCurrentAccess
    , VkAccessFlags destinationImageNewAccess
    , VkImageAspectFlags destinationImageAspect
    , VkPipelineStageFlags destinationImageGeneratingStages
    , VkPipelineStageFlags destinationImageConsumingStages
    , VkQueue queue
    , VkCommandBuffer commandBuffer
    , std::vector<VkSemaphore> signalSemaphores);

  bool CreateSampledImage(VkPhysicalDevice physicalDevice
    , VkDevice logicalDevice
    , VmaAllocator allocator
    , VkImageType type
    , VkFormat format
    , VkExtent3D size
    , uint32_t numMipmaps
    , uint32_t numLayers
    , VkImageUsageFlags usage
    , VkImageViewType viewType
    , VkImageAspectFlags aspect
    , bool linearFiltering
    , VkImage & sampledImage
    , VmaMemoryUsage memUsage
    , VmaAllocation & allocation
    , VkImageView & sampledImageView);

  bool CreateCombinedImageSampler(VkPhysicalDevice physicalDevice
    , VkDevice logicalDevice
    , VmaAllocator allocator
    , VkImageType type
    , VkFormat format
    , VkExtent3D size
    , uint32_t numMipmaps
    , uint32_t numLayers
    , VkImageUsageFlags usage
    , VkImageViewType viewType
    , VkImageAspectFlags aspect
    , VkFilter magFilter
    , VkFilter minFilter
    , VkSamplerMipmapMode mipmapMode
    , VkSamplerAddressMode uAddressMode
    , VkSamplerAddressMode vAddressMode
    , VkSamplerAddressMode wAddressMode
    , float lodBias
    , bool anistropyEnable
    , float maxAnisotropy
    , bool compareEnable
    , VkCompareOp compareOperator
    , float minLod
    , float maxLod
    , VkBorderColor borderColor
    , bool unnormalisedCoords
    , VkSampler & sampler
    , VkImage & sampledImage
    , VmaMemoryUsage memUsage
    , VmaAllocation & allocation
    , VkImageView & sampledImageView);

  bool CreateStorageImage(VkPhysicalDevice physicalDevice
    , VkDevice logicalDevice
    , VmaAllocator allocator
    , VkImageType type
    , VkFormat format
    , VkExtent3D size
    , uint32_t numMipmaps
    , uint32_t numLayers
    , VkImageUsageFlags usage
    , VkImageViewType viewType
    , VkImageAspectFlags aspect
    , bool atomicOperations
    , VkImage & storageImage
    , VmaMemoryUsage memUsage
    , VmaAllocation & allocation
    , VkImageView & storageImagesView);

  bool CreateUniformTexelBuffer(VkPhysicalDevice physicalDevice
    , VkDevice logicalDevice
    , VmaAllocator allocator
    , VkFormat format
    , VkDeviceSize size
    , VkImageUsageFlags usage
    , VkBuffer & uniformTexelBuffer
    , VmaMemoryUsage memUsage
    , VmaAllocation & allocation
    , VkBufferView & uniformTexelBufferView);

  bool CreateStorageTexelBuffer(VkPhysicalDevice physicalDevice
    , VkDevice logicalDevice
    , VmaAllocator allocator
    , VkFormat format
    , VkDeviceSize size
    , VkBufferUsageFlags usage
    , bool atomicOperations
    , VkBuffer & storageTexelBuffer
    , VmaMemoryUsage meUsage
    , VmaAllocation & allocation
    , VkBufferView & storageTexelBufferView);

  bool CreateUniformBuffer(VmaAllocator allocator
    , VkDeviceSize size
    , VkBufferUsageFlags usage
    , VkBuffer & uniformBuffer
    , VmaMemoryUsage memUsage
    , VmaAllocation & allocation);

  bool CreateStorageBuffer(VmaAllocator allocator
    , VkDeviceSize size
    , VkBufferUsageFlags usage
    , VkBuffer & storageBuffer
    , VmaMemoryUsage memUsage
    , VmaAllocation & allocation);

  bool CreateInputAttachment(VkPhysicalDevice physicalDevice
    , VkDevice logicalDevice
    , VmaAllocator allocator
    , VkImageType type
    , VkFormat format
    , VkExtent3D size
    , VkImageUsageFlags usage
    , VkImageViewType viewType
    , VkImageAspectFlags aspect
    , VkImage & inputAttachment
    , VmaMemoryUsage memUsage
    , VmaAllocation & allocation
    , VkImageView & inputAttachmentImageView);

}
