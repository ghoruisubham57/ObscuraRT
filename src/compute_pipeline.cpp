#include "compute_pipeline.h"
#include "vulkan_context.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <stdexcept>

// Helper to load SPIR-V shader
static std::vector<uint32_t> readShaderFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open shader file: " + filename);
    }
    
    size_t fileSize = (size_t)file.tellg();
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
    
    file.seekg(0);
    file.read((char*)buffer.data(), fileSize);
    file.close();
    
    return buffer;
}

ComputePipeline::ComputePipeline(VulkanContext* vk_ctx)
    : vk_ctx_(vk_ctx) {
}

ComputePipeline::~ComputePipeline() {
    cleanup();
}

void ComputePipeline::init(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;
    
    createShaderModule();
    createDescriptorSetLayout();
    createPipelineLayout();
    createComputePipeline();
    createDescriptorPool();
    allocateDescriptorSets();
    createCommandBuffer();
    createSynchronization();
    
    std::cout << "[Compute] Pipeline initialized (" << width_ << "x" << height_ << ")" << std::endl;
}

void ComputePipeline::cleanup() {
    VkDevice device = vk_ctx_->getDevice();
    
    if (compute_fence_ != VK_NULL_HANDLE) {
        vkDestroyFence(device, compute_fence_, nullptr);
    }
    if (descriptor_pool_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, descriptor_pool_, nullptr);
    }
    if (compute_pipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, compute_pipeline_, nullptr);
    }
    if (pipeline_layout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, pipeline_layout_, nullptr);
    }
    if (descriptor_set_layout_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, descriptor_set_layout_, nullptr);
    }
    if (compute_shader_ != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device, compute_shader_, nullptr);
    }
}

void ComputePipeline::createShaderModule() {
    auto code = readShaderFile("shaders/pixelation.comp.spv");
    
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size() * sizeof(uint32_t);
    createInfo.pCode = code.data();
    
    if (vkCreateShaderModule(vk_ctx_->getDevice(), &createInfo, nullptr, &compute_shader_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create compute shader module");
    }
}

void ComputePipeline::createDescriptorSetLayout() {
    // Two storage images: input and output
    VkDescriptorSetLayoutBinding bindings[2];
    
    // Input image
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    bindings[0].pImmutableSamplers = nullptr;
    
    // Output image
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    bindings[1].pImmutableSamplers = nullptr;
    
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 2;
    layoutInfo.pBindings = bindings;
    
    if (vkCreateDescriptorSetLayout(vk_ctx_->getDevice(), &layoutInfo, nullptr, &descriptor_set_layout_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout");
    }
}

void ComputePipeline::createPipelineLayout() {
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptor_set_layout_;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    
    if (vkCreatePipelineLayout(vk_ctx_->getDevice(), &pipelineLayoutInfo, nullptr, &pipeline_layout_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout");
    }
}

void ComputePipeline::createComputePipeline() {
    VkPipelineShaderStageCreateInfo shaderStageInfo{};
    shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStageInfo.module = compute_shader_;
    shaderStageInfo.pName = "main";
    
    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = pipeline_layout_;
    pipelineInfo.stage = shaderStageInfo;
    
    if (vkCreateComputePipelines(vk_ctx_->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &compute_pipeline_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create compute pipeline");
    }
}

void ComputePipeline::createDescriptorPool() {
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    poolSize.descriptorCount = 2 * 2;  // 2 sets, 2 images each
    
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 2;
    
    if (vkCreateDescriptorPool(vk_ctx_->getDevice(), &poolInfo, nullptr, &descriptor_pool_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool");
    }
}

void ComputePipeline::allocateDescriptorSets() {
    VkDescriptorSetLayout layouts[2] = {descriptor_set_layout_, descriptor_set_layout_};
    
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptor_pool_;
    allocInfo.descriptorSetCount = 2;
    allocInfo.pSetLayouts = layouts;
    
    if (vkAllocateDescriptorSets(vk_ctx_->getDevice(), &allocInfo, descriptor_sets_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor sets");
    }
}

void ComputePipeline::createCommandBuffer() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = vk_ctx_->getCommandPool();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    
    if (vkAllocateCommandBuffers(vk_ctx_->getDevice(), &allocInfo, &compute_command_buffer_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffer");
    }
}

void ComputePipeline::createSynchronization() {
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;  // Start signaled
    
    if (vkCreateFence(vk_ctx_->getDevice(), &fenceInfo, nullptr, &compute_fence_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create fence");
    }
}

void ComputePipeline::processFrame(VkImage input_image, VkImage output_image, uint32_t block_size) {
    // MVP stub: this will be fully implemented
    // For now, just signal that compute is done
}

VkDescriptorSet ComputePipeline::getDescriptorSet(uint32_t frame_index) const {
    return descriptor_sets_[frame_index % 2];
}
