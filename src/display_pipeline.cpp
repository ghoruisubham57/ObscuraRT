#include "display_pipeline.h"
#include "vulkan_context.h"
#include <iostream>
#include <vector>
#include <stdexcept>

DisplayPipeline::DisplayPipeline(VulkanContext* vk_ctx)
    : vk_ctx_(vk_ctx) {
}

DisplayPipeline::~DisplayPipeline() {
    cleanup();
}

void DisplayPipeline::init(uint32_t width, uint32_t height, const char* window_title) {
    createWindow(width, height, window_title);
    // TODO: Surface, swapchain, renderpass, framebuffers, graphics pipeline, etc.
    std::cout << "[Display] Pipeline initialized (stub)" << std::endl;
}

void DisplayPipeline::cleanup() {
    VkDevice device = vk_ctx_->getDevice();
    
    if (device != VK_NULL_HANDLE) {
        if (in_flight_fence_ != VK_NULL_HANDLE) {
            vkDestroyFence(device, in_flight_fence_, nullptr);
        }
        if (render_finished_semaphore_ != VK_NULL_HANDLE) {
            vkDestroySemaphore(device, render_finished_semaphore_, nullptr);
        }
        if (image_available_semaphore_ != VK_NULL_HANDLE) {
            vkDestroySemaphore(device, image_available_semaphore_, nullptr);
        }
    }
    
    if (window_) {
        glfwDestroyWindow(window_);
        glfwTerminate();
    }
}

void DisplayPipeline::createWindow(uint32_t width, uint32_t height, const char* title) {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }
    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    
    window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window_) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }
}

void DisplayPipeline::createSurface() {
    // TODO: VkSurfaceKHR from GLFW window
}

bool DisplayPipeline::presentFrame(VkImage frame_image) {
    if (shouldClose()) {
        return false;
    }
    
    glfwPollEvents();
    // TODO: Actual presentation logic
    return true;
}

bool DisplayPipeline::shouldClose() const {
    return window_ && glfwWindowShouldClose(window_);
}
