#include <VkBootstrap.h>
#include <iostream>
#include <vector>
#include <vulkan/vulkan.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

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
    vkb::InstanceBuilder instance_builder;
    auto inst_ret = instance_builder.set_app_name("My App")
                        .request_validation_layers()
                        .set_headless() // Enable headless mode
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

VulkanError create_host_visible_buffer(VulkanContext& ctx, Buffer& buffer,
                                       VkDeviceSize size,
                                       VkBufferUsageFlags usage)
{
    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    alloc_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VmaAllocationInfo allocation_info;
    if(vmaCreateBuffer(ctx.allocator, &buffer_info, &alloc_info, &buffer.buffer,
                       &buffer.allocation, &allocation_info)
       != VK_SUCCESS) {
        return VulkanError{"Failed to create buffer", false};
    }

    buffer.mapped_data = allocation_info.pMappedData;
    buffer.size = size;
    return VulkanError{"", true};
}

void destroy_buffer(VulkanContext& ctx, Buffer& buffer)
{
    if(buffer.buffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(ctx.allocator, buffer.buffer, buffer.allocation);
        buffer.buffer = VK_NULL_HANDLE;
        buffer.mapped_data = nullptr;
    }
}

void cleanup_vulkan(VulkanContext& ctx)
{
    vmaDestroyAllocator(ctx.allocator);
    vkb::destroy_device(ctx.device);
    vkb::destroy_instance(ctx.instance);
}

int main()
{
    VulkanContext ctx{};

    auto result = init_vulkan(ctx);
    if(!result.success) {
        std::cerr << "Failed to initialize Vulkan: " << result.message
                  << std::endl;
        return 1;
    }

    // Create test data
    const size_t data_size = 1024;
    std::vector<float> test_data(data_size);
    for(size_t i = 0; i < data_size; i++) {
        test_data[i] = static_cast<float>(i);
    }

    // Create a buffer for transfer
    Buffer staging_buffer{};
    result = create_host_visible_buffer(ctx, staging_buffer,
                                        sizeof(float) * data_size,
                                        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    if(!result.success) {
        std::cerr << "Failed to create buffer: " << result.message << std::endl;
        cleanup_vulkan(ctx);
        return 1;
    }

    // Write test data to buffer
    std::memcpy(staging_buffer.mapped_data, test_data.data(),
                staging_buffer.size);

    // Read back data
    std::vector<float> read_back_data(data_size);
    std::memcpy(read_back_data.data(), staging_buffer.mapped_data,
                staging_buffer.size);

    // Verify data
    bool transfer_success = true;
    for(size_t i = 0; i < data_size; i++) {
        if(test_data[i] != read_back_data[i]) {
            std::cerr << "Data mismatch at index " << i << ": expected "
                      << test_data[i] << ", got " << read_back_data[i]
                      << std::endl;
            transfer_success = false;
            break;
        }
    }

    if(transfer_success) {
        std::cout << "Data transfer verified successfully!" << std::endl;
    }

    // Cleanup
    destroy_buffer(ctx, staging_buffer);
    cleanup_vulkan(ctx);
    return transfer_success ? 0 : 1;
}