// Contains functions for actually running computations.

// Local libraries
#include "lvinit.hpp"

VulkanError setup_compute(VulkanContext& ctx, ComputePipeline& compute,
                          Buffer& a_buffer, Buffer& b_buffer,
                          Buffer& result_buffer);

VulkanError record_and_submit_compute(VulkanContext& ctx,
                                      ComputePipeline& compute,
                                      uint32_t num_elements);