#include "frame_grabber.h"
#include <algorithm>
#include <cstring>

FrameGrabber::FrameGrabber(uint32_t width, uint32_t height)
    : width_(width), height_(height) {
}

FrameGrabber::~FrameGrabber() {
    cleanup();
}

void FrameGrabber::init(const char* source) {
    // MVP: no actual source. Just generate test pattern.
    // Later: V4L2 or FFmpeg initialization
}

void FrameGrabber::cleanup() {
    // TODO: Release V4L2 or FFmpeg resources
}

bool FrameGrabber::grabFrame(Frame& out_frame) {
    out_frame.width = width_;
    out_frame.height = height_;
    out_frame.stride = width_ * 4;  // RGBA
    
    // Generate test pattern: gradient
    size_t pixels = width_ * height_;
    out_frame.data.resize(pixels * 4);
    
    uint8_t* ptr = out_frame.data.data();
    for (uint32_t y = 0; y < height_; y++) {
        for (uint32_t x = 0; x < width_; x++) {
            uint8_t r = (x * 255) / width_;
            uint8_t g = (y * 255) / height_;
            uint8_t b = ((x + y) * 255) / (width_ + height_);
            uint8_t a = 255;
            
            *ptr++ = r;
            *ptr++ = g;
            *ptr++ = b;
            *ptr++ = a;
        }
    }
    
    frame_count_++;
    return true;
}
