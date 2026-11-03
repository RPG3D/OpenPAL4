#include "pal4/game_manager_quad_drawer.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IGameManagerQuadRuntime {
public:
    std::uint32_t CurrentDevice() override {
        events.push_back("device");
        return 0x9000u;
    }
    std::uint32_t TextureHandle(const std::uint32_t pointer) override {
        Check(pointer == 0x8000u, "texture handle comes from input pointer");
        events.push_back("texture");
        return 0x7000u;
    }
    void BindTexture(const std::uint32_t stage,
                     const std::uint32_t handle) override {
        Check(stage == 1u && (handle == 0x7000u || handle == 0u),
              "texture stage one is bound then cleared");
        events.push_back(handle == 0u ? "unbind" : "bind");
    }
    std::uint32_t SetDeviceState(const std::uint32_t device,
                                 const std::uint32_t state,
                                 const std::uint32_t value) override {
        Check(device == 0x9000u, "render state uses current D3D device");
        events.push_back("state " + std::to_string(state) + "=" +
                         std::to_string(value));
        return state == 14u && value == 1u ? 0xbeefu : 0u;
    }
    void SetFvf(const std::uint32_t device,
                const std::uint32_t fvf) override {
        Check(device == 0x9000u && fvf == 324u,
              "FVF call uses observed 324 value");
        events.push_back("fvf");
    }
    void DrawPrimitiveUp(
        const std::uint32_t device, const std::uint32_t primitive,
        const std::uint32_t count,
        const std::array<pal4::CeguiRenderVertex, 4>& vertices,
        const std::uint32_t stride) override {
        Check(device == 0x9000u && primitive == 5u &&
                  count == 2u && stride == 28u,
              "draw submits two triangle-strip primitives of stride 28");
        const auto CheckVertex = [&](const std::size_t index,
                                     const float x, const float y,
                                     const float u, const float v) {
            const auto& vertex = vertices[index];
            Check(vertex.x == x && vertex.y == y &&
                      vertex.depth == 0.5f &&
                      vertex.reciprocal_camera_depth == 1.0f &&
                      vertex.colour == 0xaabbccddu &&
                      vertex.u == u && vertex.v == v,
                  "vertex fields match observed position, color and UV");
        };
        CheckVertex(0u, 10.0f, 20.0f, 0.0f, 0.0f);
        CheckVertex(1u, 10.0f, 60.0f, 0.0f, 1.0f);
        CheckVertex(2u, 40.0f, 20.0f, 1.0f, 0.0f);
        CheckVertex(3u, 40.0f, 60.0f, 1.0f, 1.0f);
        events.push_back("draw");
    }
    std::vector<std::string> events;
};

void DisabledPathPreservesHighRegisterBits() {
    Runtime runtime;
    Check(pal4::GameManagerQuadDrawer::Draw(
              0u, 0x12345678u, 0x8000u,
              10.0f, 20.0f, 30.0f, 40.0f, 0xaabbccddu, runtime) ==
              0x12345600u && runtime.events.empty(),
          "disabled path clears only incoming AL and makes no external calls");
}

void DrawOrderAndResult() {
    Runtime runtime;
    Check(pal4::GameManagerQuadDrawer::Draw(
              1u, 0x12345678u, 0x8000u,
              10.0f, 20.0f, 30.0f, 40.0f, 0xaabbccddu, runtime) ==
              0xbeefu,
          "active path returns final render-state result");
    Check(runtime.events == std::vector<std::string>({
              "device", "texture", "bind", "state 15=0", "state 27=1",
              "state 19=5", "state 20=6", "state 7=1", "state 14=0",
              "fvf", "draw", "unbind", "state 7=1", "state 14=1"}),
          "D3D and texture calls follow exact binary order");
    pal4::GameManagerQuadRuntimeStub stub;
    Check(pal4::GameManagerQuadDrawer::Draw(
              1u, 0u, 0x8000u, 0.0f, 0.0f, 0.0f, 0.0f, 0u, stub) == 0u,
          "empty D3D runtime links and returns zero");
}

}  // namespace

int main() {
    DisabledPathPreservesHighRegisterBits();
    DrawOrderAndResult();
    std::cout << "2 game manager quad drawer test cases passed\n";
}
