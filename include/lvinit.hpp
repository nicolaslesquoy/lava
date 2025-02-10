// Contains functions for initializing the library.
// TODO(nlesquoy): add proper header doc.
// Stnadard libraries
#include <fstream>
#include <iostream>
#include <vector>
#include <vulkan/vulkan.h>

// Third-party libraries
#include <VkBootstrap.h>
#include "vk_mem_alloc.h"

#ifndef LVINIT_HPP
#define LVINIT_HPP

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

VulkanError init_vulkan(VulkanContext& ctx);

VulkanError create_host_visible_buffer(VulkanContext& ctx, Buffer& buffer,
                                       VkDeviceSize size,
                                       VkBufferUsageFlags usage);

VulkanError create_shader_module(VulkanContext& ctx,
                                 const std::string& filename,
                                 VkShaderModule& shader_module);

#endif // LVINIT_HPP