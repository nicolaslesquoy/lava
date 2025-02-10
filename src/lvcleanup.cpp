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

void cleanup_compute(VulkanContext& ctx, ComputePipeline& compute)
{
    vkDestroyPipeline(ctx.device.device, compute.pipeline, nullptr);
    vkDestroyPipelineLayout(ctx.device.device, compute.pipeline_layout,
                            nullptr);
    vkDestroyDescriptorSetLayout(ctx.device.device, compute.descriptor_layout,
                                 nullptr);
    vkDestroyDescriptorPool(ctx.device.device, compute.descriptor_pool,
                            nullptr);
    vkDestroyCommandPool(ctx.device.device, compute.command_pool, nullptr);
}