# ObscuraRT Architecture & Design Decisions

## Philosophy

> **"Privacy at the edge, latency on the GPU, simplicity in design."**

This project prioritizes:
1. **Local processing** — zero cloud dependency
2. **GPU acceleration** — real-time performance
3. **Cross-platform** — not locked to NVIDIA
4. **Modular** — each component is independently testable

---

## Core Components

### 1. VulkanContext (`include/vulkan_context.h`)

**Responsibility**: Encapsulate all Vulkan "plumbing."

```cpp
class VulkanContext {
    VkInstance instance_;
    VkPhysicalDevice physical_device_;
    VkDevice device_;
    VkQueue compute_queue_, present_queue_;
    // ...
};
```

**Key features**:
- **Instance creation** with validation layers (debug mode)
- **Device selection**: Prefers discrete GPU, falls back to iGPU
- **Queue family management**: Separate compute & present (can merge)
- **Memory utilities**: `findMemoryType()` for buffer allocation

**Design note**: Vulkan is verbose by design. This abstraction provides sensible defaults while exposing raw handles for advanced usage.

---

### 2. FrameGrabber (`include/frame_grabber.h`)

**Responsibility**: Provide video frames (from camera, file, or IP stream).

```cpp
class FrameGrabber {
    bool grabFrame(Frame& out_frame);
    // Later: OpenCV VideoCapture or FFmpeg
};

struct Frame {
    uint32_t width, height, stride;
    std::vector<uint8_t> data;  // RGBA or YUV420
};
```

**MVP Implementation**:
- Generates a test gradient pattern
- Returns RGBA data (4 bytes/pixel)

**Future integrations**:
- **V4L2**: Linux webcam (low-level, zero-copy potential)
- **FFmpeg**: Remote RTSP, MP4 files
- **OpenCV VideoCapture**: Simplified but higher overhead

**Memory strategy**:
- Ideally: V4L2 mmap → GPU-pinned buffer
- Fallback: CPU allocates, GPU transfers

---

### 3. ComputePipeline (`include/compute_pipeline.h`)

**Responsibility**: Execute GPU compute shaders.

```cpp
class ComputePipeline {
    void processFrame(VkImage input_image, VkImage output_image, uint32_t block_size);
    VkPipeline compute_pipeline_;
    VkDescriptorSet descriptor_sets_[2];  // Double-buffering
};
```

**Vulkan pipeline structure**:
```
Shader Module (SPIR-V)
    ↓
Pipeline Layout (parameters)
    ↓
Compute Pipeline
    ↓
Descriptor Set (image bindings)
    ↓
Command Buffer (recording)
```

**Synchronization**:
- **Memory barriers** between passes (WAR, RAW)
- **Fences** for CPU-GPU sync
- **Semaphores** for queue ordering

**Shader design**:
```glsl
layout(local_size_x = 16, local_size_y = 16) in;
// Each workgroup: 256 threads
// Invocation (i,j) → output pixel[i,j]
```

---

### 4. DisplayPipeline (`include/display_pipeline.h`)

**Responsibility**: Render processed frames to window.

```cpp
class DisplayPipeline {
    bool presentFrame(VkImage frame_image);
    VkSwapchainKHR swapchain_;
    VkRenderPass render_pass_;
};
```

**Vulkan graphics pipeline** (MVP stub):
1. Acquire swapchain image
2. Copy compute output → framebuffer
3. Present to display

**Later phases**:
- Full graphics pipeline (vertex/fragment shaders)
- UI overlay (FPS, status)
- Multiple output formats (H.264 encode)

---

## Data Flow

### Single Frame Lifecycle

```
┌─ Iteration N ──────────────────────────────────────┐
│                                                    │
│  CPU:  FrameGrabber.grabFrame()                   │
│        [allocate CPU buffer]                      │
│           ↓                                        │
│  GPU:  vkQueueSubmit(upload command buffer)      │
│        [copy CPU → GPU]                           │
│           ↓                                        │
│  GPU:  vkQueueSubmit(compute command buffer)     │
│        [pixelation shader]                        │
│        [memory barrier]                           │
│           ↓                                        │
│  GPU:  vkQueueSubmit(graphics command buffer)    │
│        [render output image]                      │
│           ↓                                        │
│  GPU:  vkQueuePresentKHR()                        │
│        [display to screen]                        │
│           ↓                                        │
│  CPU:  vkWaitForFences()                          │
│        [sync before next iteration]               │
│                                                    │
└────────────────────────────────────────────────────┘
```

### Memory Layout (GPU)

```
Device Local Memory
├─ Input Image (VkImage)
│  └─ Format: VK_FORMAT_R8G8B8A8_UNORM (RGBA)
│
├─ Output Image (VkImage)
│  └─ Format: same as input
│
└─ (Optional: Staging Buffer for transfers)

Host Visible Memory
├─ Frame Buffer (CPU → GPU staging)
│
└─ Transfer Buffer (GPU → CPU for encoding)
```

---

## Threading & Concurrency

**Current design**: Single-threaded main loop.

```cpp
while (!should_exit) {
    grabFrame(cpu_buffer);       // CPU: ~5ms
    uploadToGpu(...);            // GPU: async
    runCompute(...);             // GPU: ~10ms
    presentFrame(...);           // GPU: async + display
    waitForGpu(...);             // CPU: blocking
}
```

**Improvements (V2+)**:
- Separate grab/encode thread
- Triple buffering for pipelining
- Lock-free ringbuffer for frame queue

---

## Shader Architecture

### Compute Shader: `pixelation.comp`

```glsl
layout(local_size_x = 16, local_size_y = 16) in;
layout(rgba8, binding = 0) uniform readonly image2D inputImage;
layout(rgba8, binding = 1) uniform writeonly image2D outputImage;

void main() {
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    uint block_size = 16u;
    
    // Sample every N pixels
    ivec2 blockCoord = pixelCoord / int(block_size);
    ivec2 sampleCoord = blockCoord * int(block_size);
    
    vec4 color = imageLoad(inputImage, sampleCoord);
    imageStore(outputImage, pixelCoord, color);
}
```

**Design decisions**:
- **16x16 workgroup size**: Balances cache usage & latency
- **Block-based pixelation**: Block size 16 pixels → secure anonymization
- **Storage images**: Better performance than samplers for compute

**Future shader variants**:
- `gaussian_blur.comp`: Lower-quality (faster) blur
- `face_mask.comp`: Takes face detection output, selective pixelation
- `temporal.comp`: Optical flow-aware smoothing

---

## Error Handling

**Philosophy**: Fail fast, fail loud.

```cpp
try {
    app.init();
} catch (const std::exception& e) {
    std::cerr << "[ERROR] " << e.what() << std::endl;
    return 1;
}
```

**Vulkan validation**:
```cpp
#ifndef NDEBUG
    vkCreateDebugMessengerEXT(instance, &createInfo, nullptr, &messenger);
#endif
```

**Future improvements**:
- Graceful fallback (CPU pixelation if GPU unavailable)
- Frame drop detection (monitor FPS)
- Recovery from context loss (headless rendering)

---

## Performance Targets & Profiling

### Latency Budget (per frame @ 1080p)

```
Frame grab:          2–5 ms   (CPU)
GPU upload:          0–2 ms   (async)
Pixelation shader:   3–8 ms   (GPU)
Graphics render:     1–3 ms   (GPU)
Present:             1–2 ms   (GPU)
CPU sync:            0–1 ms   (vkWaitForFences)
─────────────────────────────
Total:              ~10–15 ms  → 60–100 FPS
```

### Profiling Tools

```bash
# GPU timestamp queries (Vulkan API)
vkCmdWriteTimestamp(cmd_buffer, VK_PIPELINE_STAGE_COMPUTE_BIT, query_pool, 0);

# CPU profiling
perf stat ./obscura_rt

# GPU profiling
renderdoc ./obscura_rt
```

---

## Extensibility

### Adding a New Shader Pass

1. **Write GLSL** (e.g., `face_detect.comp`)
2. **Create pipeline class**:
   ```cpp
   class FaceDetectPipeline { ... };
   ```
3. **Integrate into main loop**:
   ```cpp
   faceDetect->processFrame(inputImage, detectionOutput);
   compute->processFrame(detectionOutput, outputImage);
   ```

### Adding Input Source

1. Inherit from FrameGrabber interface
2. Implement `grabFrame()`
3. Swap in `main.cpp`:
   ```cpp
   frame_grabber_ = std::make_unique<WebcamGrabber>();
   // or
   frame_grabber_ = std::make_unique<FFmpegGrabber>("rtsp://...");
   ```

---

## Known Limitations & TODOs

- [ ] **Display pipeline**: Not fully implemented
- [ ] **GPU memory management**: No explicit VkDeviceMemory pooling
- [ ] **Error recovery**: No fallback if GPU memory exhausted
- [ ] **Face detection**: Currently stubbed; Haar cascade pending
- [ ] **Video encode**: No hardware-accelerated H.264/H.265 output
- [ ] **Temporal filtering**: No motion-aware blur
- [ ] **CLI interface**: Config from command-line args

---

## References

- [Vulkan Spec](https://registry.khronos.org/vulkan/specs/1.3/html/)
- [Khronos Vulkan Samples](https://github.com/KhronosGroup/Vulkan-Samples)
- [Pixelation algorithm](https://en.wikipedia.org/wiki/Pixelation)
- [GDPR/KVKK compliance](https://gdpr-info.eu/)
