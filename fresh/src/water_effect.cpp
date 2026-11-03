#include "pal4/water_effect.h"

#include <algorithm>
#include <cmath>
#include <new>

namespace pal4 {

void WaterEffectRuntimeStub::Reset(WaterEffectState&) {}
bool WaterEffectRuntimeStub::CheckPixelShader(std::uint32_t) { return false; }
bool WaterEffectRuntimeStub::CheckVertexShader(std::uint32_t) { return false; }
std::int32_t WaterEffectRuntimeStub::ScreenWidth() { return 0; }
std::int32_t WaterEffectRuntimeStub::ScreenHeight() { return 0; }
void WaterEffectRuntimeStub::SetInteger(
    std::uint32_t, std::string_view, std::int32_t) {}

std::uint8_t WaterEffect::Enable(WaterEffectState& state) {
    if (state.loaded == 0u) return state.loaded;
    const auto previous = state.enabled;
    if (previous == 0u) {
        state.time = 0.0f;
        state.enabled = 1u;
    }
    return previous;
}

std::uint32_t WaterEffect::Apply(
    const WaterEffectState& state, std::uint32_t input,
    IScreenEffectRuntime& script_runtime, IWaterEffectRuntime& water_runtime) {
    if (state.loaded == 0u || state.enabled == 0u || input == 0u ||
        script_runtime.ReadByte(input) == 0u) return 0u;
    script_runtime.SelectTechnique(state.script, "techWater");
    water_runtime.SetInteger(state.script, "halfWidth",
                             water_runtime.ScreenWidth() / 2);
    water_runtime.SetInteger(state.script, "halfHeight",
                             water_runtime.ScreenHeight() / 2);
    script_runtime.ExecuteTechnique(state.script, input, 0u);
    return input;
}

std::uint8_t WaterEffect::Render(
    const WaterEffectState& state, IScreenEffectRuntime& runtime) {
    if (state.loaded == 0u) return state.loaded;
    if (state.enabled == 0u) return state.enabled;
    return static_cast<std::uint8_t>(runtime.InvokeRenderSlot(state.script, 276u));
}

std::uint8_t WaterEffect::SetParameter(
    const WaterEffectState& state, std::uint32_t value,
    IScreenEffectRuntime& runtime) {
    if (state.loaded == 0u) return state.loaded;
    if (state.enabled == 0u) return state.enabled;
    return static_cast<std::uint8_t>(runtime.InvokeParameterSlot(state.script, value));
}

std::uint8_t WaterEffect::StopRendering(
    const WaterEffectState& state, IScreenEffectRuntime& runtime) {
    if (state.loaded == 0u) return state.loaded;
    if (state.enabled == 0u) return state.enabled;
    return static_cast<std::uint8_t>(runtime.InvokeRenderSlot(state.script, 272u));
}

bool WaterEffect::Load(
    WaterEffectState& state, std::uint32_t message_box_type,
    std::uint32_t environment_word, IScreenEffectRuntime& script_runtime,
    IWaterEffectRuntime& water_runtime) {
    water_runtime.Reset(state);
    if (!water_runtime.CheckPixelShader(0xffff0101u) ||
        !water_runtime.CheckVertexShader(0xfffe0101u)) return false;

    script_runtime.ProcessPackageScript();
    try {
        const auto file = script_runtime.OpenFile(
            "gamedata\\2D\\ScreenEffect\\WaterScreen\\Water.fx", "r");
        if (file == 0u) {
            script_runtime.CleanupScriptEnvironment(environment_word);
            return false;
        }
        script_runtime.SeekFile(file, 0u, 2u);
        const auto bytes = script_runtime.FileSize(file);
        script_runtime.SeekFile(file, 0u, 0u);
        const auto buffer = script_runtime.AllocateBuffer(bytes);
        if (buffer == 0u) throw std::bad_alloc();
        script_runtime.ReadFile(buffer, bytes, 1u, file);
        script_runtime.CloseFile(file);

        std::uint32_t error = 0u;
        const auto device = script_runtime.CurrentD3DDevice();
        const auto result = script_runtime.LoadScript(
            device, buffer, bytes, 0u, 0u, 0x104u, 0u,
            state.script, error);
        state.enabled = 0u;
        if (result == 0u) {
            state.loaded = 1u;
        } else {
            state.loaded = 0u;
            state.script = 0u;
            script_runtime.ShowError(script_runtime.ErrorText(error, "Error"),
                                     message_box_type);
            script_runtime.ReleaseEnvironmentOnLoadError(environment_word);
        }
        script_runtime.FreeBuffer(buffer);
        const auto loaded = state.loaded != 0u;
        script_runtime.CleanupScriptEnvironment(environment_word);
        return loaded;
    } catch (...) {
        script_runtime.CleanupScriptEnvironment(environment_word);
        throw;
    }
}

std::uint8_t WaterEffect::Cleanup(
    WaterEffectState& state, IScreenEffectRuntime& runtime) {
    if (state.loaded == 0u) return state.loaded;
    const auto script = state.script;
    state.loaded = 0u;
    state.enabled = 0u;
    if (script == 0u) return 0u;
    const auto result = runtime.ReleaseScript(script);
    state.script = 0u;
    return static_cast<std::uint8_t>(result);
}

std::uint8_t WaterEffect::Advance(WaterEffectState& state, double delta) {
    if (state.loaded == 0u) return state.loaded;
    if (state.enabled == 0u) return state.enabled;
    state.time = static_cast<float>(static_cast<double>(state.time) + delta);
    return state.enabled;
}

void WaterMeshRuntimeStub::BindRaster(std::uint32_t) {}
void WaterMeshRuntimeStub::SetRenderState(std::uint32_t, std::uint32_t) {}
void WaterMeshRuntimeStub::SetFvf(std::uint32_t) {}
void WaterMeshRuntimeStub::DrawTriangleStrip(
    const WaterVertex*, std::uint32_t, std::uint32_t) {}
std::uint32_t WaterMeshRuntimeStub::RestoreState(
    std::uint32_t, std::uint32_t) { return 0u; }

std::uint32_t WaterMeshRenderer::Render(
    const WaterEffectState& state, std::uint32_t raster,
    float left, float top, float width, float height,
    std::uint32_t color, IWaterMeshRuntime& runtime) {
    if (state.loaded == 0u) return 0u;
    if (vertices_.size() != 48u * 65u * 2u) {
        vertices_.resize(48u * 65u * 2u);
    }
    constexpr double kHorizontalStep = 1.0 / 64.0;
    constexpr double kVerticalStep = 1.0 / 48.0;
    for (std::size_t row = 0; row < 48u; ++row) {
        const float y0 = static_cast<float>(top + row * height * kVerticalStep);
        const float y1 = static_cast<float>(top + (row + 1u) * height * kVerticalStep);
        for (std::size_t column = 0; column < 65u; ++column) {
            const float x = static_cast<float>(left + column * width * kHorizontalStep);
            const float u = static_cast<float>(column * kHorizontalStep);
            for (std::size_t side = 0; side < 2u; ++side) {
                const float y = side == 0u ? y0 : y1;
                auto& vertex = vertices_[(row * 65u + column) * 2u + side];
                const double wave_x = static_cast<double>(x) +
                    std::sin(static_cast<double>(x) + state.time) * 3.0;
                const double wave_y = static_cast<double>(y) +
                    std::cos(static_cast<double>(y) + state.time) * 3.0;
                vertex.x = x == left ? left : x == width ? width :
                    static_cast<float>(std::clamp(wave_x,
                        static_cast<double>(left), static_cast<double>(width)));
                vertex.y = y == top ? top : y == height ? height :
                    static_cast<float>(std::clamp(wave_y,
                        static_cast<double>(top), static_cast<double>(height)));
                vertex.z = 0.5f;
                vertex.color = color;
                vertex.u = u;
                vertex.v = static_cast<float>((row + side) * kVerticalStep);
            }
        }
    }
    runtime.BindRaster(raster);
    runtime.SetRenderState(15u, 0u);
    runtime.SetRenderState(27u, 1u);
    runtime.SetRenderState(19u, 5u);
    runtime.SetRenderState(20u, 6u);
    runtime.SetRenderState(7u, 1u);
    runtime.SetRenderState(14u, 0u);
    runtime.SetFvf(322u);
    for (std::size_t row = 0; row < 48u; ++row) {
        runtime.DrawTriangleStrip(vertices_.data() + row * 130u, 128u, 24u);
    }
    runtime.BindRaster(0u);
    runtime.SetRenderState(7u, 1u);
    return runtime.RestoreState(14u, 1u);
}

std::uint8_t WaterMeshRenderer::RenderFullScreen(
    const WaterEffectState& state, const WaterFrameContext& frame,
    std::int32_t screen_width, std::int32_t screen_height,
    IWaterMeshRuntime& runtime) {
    static_cast<void>(Render(state, frame.image->raster, 0.0f, 0.0f,
                             static_cast<float>(screen_width),
                             static_cast<float>(screen_height),
                             0xffffffffu, runtime));
    return 1u;
}

void WaterMeshRenderer::Cleanup() { vertices_.clear(); vertices_.shrink_to_fit(); }
const std::vector<WaterVertex>& WaterMeshRenderer::Vertices() const {
    return vertices_;
}

}  // namespace pal4
