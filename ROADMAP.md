# Development Roadmap

## Phase 1: MVP (Weeks 1-2) — Current ✓

**Goals**: Vulkan pipeline + pixelation shader working end-to-end

- [x] Project structure & CMake setup
- [x] Vulkan context (instance, device, queues)
- [x] Frame grabber (test pattern MVP)
- [x] Compute pipeline skeleton
- [x] Pixelation shader (GLSL)
- [ ] GPU memory management (buffers, images)
- [ ] Frame upload/download
- [ ] Display pipeline (window, swapchain, presentation)
- [ ] End-to-end frame processing
- [ ] Real-time FPS monitoring

**Success criteria**:
- Pixelated test pattern displays in window
- 30+ FPS @ 1080p on discrete GPU
- No validation errors

**Deliverables**:
- `./build/obscura_rt` executable
- Validation layers clean
- README + architecture docs

---

## Phase 2: Real Input (Weeks 3-4)

**Goals**: Live webcam / video file input

### 2.1 V4L2 Webcam Integration

```cpp
class WebcamGrabber : public FrameGrabber {
    int fd_;  // V4L2 device
    v4l2_buffer buffers_[4];  // Ring buffer
    
    bool grabFrame(Frame& out_frame) override;
};
```

**Tasks**:
- [ ] V4L2 device enumeration
- [ ] Buffer pool allocation
- [ ] Zero-copy mmap to GPU (if supported)
- [ ] Color space conversion (YUYV → RGBA)
- [ ] Error handling (device disconnect)

### 2.2 Format Conversion (GPU)

```glsl
// Convert YUV420 → RGBA on GPU
// Faster than CPU, frees CPU cycles
```

**Tasks**:
- [ ] YUV to RGB shader
- [ ] Pipeline: V4L2 (YUV) → GPU → Pixelation
- [ ] Format negotiation (supported resolutions)

### 2.3 FFmpeg Support (Optional)

```cpp
class FFmpegGrabber : public FrameGrabber {
    AVFormatContext* fmt_ctx_;
    // Stream from RTSP, files, HLS
};
```

---

## Phase 3: Face Detection & Selective Anonymization (Weeks 5-6)

**Goals**: Detect faces, blur only those regions

### 3.1 Lightweight Face Detector

**Options** (in order of priority):
1. **Haar Cascade** (OpenCV): Fast, built-in
2. **YOLO Nano**: Better accuracy
3. **MediaPipe Face**: Cloud-based (not edge!)

```cpp
class FaceDetector {
    std::vector<cv::Rect> detectFaces(const cv::Mat& frame);
    // Returns bounding boxes
};
```

**Tasks**:
- [ ] Load pretrained model
- [ ] Multi-face support
- [ ] Bounding box filtering (min size, NMS)

### 3.2 Selective Pixelation Shader

```glsl
// face_pixelate.comp
// Input: frame, face boxes
// Output: frame with faces pixelated

uniform sampler2D faceBoxes;  // Mask texture

void main() {
    if (isFacePixel()) {
        color = pixelate(color);
    }
}
```

**Tasks**:
- [ ] Rasterize face bounding boxes to texture
- [ ] Modify pixelation shader to use mask
- [ ] Temporal smoothing (avoid flicker)

### 3.3 Temporal Consistency

```cpp
// Track face positions across frames
// Smooth region with optical flow
```

---

## Phase 4: Video Encoding & Advanced Features (Weeks 7-8)

**Goals**: Save anonymized video, optimize for edge deployment

### 4.1 Hardware-Accelerated Encoding

```cpp
class VideoEncoder {
    VkImage output_;
    // NVENC / VA-API / VideoToolbox
    
    void encodeFrame(const Frame& frame);
    void writeMKV(const std::string& path);
};
```

**Tasks**:
- [ ] H.264 / H.265 encoding (GPU-accelerated)
- [ ] Bitrate control (adaptive)
- [ ] MKV / MP4 container
- [ ] Audio support (pass-through or remove)

### 4.2 CLI & Configuration

```bash
./obscura_rt \
    --input /dev/video0 \
    --output anon_video.mkv \
    --mode face-blur \
    --block-size 16 \
    --fps 30 \
    --resolution 1920x1080
```

**Tasks**:
- [ ] Argument parsing (getopt or CLI11)
- [ ] YAML/JSON config file
- [ ] Profile presets (fast, quality, secure)

### 4.3 Performance Optimization

**Targets**:
- 4K @ 30 FPS on RTX 4070
- 1080p @ 60 FPS on RTX 3060
- 720p @ 30 FPS on Intel iGPU

**Tasks**:
- [ ] GPU timestamp profiling
- [ ] Shader optimization (register usage)
- [ ] Dynamic resolution scaling
- [ ] Multi-GPU support

---

## Phase 5: Advanced Privacy Features (Weeks 9-10)

**Goals**: GDPR / KVKK compliance, audit trail

### 5.1 Selective Redaction

```
Modes:
- face-pixelate: Block faces
- face-black:    Black out faces
- plate-blur:    License plates
- full-frame:    Entire frame (test only)
```

**Tasks**:
- [ ] License plate detection (ALPR)
- [ ] Body blur (if full privacy needed)
- [ ] Timestamp redaction

### 5.2 Metadata Handling

```cpp
class MetadataHandler {
    void stripExif(const Frame& frame);
    void addWatermark(Frame& frame);
    void logProcessing(const AuditEntry& entry);
};
```

**Tasks**:
- [ ] Remove EXIF / metadata
- [ ] Add processing timestamp
- [ ] Audit logging (JSON)
- [ ] Chain-of-custody proof

### 5.3 Performance Monitoring

```
Real-time dashboard:
- Frame rate (FPS)
- GPU load (%)
- Memory usage (MB)
- Detection rate (faces/frame)
- Encoding bitrate
```

---

## Phase 6: Deployment & Hardening (Weeks 11-12)

**Goals**: Production-ready, edge-optimized

### 6.1 Containerization

```dockerfile
FROM nvidia/cuda:12.1-runtime-ubuntu22.04
RUN apt-get install -y vulkan-loader libglfw3
COPY build/obscura_rt /usr/bin/
CMD ["obscura_rt"]
```

**Tasks**:
- [ ] Docker image
- [ ] Kubernetes manifests
- [ ] Edge runtime (NVIDIA Jetson, etc.)

### 6.2 Error Recovery & Monitoring

```cpp
class ErrorHandler {
    void handleGPULoss();      // Fallback to CPU
    void handleMemoryError();  // Degrade quality
    void handleTimeout();      // Frame drop
};
```

**Tasks**:
- [ ] Graceful degradation
- [ ] Sentry / monitoring integration
- [ ] Health check API
- [ ] Automatic restart on crash

### 6.3 Security Hardening

**Tasks**:
- [ ] Input validation (buffer overflow checks)
- [ ] Memory safety (address sanitizer in CI)
- [ ] GPU context isolation
- [ ] Secure deletion of frames in memory

---

## Testing & Quality Assurance

### Unit Tests (Throughout)

```cpp
TEST(FrameGrabber, GradientGeneration) {
    FrameGrabber grabber(640, 480);
    Frame f;
    ASSERT_TRUE(grabber.grabFrame(f));
    ASSERT_EQ(f.width, 640);
}
```

### Integration Tests

```bash
./test_pipeline.sh
# - Load video
# - Process frames
# - Compare output
# - Check FPS
```

### Compliance Testing

```
- KVKK: Personal data erasure
- GDPR: Right to be forgotten
- Performance: SLA targets
- Security: Vulnerability scan
```

---

## Stretch Goals (Optional)

- [ ] Web UI (HTTP API + React dashboard)
- [ ] Multi-stream processing (multiple videos)
- [ ] Distributed processing (CPU + multi-GPU)
- [ ] Machine learning-based face quality assessment
- [ ] Integration with CCTV management systems

---

## Timeline Summary

```
Week 1-2:  MVP + pixelation                 ← Current
Week 3-4:  Webcam + format conversion
Week 5-6:  Face detection + selective blur
Week 7-8:  Encoding + CLI
Week 9-10: Privacy features + metadata
Week 11-12: Deployment + hardening

Total: ~3 months for production-ready MVP
```

---

## Key Decisions & Trade-offs

| Decision | Rationale | Alternative |
|----------|-----------|-------------|
| Vulkan | Cross-GPU, low latency | CUDA (vendor lock), OpenGL (no compute) |
| Pixelation | Fast, secure enough for GDPR | Gaussian blur (slower), AES (overkill) |
| Haar cascade | Quick to implement | YOLO (better but slower initially) |
| GLFW window | Simple, cross-platform | Qt, GTK (heavier) |
| CMake | Standard, portable | Meson, Bazel (overkill for MVP) |

---

## Contributing

- **Code review**: Every PR needs 2 approvals
- **Testing**: 80%+ coverage
- **Docs**: Update README for new features
- **Commits**: "feat:", "fix:", "docs:" prefixes

---

## Questions?

See [ARCHITECTURE.md](ARCHITECTURE.md) for design deep-dives.

