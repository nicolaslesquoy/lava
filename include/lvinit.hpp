// Contains functions for initializing the library.
// TODO(nlesquoy): add proper header doc.
// Stnadard libraries
#include <fstream>
#include <iostream>
#include <vector>
#include <vulkan/vulkan.h>

// Third-party libraries
#include <VkBootstrap.h>
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

// Structs definitions
struct VulkanContext {
    vkb::Instance instance;
    vkb::PhysicalDevice physical_device;
    vkb::Device device;
    VmaAllocator allocator;
};

struct VulkanError {
    std::string message;
    bool success;
};

struct Buffer {
    VkBuffer buffer;
    VmaAllocation allocation;
    void* mapped_data;
    VkDeviceSize size;
};

struct ComputePipeline {
    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSetLayout descriptor_layout;
    VkDescriptorSet descriptor_set;
    VkCommandPool command_pool;
    VkCommandBuffer command_buffer;
};