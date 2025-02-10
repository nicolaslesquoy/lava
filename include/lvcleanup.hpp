// Contains functions for freeing ressources the library.
#include "lvinit.hpp"

void destroy_buffer(VulkanContext& ctx, Buffer& buffer);

void cleanup_vulkan(VulkanContext& ctx);

void cleanup_compute(VulkanContext& ctx, ComputePipeline& compute);