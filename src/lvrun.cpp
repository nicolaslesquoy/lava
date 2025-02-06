#include "lvrun.hpp"

VulkanError setup_compute(VulkanContext& ctx, ComputePipeline& compute,
                          Buffer& a_buffer, Buffer& b_buffer,
                          Buffer& result_buffer)
{
    // Create descriptor set layout
    VkDescriptorSetLayoutBinding bindings[3] = {};
    for(int i = 0; i < 3; i++) {
        bindings[i].binding = i;
        bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    }

    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = 3;
    layout_info.pBindings = bindings;

    if(vkCreateDescriptorSetLayout(ctx.device.device, &layout_info, nullptr,
                                   &compute.descriptor_layout)
       != VK_SUCCESS) {
        return VulkanError{"Failed to create descriptor set layout", false};
    }

    // Create pipeline layout
    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &compute.descriptor_layout;

    if(vkCreatePipelineLayout(ctx.device.device, &pipeline_layout_info, nullptr,
                              &compute.pipeline_layout)
       != VK_SUCCESS) {
        return VulkanError{"Failed to create pipeline layout", false};
    }

    // Create shader module
    VkShaderModule compute_shader;
    auto result = create_shader_module(ctx, "../build/shaders/add.comp.spv",
                                       compute_shader);
    if(!result.success) return result;

    // Create compute pipeline
    VkComputePipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_info.stage.sType
        = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipeline_info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipeline_info.stage.module = compute_shader;
    pipeline_info.stage.pName = "main";
    pipeline_info.layout = compute.pipeline_layout;

    if(vkCreateComputePipelines(ctx.device.device, VK_NULL_HANDLE, 1,
                                &pipeline_info, nullptr, &compute.pipeline)
       != VK_SUCCESS) {
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

    if(vkCreateDescriptorPool(ctx.device.device, &pool_info, nullptr,
                              &compute.descriptor_pool)
       != VK_SUCCESS) {
        return VulkanError{"Failed to create descriptor pool", false};
    }

    // Allocate descriptor set
    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = compute.descriptor_pool;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &compute.descriptor_layout;

    if(vkAllocateDescriptorSets(ctx.device.device, &alloc_info,
                                &compute.descriptor_set)
       != VK_SUCCESS) {
        return VulkanError{"Failed to allocate descriptor sets", false};
    }

    // Update descriptor set
    VkDescriptorBufferInfo buffer_info[3]
        = {{a_buffer.buffer, 0, VK_WHOLE_SIZE},
           {b_buffer.buffer, 0, VK_WHOLE_SIZE},
           {result_buffer.buffer, 0, VK_WHOLE_SIZE}};

    VkWriteDescriptorSet descriptor_writes[3] = {};
    for(int i = 0; i < 3; i++) {
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
    command_pool_info.queueFamilyIndex
        = ctx.device.get_queue_index(vkb::QueueType::compute).value();

    if(vkCreateCommandPool(ctx.device.device, &command_pool_info, nullptr,
                           &compute.command_pool)
       != VK_SUCCESS) {
        return VulkanError{"Failed to create command pool", false};
    }

    VkCommandBufferAllocateInfo command_buffer_info{};
    command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_info.commandPool = compute.command_pool;
    command_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_info.commandBufferCount = 1;

    if(vkAllocateCommandBuffers(ctx.device.device, &command_buffer_info,
                                &compute.command_buffer)
       != VK_SUCCESS) {
        return VulkanError{"Failed to allocate command buffers", false};
    }

    return VulkanError{"", true};
}

VulkanError record_and_submit_compute(VulkanContext& ctx,
                                      ComputePipeline& compute,
                                      uint32_t num_elements)
{
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if(vkBeginCommandBuffer(compute.command_buffer, &begin_info)
       != VK_SUCCESS) {
        return VulkanError{"Failed to begin command buffer", false};
    }

    vkCmdBindPipeline(compute.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                      compute.pipeline);
    vkCmdBindDescriptorSets(
        compute.command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
        compute.pipeline_layout, 0, 1, &compute.descriptor_set, 0, nullptr);

    uint32_t group_count_x = (num_elements + 255) / 256;
    vkCmdDispatch(compute.command_buffer, group_count_x, 1, 1);

    if(vkEndCommandBuffer(compute.command_buffer) != VK_SUCCESS) {
        return VulkanError{"Failed to end command buffer", false};
    }

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &compute.command_buffer;

    auto compute_queue = ctx.device.get_queue(vkb::QueueType::compute).value();
    if(vkQueueSubmit(compute_queue, 1, &submit_info, VK_NULL_HANDLE)
       != VK_SUCCESS) {
        return VulkanError{"Failed to submit compute work", false};
    }

    if(vkQueueWaitIdle(compute_queue) != VK_SUCCESS) {
        return VulkanError{"Failed to wait for compute queue", false};
    }

    return VulkanError{"", true};
}