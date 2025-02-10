#include "lvinit.hpp"
#include "lvrun.hpp"
#include "lvcleanup.hpp"

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
    std::vector<float> a_data(data_size, 1.0f); // Array of 1's
    std::vector<float> b_data(data_size, 2.0f); // Array of 2's
    std::vector<float> result_data(data_size);

    // Create buffers
    Buffer a_buffer{}, b_buffer{}, result_buffer{};

    // Create input buffer A
    result
        = create_host_visible_buffer(ctx, a_buffer, sizeof(float) * data_size,
                                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    if(!result.success) {
        std::cerr << "Failed to create buffer A: " << result.message
                  << std::endl;
        cleanup_vulkan(ctx);
        return 1;
    }

    // Create input buffer B
    result
        = create_host_visible_buffer(ctx, b_buffer, sizeof(float) * data_size,
                                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    if(!result.success) {
        std::cerr << "Failed to create buffer B: " << result.message
                  << std::endl;
        destroy_buffer(ctx, a_buffer);
        cleanup_vulkan(ctx);
        return 1;
    }

    // Create result buffer
    result = create_host_visible_buffer(ctx, result_buffer,
                                        sizeof(float) * data_size,
                                        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    if(!result.success) {
        std::cerr << "Failed to create result buffer: " << result.message
                  << std::endl;
        destroy_buffer(ctx, a_buffer);
        destroy_buffer(ctx, b_buffer);
        cleanup_vulkan(ctx);
        return 1;
    }

    // Copy input data to buffers
    std::memcpy(a_buffer.mapped_data, a_data.data(), a_buffer.size);
    std::memcpy(b_buffer.mapped_data, b_data.data(), b_buffer.size);

    // Setup compute pipeline
    ComputePipeline compute{};
    result = setup_compute(ctx, compute, a_buffer, b_buffer, result_buffer);
    if(!result.success) {
        std::cerr << "Failed to setup compute pipeline: " << result.message
                  << std::endl;
        destroy_buffer(ctx, a_buffer);
        destroy_buffer(ctx, b_buffer);
        destroy_buffer(ctx, result_buffer);
        cleanup_vulkan(ctx);
        return 1;
    }

    // Execute compute shader
    result = record_and_submit_compute(ctx, compute, data_size);
    if(!result.success) {
        std::cerr << "Failed to execute compute shader: " << result.message
                  << std::endl;
        cleanup_compute(ctx, compute);
        destroy_buffer(ctx, a_buffer);
        destroy_buffer(ctx, b_buffer);
        destroy_buffer(ctx, result_buffer);
        cleanup_vulkan(ctx);
        return 1;
    }

    // Read back results
    std::memcpy(result_data.data(), result_buffer.mapped_data,
                result_buffer.size);

    // Verify results
    bool computation_success = true;
    for(size_t i = 0; i < data_size; i++) {
        float expected = a_data[i] + b_data[i];
        if(std::abs(result_data[i] - expected) > 1e-6) {
            std::cerr << "Computation error at index " << i << ": expected "
                      << expected << ", got " << result_data[i] << std::endl;
            computation_success = false;
            break;
        }
    }

    if(computation_success) {
        std::cout << "Compute shader execution verified successfully!"
                  << std::endl;
    }

    // Cleanup
    cleanup_compute(ctx, compute);
    destroy_buffer(ctx, a_buffer);
    destroy_buffer(ctx, b_buffer);
    destroy_buffer(ctx, result_buffer);
    cleanup_vulkan(ctx);

    return computation_success ? 0 : 1;
}