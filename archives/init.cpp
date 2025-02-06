// Standard includes
#include <fstream>
#include <iostream>
#include <vector>

// Vulkan-specific includes
#include <VkBootstrap.h>
#include <vulkan/vulkan.h>

#define VMA_IMPLEMENTATION //! Only once
#include "vk_mem_alloc.h"

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

VulkanError init_vulkan(VulkanContext& ctx)
{
    // Create instance with vk-bootstrap
    // clang-format off
    // TODO(nlesquoy): Add possibility to enable/disable validation layers through a compile-time flag!
    // clang-format on
    vkb::InstanceBuilder instance_builder;
    auto inst_ret = instance_builder.request_validation_layers()
                        .set_headless() // Enable headless mode
                        .use_default_debug_messenger()
                        .build();
    if(!inst_ret) { return VulkanError{inst_ret.error().message(), false}; }
    ctx.instance = inst_ret.value();

    // Select physical device
    vkb::PhysicalDeviceSelector selector{ctx.instance};
    auto phys_ret = selector.set_minimum_version(1, 1)
                        .require_dedicated_transfer_queue()
                        .select();
    if(!phys_ret) { return VulkanError{phys_ret.error().message(), false}; }
    ctx.physical_device = phys_ret.value();

    // Create logical device
    vkb::DeviceBuilder device_builder{ctx.physical_device};
    auto dev_ret = device_builder.build();
    if(!dev_ret) { return VulkanError{dev_ret.error().message(), false}; }
    ctx.device = dev_ret.value();

    // Initialize VMA allocator
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = ctx.physical_device.physical_device;
    allocatorInfo.device = ctx.device.device;
    allocatorInfo.instance = ctx.instance.instance;

    if(vmaCreateAllocator(&allocatorInfo, &ctx.allocator) != VK_SUCCESS) {
        return VulkanError{
            "Failed to create VMA allocator: VMA allocation failed", false};
    }

    return VulkanError{"", true};
}
