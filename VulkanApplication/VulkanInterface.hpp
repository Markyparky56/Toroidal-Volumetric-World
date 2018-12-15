#pragma once
#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <thread>
#include <functional>
#include <stdexcept>

#include <vulkan/vulkan.h>
#include "VulkanInterface.Functions.hpp"
#include "VulkanInterface.OSWindow.hpp"
#include "VulkanInterface.VulkanHandle.hpp"

#include "UnrecoverableException.hpp"

#undef CreateSemaphore

namespace VulkanInterface
{
  struct QueueInfo {
    uint32_t FamilyIndex;
    std::vector<float> Priorities;
  };

  struct QueueParameters {
    VkQueue Handle;
    uint32_t FamilyIndex;
  };

  struct SwapchainParameters {
    VulkanHandle<VkSwapchainKHR> handle;
    VkFormat format;
    VkExtent2D size;
    std::vector<VkImage> images;
    std::vector<VulkanHandle<VkImageView>> imageViews;
    std::vector<VkImageView> imageViewsRaw;
  };

  struct FrameResources {
    VkCommandBuffer commandBuffer;
    VulkanHandle<VkSemaphore> imageAcquiredSemaphore;
    VulkanHandle<VkSemaphore> readyToPresentSemaphore;
    VulkanHandle<VkFence> drawingFinishedFence;
    VulkanHandle<VkImageView> depthAttachment;
    VulkanHandle<VkFramebuffer> framebuffer;
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
                                                          , uint64_t timeout
                                                          , VkResult & waitStatus);
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
  void CreateImageView( VkDevice logicalDevice
                      , VkImage image
                      , VkImageViewType viewType
                      , VkFormat format
                      , VkImageAspectFlags aspect
                      , VkImageView & imageView);
  void Create2DImageAndView(VkPhysicalDevice physicalDevice
                          , VkDevice logicalDevice
                          , VkFormat format
                          , VkExtent2D size
                          , uint32_t numMipmaps
                          , uint32_t numLayers
                          , VkSampleCountFlags samples
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
  void UseStagingBufferToUpdateBufferWithDeviceLocalMemoryBound(
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
  void UseStagingBufferToUpdateImageWithDeviceLocalMemoryBound(
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

}
