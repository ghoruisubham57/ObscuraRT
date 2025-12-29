#include "vulkan_context.h"
#include "frame_grabber.h"
#include "compute_pipeline.h"
#include "display_pipeline.h"
#include <iostream>
#include <chrono>

class ObscuraRT {
public:
    ObscuraRT() = default;
    ~ObscuraRT() = default;
    
    void init() {
        std::cout << "[ObscuraRT] Initializing..." << std::endl;
        
        vk_ctx_ = std::make_unique<VulkanContext>();
        vk_ctx_->init();
        
        frame_grabber_ = std::make_unique<FrameGrabber>(1920, 1080);
        frame_grabber_->init();
        
        compute_pipeline_ = std::make_unique<ComputePipeline>(vk_ctx_.get());
        compute_pipeline_->init(1920, 1080);
        
        display_pipeline_ = std::make_unique<DisplayPipeline>(vk_ctx_.get());
        display_pipeline_->init(1920, 1080, "ObscuraRT - Real-time Video Anonymization");
    }
    
    void run() {
        std::cout << "[ObscuraRT] Starting main loop..." << std::endl;
        
        auto start = std::chrono::high_resolution_clock::now();
        int frame_count = 0;
        
        while (!display_pipeline_->shouldClose()) {
            // Grab frame
            Frame frame;
            if (!frame_grabber_->grabFrame(frame)) {
                break;
            }
            
            // TODO: Upload frame to GPU, run compute shader, download result
            
            // Display
            // For now, just update window (stub)
            frame_count++;
            
            // FPS counter every 30 frames
            if (frame_count % 30 == 0) {
                auto now = std::chrono::high_resolution_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
                double fps = frame_count / (double)elapsed;
                std::cout << "[FPS] " << fps << std::endl;
            }
        }
        
        std::cout << "[ObscuraRT] Loop ended. Total frames: " << frame_count << std::endl;
    }
    
    void cleanup() {
        std::cout << "[ObscuraRT] Cleaning up..." << std::endl;
        
        display_pipeline_.reset();
        compute_pipeline_.reset();
        frame_grabber_.reset();
        vk_ctx_.reset();
    }
    
private:
    std::unique_ptr<VulkanContext> vk_ctx_;
    std::unique_ptr<FrameGrabber> frame_grabber_;
    std::unique_ptr<ComputePipeline> compute_pipeline_;
    std::unique_ptr<DisplayPipeline> display_pipeline_;
};

int main() {
    try {
        ObscuraRT app;
        app.init();
        app.run();
        app.cleanup();
        
        std::cout << "[ObscuraRT] Shutdown complete" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << std::endl;
        return 1;
    }
}
