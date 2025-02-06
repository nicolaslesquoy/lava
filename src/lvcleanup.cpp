#include "lvcleanup.hpp"

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