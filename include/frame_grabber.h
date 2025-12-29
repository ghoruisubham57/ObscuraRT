#pragma once

#include <cstdint>
#include <vector>

struct Frame {
    uint32_t width;
    uint32_t height;
    uint32_t stride;  // bytes per row
    std::vector<uint8_t> data;  // YUV420 or RGB
    
    size_t getTotalBytes() const { return data.size(); }
};

/**
 * @brief Acquires video frames from webcam or file.
 *
 * Supports:
 * - V4L2 (Linux webcam)
 * - FFmpeg (files, IP cameras)
 *
 * Currently: Mock implementation returning test pattern.
 */
class FrameGrabber {
public:
    explicit FrameGrabber(uint32_t width = 1920, uint32_t height = 1080);
    ~FrameGrabber();
    
    // Non-copyable
    FrameGrabber(const FrameGrabber&) = delete;
    FrameGrabber& operator=(const FrameGrabber&) = delete;
    
    void init(const char* source = nullptr);
    void cleanup();
    
    bool grabFrame(Frame& out_frame);
    
    uint32_t getWidth() const { return width_; }
    uint32_t getHeight() const { return height_; }
    
private:
    uint32_t width_;
    uint32_t height_;
    uint32_t frame_count_ = 0;
    
    // Placeholder for V4L2 or FFmpeg handle
    // For MVP: just generate test pattern
};
