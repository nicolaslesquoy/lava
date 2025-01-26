#include <VkBootstrap.h>
#include <iostream>
#include <vector>
#include <vulkan/vulkan.h>
#include <fstream>

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

struct ComputePipeline {
    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSetLayout descriptor_layout;
    VkDescriptorSet descriptor_set;
    VkCommandPool command_pool;
    VkCommandBuffer command_buffer;
};

VulkanError create_shader_module(VulkanContext& ctx, const std::string& filename, 
                               VkShaderModule& shader_module) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        return VulkanError{"Failed to open shader file", false};
    }

    size_t file_size = (size_t)file.tellg();
    std::vector<char> buffer(file_size);
    
    file.seekg(0);
    file.read(buffer.data(), file_size);
    file.close();

    VkShaderModuleCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = buffer.size();
    create_info.pCode = reinterpret_cast<const uint32_t*>(buffer.data());

    if (vkCreateShaderModule(ctx.device.device, &create_info, nullptr, &shader_module) != VK_SUCCESS) {
        return VulkanError{"Failed to create shader module", false};
    }

    return VulkanError{"", true};
}

VulkanError setup_compute(VulkanContext& ctx, ComputePipeline& compute, 
                         Buffer& a_buffer, Buffer& b_buffer, Buffer& result_buffer) {
    // Create descriptor set layout
    VkDescriptorSetLayoutBinding bindings[3] = {};
    for (int i = 0; i < 3; i++) {
        bindings[i].binding = i;
        bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    }

    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = 3;
    layout_info.pBindings = bindings;

    if (vkCreateDescriptorSetLayout(ctx.device.device, &layout_info, nullptr, 
                                  &compute.descriptor_layout) != VK_SUCCESS) {
        return VulkanError{"Failed to create descriptor set layout", false};
    }

    // Create pipeline layout
    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &compute.descriptor_layout;

    if (vkCreatePipelineLayout(ctx.device.device, &pipeline_layout_info, nullptr,
                              &compute.pipeline_layout) != VK_SUCCESS) {
        return VulkanError{"Failed to create pipeline layout", false};
    }

    // Create shader module
    VkShaderModule compute_shader;
    auto result = create_shader_module(ctx, "../build/shaders/add.comp.spv", compute_shader);
    if (!result.success) return result;

    // Create compute pipeline
    VkComputePipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipeline_info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipeline_info.stage.module = compute_shader;
    pipeline_info.stage.pName = "main";
    pipeline_info.layout = compute.pipeline_layout;

    if (vkCreateComputePipelines(ctx.device.device, VK_NULL_HANDLE, 1,
                                &pipeline_info, nullptr, &compute.pipeline) != VK_SUCCESS) {
        return VulkanError{"Failed to create compute pipeline", false};
    }

    vkDestroyShaderModule(ctx.device.device, compute_shader, nullptr);

    // Create descriptor pool
    VkDescriptorPoolSize pool_size{};
    pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    pool_size.descriptorCount = 3;

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.maxSets = 1;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = &pool_size;

    if (vkCreateDescriptorPool(ctx.device.device, &pool_info, nullptr,
                              &compute.descriptor_pool) != VK_SUCCESS) {
        return VulkanError{"Failed to create descriptor pool", false};
    }

    // Allocate descriptor set
    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = compute.descriptor_pool;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &compute.descriptor_layout;

    if (vkAllocateDescriptorSets(ctx.device.device, &alloc_info,
                                &compute.descriptor_set) != VK_SUCCESS) {
        return VulkanError{"Failed to allocate descriptor sets", false};
    }

    // Update descriptor set
    VkDescriptorBufferInfo buffer_info[3] = {
        {a_buffer.buffer, 0, VK_WHOLE_SIZE},
        {b_buffer.buffer, 0, VK_WHOLE_SIZE},
        {result_buffer.buffer, 0, VK_WHOLE_SIZE}
    };

    VkWriteDescriptorSet descriptor_writes[3] = {};
    for (int i = 0; i < 3; i++) {
        descriptor_writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_writes[i].dstSet = compute.descriptor_set;
        descriptor_writes[i].dstBinding = i;
        descriptor_writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptor_writes[i].descriptorCount = 1;
        descriptor_writes[i].pBufferInfo = &buffer_info[i];
    }

    vkUpdateDescriptorSets(ctx.device.device, 3, descriptor_writes, 0, nullptr);

    // Create command pool and buffer
    VkCommandPoolCreateInfo command_pool_info{};
    command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_info.queueFamilyIndex = ctx.device.get_queue_index(vkb::QueueType::compute).value();

    if (vkCreateCommandPool(ctx.device.device, &command_pool_info, nullptr,
                           &compute.command_pool) != VK_SUCCESS) {
        return VulkanError{"Failed to create command pool", false};
    }

    VkCommandBufferAllocateInfo command_buffer_info{};
    command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_info.commandPool = compute.command_pool;
    command_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_info.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(ctx.device.device, &command_buffer_info,
                                &compute.command_buffer) != VK_SUCCESS) {
        return VulkanError{"Failed to allocate command buffers", false};
    }

    return VulkanError{"", true};
}

void cleanup_compute(VulkanContext& ctx, ComputePipeline& compute) {
    vkDestroyPipeline(ctx.device.device, compute.pipeline, nullptr);
    vkDestroyPipelineLayout(ctx.device.device, compute.pipeline_layout, nullptr);
    vkDestroyDescriptorSetLayout(ctx.device.device, compute.descriptor_layout, nullptr);
    vkDestroyDescriptorPool(ctx.device.device, compute.descriptor_pool, nullptr);
    vkDestroyCommandPool(ctx.device.device, compute.command_pool, nullptr);
}

VulkanError record_and_submit_compute(VulkanContext& ctx, ComputePipeline& compute,
                                    uint32_t num_elements) {
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(compute.command_buffer, &begin_info) != VK_SUCCESS) {
        return VulkanError{"Failed to begin command buffer", false};
    }

    vkCmdBindPipeline(compute.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute.pipeline);
    vkCmdBindDescriptorSets(compute.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                           compute.pipeline_layout, 0, 1, &compute.descriptor_set, 0, nullptr);

    uint32_t group_count_x = (num_elements + 255) / 256;
    vkCmdDispatch(compute.command_buffer, group_count_x, 1, 1);

    if (vkEndCommandBuffer(compute.command_buffer) != VK_SUCCESS) {
        return VulkanError{"Failed to end command buffer", false};
    }

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &compute.command_buffer;

    auto compute_queue = ctx.device.get_queue(vkb::QueueType::compute).value();
    if (vkQueueSubmit(compute_queue, 1, &submit_info, VK_NULL_HANDLE) != VK_SUCCESS) {
        return VulkanError{"Failed to submit compute work", false};
    }

    if (vkQueueWaitIdle(compute_queue) != VK_SUCCESS) {
        return VulkanError{"Failed to wait for compute queue", false};
    }

    return VulkanError{"", true};
}

int main() {
    VulkanContext ctx{};
    
    auto result = init_vulkan(ctx);
    if (!result.success) {
        std::cerr << "Failed to initialize Vulkan: " << result.message << std::endl;
        return 1;
    }

    // Create test data
    const size_t data_size = 1024;
    std::vector<float> a_data(data_size, 1.0f);  // Array of 1's
    std::vector<float> b_data(data_size, 2.0f);  // Array of 2's
    std::vector<float> result_data(data_size);

    // Create buffers
    Buffer a_buffer{}, b_buffer{}, result_buffer{};
    
    // Create input buffer A
    result = create_host_visible_buffer(ctx, a_buffer, 
        sizeof(float) * data_size, 
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    if (!result.success) {
        std::cerr << "Failed to create buffer A: " << result.message << std::endl;
        cleanup_vulkan(ctx);
        return 1;
    }

    // Create input buffer B
    result = create_host_visible_buffer(ctx, b_buffer, 
        sizeof(float) * data_size, 
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    if (!result.success) {
        std::cerr << "Failed to create buffer B: " << result.message << std::endl;
        destroy_buffer(ctx, a_buffer);
        cleanup_vulkan(ctx);
        return 1;
    }

    // Create result buffer
    result = create_host_visible_buffer(ctx, result_buffer, 
        sizeof(float) * data_size, 
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    if (!result.success) {
        std::cerr << "Failed to create result buffer: " << result.message << std::endl;
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
    if (!result.success) {
        std::cerr << "Failed to setup compute pipeline: " << result.message << std::endl;
        destroy_buffer(ctx, a_buffer);
        destroy_buffer(ctx, b_buffer);
        destroy_buffer(ctx, result_buffer);
        cleanup_vulkan(ctx);
        return 1;
    }

    // Execute compute shader
    result = record_and_submit_compute(ctx, compute, data_size);
    if (!result.success) {
        std::cerr << "Failed to execute compute shader: " << result.message << std::endl;
        cleanup_compute(ctx, compute);
        destroy_buffer(ctx, a_buffer);
        destroy_buffer(ctx, b_buffer);
        destroy_buffer(ctx, result_buffer);
        cleanup_vulkan(ctx);
        return 1;
    }

    // Read back results
    std::memcpy(result_data.data(), result_buffer.mapped_data, result_buffer.size);

    // Verify results
    bool computation_success = true;
    for (size_t i = 0; i < data_size; i++) {
        float expected = a_data[i] + b_data[i];
        if (std::abs(result_data[i] - expected) > 1e-6) {
            std::cerr << "Computation error at index " << i 
                     << ": expected " << expected 
                     << ", got " << result_data[i] << std::endl;
            computation_success = false;
            break;
        }
    }

    if (computation_success) {
        std::cout << "Compute shader execution verified successfully!" << std::endl;
    }

    // Cleanup
    cleanup_compute(ctx, compute);
    destroy_buffer(ctx, a_buffer);
    destroy_buffer(ctx, b_buffer);
    destroy_buffer(ctx, result_buffer);
    cleanup_vulkan(ctx);
    
    return computation_success ? 0 : 1;
}