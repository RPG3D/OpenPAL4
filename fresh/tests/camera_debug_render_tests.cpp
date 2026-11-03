#include "pal4/camera_debug_render.h"

#include <bit>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::ICameraDebugRenderRuntime {
public:
    std::vector<std::string> calls;
    std::vector<std::vector<pal4::CameraDebugVertex>> batches;
    bool transform_succeeds = true;
    std::uint32_t saved_state = 0x55u;

    std::uint32_t Device() override {
        calls.emplace_back("device");
        return 0x8000u;
    }
    std::uint32_t GetD3DRenderState(std::uint32_t device,
                                    std::uint32_t state) override {
        Check(device == 0x8000u && state == 28u,
              "D3D render state query");
        calls.emplace_back("get D3D");
        return saved_state;
    }
    std::uint32_t SetD3DRenderState(std::uint32_t device,
                                    std::uint32_t state,
                                    std::uint32_t value) override {
        Check(device == 0x8000u && state == 28u &&
                  (value == 0u || value == saved_state),
              "D3D render state write");
        calls.emplace_back(value == 0u ? "disable D3D" : "restore D3D");
        return value == 0u ? 0u : 0x1234u;
    }
    void DrawBoundingBox(pal4::CameraDataState& camera,
                         std::uint32_t box_offset,
                         std::uint32_t color,
                         std::uint32_t transform_offset) override {
        Check(camera.At(0x1ac) != 0u && box_offset == 0x230u &&
                  color == 0xc7f93737u && transform_offset == 0xa8u,
              "debug bounding box operands");
        calls.emplace_back("bounding box");
    }
    void SetRenderState(std::uint32_t state,
                        std::uint32_t value) override {
        calls.emplace_back("RW:" + std::to_string(state) + ":" +
                           std::to_string(value));
    }
    bool Transform(std::span<const pal4::CameraDebugVertex> vertices,
                   std::uint32_t flags) override {
        Check(flags == 2u &&
                  (vertices.size() == 8u || vertices.size() == 6u),
              "immediate-mode transform operands");
        batches.emplace_back(vertices.begin(), vertices.end());
        calls.emplace_back("transform:" +
                           std::to_string(vertices.size()));
        return transform_succeeds;
    }
    void RenderIndexed(std::uint32_t primitive,
                       std::span<const std::uint16_t> indices) override {
        Check(primitive == 1u &&
                  (indices.size() == 8u || indices.size() == 6u),
              "indexed debug primitive operands");
        for (std::size_t i = 0; i < indices.size(); ++i)
            Check(indices[i] == i, "IDA debug index table");
        calls.emplace_back("render:" + std::to_string(indices.size()));
    }
    void End() override { calls.emplace_back("end"); }
};

pal4::CameraDataState MakeCamera() {
    pal4::CameraDataState camera;
    for (std::size_t i = 0; i < 8u; ++i) {
        const auto offset = 0x1c0u + i * 12u;
        camera.At(offset) = std::bit_cast<std::uint32_t>(
            static_cast<float>(i * 3u));
        camera.At(offset + 4u) = std::bit_cast<std::uint32_t>(
            static_cast<float>(i * 3u + 1u));
        camera.At(offset + 8u) = std::bit_cast<std::uint32_t>(
            static_cast<float>(i * 3u + 2u));
    }
    camera.At(0x220u) = 0x11223344u;
    camera.At(0x224u) = 0x55667788u;
    camera.At(0x228u) = 0x99aabbccu;
    camera.At(0x22cu) = 0xddeeff00u;
    camera.At(0x74u) = std::bit_cast<std::uint32_t>(1.0f);
    camera.At(0x78u) = std::bit_cast<std::uint32_t>(2.0f);
    camera.At(0x7cu) = std::bit_cast<std::uint32_t>(3.0f);
    return camera;
}

void RenderCases() {
    Runtime runtime;
    pal4::CameraDebugRenderer renderer(runtime);
    auto camera = MakeCamera();
    Check(renderer.Render(camera) == 0u && runtime.calls.empty(),
          "disabled debug overlays make no external calls");

    camera.At(0x1acu) = 1u;
    Check(renderer.Render(camera) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "device", "get D3D", "disable D3D", "bounding box",
                  "RW:7:1", "RW:1:0", "transform:8", "render:8",
                  "end", "RW:6:1", "RW:7:2", "restore D3D"},
          "frustum overlay saves, draws and restores render state");
    Check(runtime.batches.size() == 1u &&
              runtime.batches[0][0].position ==
                  std::array<float, 3>{0.0f, 1.0f, 2.0f} &&
              runtime.batches[0][7].position ==
                  std::array<float, 3>{21.0f, 22.0f, 23.0f} &&
              runtime.batches[0][0].argb == 0x11443322u &&
              runtime.batches[0][1].argb == 0x11443322u &&
              runtime.batches[0][2].argb == 0x55887766u &&
              runtime.batches[0][7].argb == 0xdd00ffeeu,
          "frustum vertices use eight positions and paired RGBA colors");

    runtime.calls.clear();
    runtime.batches.clear();
    camera.At(0x1acu) = 0u;
    camera.At(0x1b0u) = 1u;
    Check(renderer.Render(camera) == 0x1234u &&
              runtime.calls == std::vector<std::string>{
                  "device", "get D3D", "disable D3D", "RW:7:1",
                  "RW:1:0", "transform:6", "render:6", "end",
                  "RW:6:1", "RW:7:2", "restore D3D"},
          "axis overlay returns final D3D restore result");
    const auto& axes = runtime.batches[0];
    Check(axes[0].position == std::array<float, 3>{1, 2, 3} &&
              axes[1].position == std::array<float, 3>{21, 2, 3} &&
              axes[2].position == std::array<float, 3>{1, 2, 3} &&
              axes[3].position == std::array<float, 3>{1, 22, 3} &&
              axes[4].position == std::array<float, 3>{1, 2, 3} &&
              axes[5].position == std::array<float, 3>{1, 2, 23} &&
              axes[0].argb == 0xffff4545u &&
              axes[2].argb == 0xff45ff45u &&
              axes[4].argb == 0xff4545ffu,
          "target axes use three 20-unit colored lines");

    runtime.calls.clear();
    runtime.batches.clear();
    runtime.transform_succeeds = false;
    camera.At(0x1acu) = 1u;
    Check(renderer.Render(camera) == 0x1234u &&
              runtime.batches.size() == 2u &&
              runtime.calls == std::vector<std::string>{
                  "device", "get D3D", "disable D3D", "bounding box",
                  "RW:7:1", "RW:1:0", "transform:8", "RW:6:1",
                  "RW:7:2", "restore D3D", "device", "get D3D",
                  "disable D3D", "RW:7:1", "RW:1:0", "transform:6",
                  "RW:6:1", "RW:7:2", "restore D3D"},
          "failed transforms skip indexed draw and still restore both states");
}

}  // namespace

int main() {
    try {
        RenderCases();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
