#include "lvinit.hpp"
// TODO(nlesquoy): add proper error handling and debug mode -> compatible with existing error handling from VMA, VkBootstrap and Vulkan proper

VulkanError init_vulkan(VulkanContext& ctx)
{
    // Create instance with vk-bootstrap
    vkb::InstanceBuilder instance_builder;
    // TODO(nlesquoy): modify the instance_builder to support more features
    auto inst_ret = instance_builder.request_validation_layers()
                        .set_headless() // Enable headless mode
                        .build();
    if(!inst_ret) { return VulkanError{inst_ret.error().message(), false}; }
    ctx.instance = inst_ret.value();

    // Select physical device
    // TODO(nlesquoy): modify the physical device selector to allow more choices
    // TODO(nlesquoy) - cont: and to support choices between devices
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

    // TODO(nlesquoy): to replace with home-made error handling
    if(vmaCreateAllocator(&allocatorInfo, &ctx.allocator) != VK_SUCCESS) {
        return VulkanError{
            "Failed to create VMA allocator: VMA allocation failed", false};
    }

    return VulkanError{"", true};
}