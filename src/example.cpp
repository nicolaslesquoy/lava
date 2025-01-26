#include <vulkan/vulkan.h>
#include <VkBootstrap.h>
#include <iostream>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

struct VulkanContext {
    vkb::Instance instance;
    vkb::PhysicalDevice physical_device;
    vkb::Device device;
    VmaAllocator allocator;
};

bool init_vulkan(VulkanContext& ctx) {
    // Create instance with vk-bootstrap
    vkb::InstanceBuilder instance_builder;
    auto inst_ret = instance_builder
        .set_app_name("My App")
        .request_validation_layers()
        .build();
    if (!inst_ret) {
        std::cerr << "Failed to create Vulkan instance" << std::endl;
        return false;
    }
    ctx.instance = inst_ret.value();

    // Select physical device
    vkb::PhysicalDeviceSelector selector{ctx.instance};
    auto phys_ret = selector
        .set_minimum_version(1, 1)
        .select();
    if (!phys_ret) {
        std::cerr << "Failed to select physical device" << std::endl;
        return false;
    }
    ctx.physical_device = phys_ret.value();

    // Create logical device
    vkb::DeviceBuilder device_builder{ctx.physical_device};
    auto dev_ret = device_builder.build();
    if (!dev_ret) {
        std::cerr << "Failed to create logical device" << std::endl;
        return false;
    }
    ctx.device = dev_ret.value();

    // Initialize VMA allocator
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = ctx.physical_device.physical_device;
    allocatorInfo.device = ctx.device.device;
    allocatorInfo.instance = ctx.instance.instance;

    if (vmaCreateAllocator(&allocatorInfo, &ctx.allocator) != VK_SUCCESS) {
        std::cerr << "Failed to create VMA allocator" << std::endl;
        return false;
    }

    return true;
}

void cleanup_vulkan(VulkanContext& ctx) {
    vmaDestroyAllocator(ctx.allocator);
    vkb::destroy_device(ctx.device);
    vkb::destroy_instance(ctx.instance);
}

int main() {
    VulkanContext ctx{};
    
    if (!init_vulkan(ctx)) {
        std::cerr << "Failed to initialize Vulkan" << std::endl;
        return 1;
    }

    std::cout << "Vulkan initialized successfully!" << std::endl;

    cleanup_vulkan(ctx);
    return 0;
}