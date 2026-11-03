#include "pal4/water_effect.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct ScriptRuntime final : pal4::ScreenEffectRuntimeStub {
    explicit ScriptRuntime(std::vector<std::string>& trace) : events(trace) {}
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == 0x5000u, "input address");
        events.emplace_back("read input");
        return input_byte;
    }
    void SelectTechnique(std::uint32_t script, std::string_view name) override {
        Check(script == 0x9000u && name == "techWater", "water technique");
        events.emplace_back("select");
    }
    void ExecuteTechnique(std::uint32_t script, std::uint32_t& input,
                          std::uint32_t flags) override {
        Check(script == 0x9000u && input == 0x5000u && flags == 0u,
              "execute receives input by reference");
        events.emplace_back("execute");
        input = 0x6000u;
    }
    std::uint32_t InvokeRenderSlot(std::uint32_t script,
                                    std::uint32_t slot) override {
        Check(script == 0x9000u, "render script");
        events.push_back(slot == 276u ? "render" : "stop");
        Check(slot == 276u || slot == 272u, "render slot");
        return 0x12abu;
    }
    std::uint32_t InvokeParameterSlot(std::uint32_t script,
                                      std::uint32_t value) override {
        Check(script == 0x9000u && value == 0x12345678u,
              "parameter forwarding");
        events.emplace_back("parameter");
        return 0x12cdu;
    }
    std::uint32_t ReleaseScript(std::uint32_t script) override {
        Check(script == 0x9000u, "release water script");
        events.emplace_back("release");
        return 0x34efu;
    }
    void ProcessPackageScript() override { events.emplace_back("package"); }
    std::uint32_t OpenFile(std::string_view path,
                            std::string_view mode) override {
        Check(path == "gamedata\\2D\\ScreenEffect\\WaterScreen\\Water.fx" &&
                  std::count(path.begin(), path.end(), '\\') == 4 && mode == "r",
              "exact water script path");
        events.emplace_back("open");
        return file;
    }
    void SeekFile(std::uint32_t handle, std::uint32_t offset,
                  std::uint32_t origin) override {
        Check(handle == 0x8000u && offset == 0u, "seek arguments");
        events.push_back(origin == 2u ? "seek end" : "seek start");
    }
    std::uint32_t FileSize(std::uint32_t handle) override {
        Check(handle == 0x8000u, "file size handle");
        events.emplace_back("size");
        return 64u;
    }
    std::uint32_t AllocateBuffer(std::uint32_t bytes) override {
        Check(bytes == 64u, "file allocation size");
        events.emplace_back("allocate");
        return buffer;
    }
    void ReadFile(std::uint32_t address, std::uint32_t bytes,
                  std::uint32_t count, std::uint32_t handle) override {
        Check(address == 0xa000u && bytes == 64u && count == 1u &&
                  handle == 0x8000u, "read whole script");
        events.emplace_back("read");
    }
    void CloseFile(std::uint32_t handle) override {
        Check(handle == 0x8000u, "close handle");
        events.emplace_back("close");
    }
    std::uint32_t CurrentD3DDevice() override {
        events.emplace_back("device");
        return 0xb000u;
    }
    std::uint32_t LoadScript(
        std::uint32_t device, std::uint32_t address, std::uint32_t bytes,
        std::uint32_t compile_flags, std::uint32_t options,
        std::uint32_t configuration, std::uint32_t execution_flags,
        std::uint32_t& output, std::uint32_t& error) override {
        Check(device == 0xb000u && address == 0xa000u && bytes == 64u &&
                  compile_flags == 0u && options == 0u &&
                  configuration == 0x104u && execution_flags == 0u,
              "water script compile parameters");
        events.emplace_back("load");
        output = 0x9000u;
        error = 0xe000u;
        return load_result;
    }
    std::string_view ErrorText(std::uint32_t error,
                               std::string_view label) override {
        Check(error == 0xe000u && label == "Error", "error extraction");
        events.emplace_back("error text");
        return "compile failed";
    }
    void ShowError(std::string_view text, std::uint32_t type) override {
        Check(text == "compile failed" && type == 0x55u, "message box");
        events.emplace_back("show error");
    }
    void ReleaseEnvironmentOnLoadError(std::uint32_t environment) override {
        Check(environment == 0x7000u, "load error environment");
        events.emplace_back("release environment");
    }
    void FreeBuffer(std::uint32_t address) override {
        Check(address == 0xa000u, "free script bytes");
        events.emplace_back("free buffer");
    }
    void CleanupScriptEnvironment(std::uint32_t& environment) override {
        Check(environment == 0x7000u, "cleanup environment word");
        events.emplace_back("cleanup environment");
    }
    std::vector<std::string>& events;
    std::uint8_t input_byte{1u};
    std::uint32_t file{0x8000u};
    std::uint32_t buffer{0xa000u};
    std::uint32_t load_result{};
};

struct WaterRuntime final : pal4::IWaterEffectRuntime {
    explicit WaterRuntime(std::vector<std::string>& trace) : events(trace) {}
    void Reset(pal4::WaterEffectState& state) override {
        Check(state.vtable == 0x1234u, "water reset receives own layout");
        events.emplace_back("reset");
    }
    bool CheckPixelShader(std::uint32_t version) override {
        Check(version == 0xffff0101u, "pixel capability version");
        events.emplace_back("pixel caps");
        return pixel_capable;
    }
    bool CheckVertexShader(std::uint32_t version) override {
        Check(version == 0xfffe0101u, "vertex capability version");
        events.emplace_back("vertex caps");
        return vertex_capable;
    }
    std::int32_t ScreenWidth() override {
        events.emplace_back("width");
        return width;
    }
    std::int32_t ScreenHeight() override {
        events.emplace_back("height");
        return height;
    }
    void SetInteger(std::uint32_t script, std::string_view name,
                    std::int32_t value) override {
        Check(script == 0x9000u, "water integer script");
        if (name == "halfWidth") {
            Check(value == width / 2, "signed integer half width");
            events.emplace_back("set half width");
        } else {
            Check(name == "halfHeight" && value == height / 2,
                  "signed integer half height");
            events.emplace_back("set half height");
        }
    }
    std::vector<std::string>& events;
    bool pixel_capable{true};
    bool vertex_capable{true};
    std::int32_t width{801};
    std::int32_t height{601};
};

struct MeshRuntime final : pal4::IWaterMeshRuntime {
    void BindRaster(std::uint32_t raster) override {
        events.push_back(raster == 0u ? "unbind" : "bind");
        Check(raster == 0u || raster == 0x8000u, "raster address");
    }
    void SetRenderState(std::uint32_t state, std::uint32_t value) override {
        states.push_back({state, value});
    }
    void SetFvf(std::uint32_t fvf) override {
        Check(fvf == 322u, "water FVF");
        events.emplace_back("fvf");
    }
    void DrawTriangleStrip(const pal4::WaterVertex* vertices,
                           std::uint32_t primitives,
                           std::uint32_t stride) override {
        Check(primitives == 128u && stride == 24u, "water strip format");
        first_vertices.push_back(vertices);
        events.emplace_back("draw");
    }
    std::uint32_t RestoreState(std::uint32_t state,
                               std::uint32_t value) override {
        Check(state == 14u && value == 1u, "final state restore");
        events.emplace_back("restore");
        return 0x1234u;
    }
    std::vector<std::string> events;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> states;
    std::vector<const pal4::WaterVertex*> first_vertices;
};

void ControlAndApply() {
    std::vector<std::string> events;
    ScriptRuntime script(events);
    WaterRuntime water(events);
    pal4::WaterEffectState state{0x1234u, 0u, 0u, {}, 7.0f, 0x9000u};
    Check(pal4::WaterEffect::Enable(state) == 0u && state.time == 7.0f,
          "unloaded enable leaves timer alone");
    state.loaded = 1u;
    Check(pal4::WaterEffect::Enable(state) == 0u && state.enabled == 1u &&
              state.time == 0.0f, "enable resets timer");
    state.time = 2.0f;
    Check(pal4::WaterEffect::Enable(state) == 1u && state.time == 2.0f,
          "second enable preserves timer");
    Check(pal4::WaterEffect::Apply(state, 0x5000u, script, water) == 0x6000u &&
              events == std::vector<std::string>({"read input", "select",
                  "width", "set half width", "height", "set half height",
                  "execute"}), "water apply ordering and mutated input");
    events.clear();
    script.input_byte = 0u;
    Check(pal4::WaterEffect::Apply(state, 0x5000u, script, water) == 0u &&
              events == std::vector<std::string>({"read input"}),
          "empty input gates script calls");
    events.clear();
    Check(pal4::WaterEffect::Apply(state, 0u, script, water) == 0u &&
              events.empty(), "null input gates byte read");
    script.input_byte = 1u;
    water.width = -801;
    water.height = -601;
    Check(pal4::WaterEffect::Apply(state, 0x5000u, script, water) == 0x6000u,
          "negative odd dimensions divide as signed integers");
    events.clear();
    Check(pal4::WaterEffect::Render(state, script) == 0xabu &&
              pal4::WaterEffect::SetParameter(state, 0x12345678u, script) ==
                  0xcdu &&
              pal4::WaterEffect::StopRendering(state, script) == 0xabu &&
              events == std::vector<std::string>({"render", "parameter", "stop"}),
          "water virtual forwarding and low bytes");
    events.clear();
    Check(pal4::WaterEffect::Advance(state, 0.25) == 1u &&
              state.time == 2.25f, "water timer adds double then stores float");
    state.enabled = 0u;
    Check(pal4::WaterEffect::Advance(state, 5.0) == 0u &&
              state.time == 2.25f && pal4::WaterEffect::Render(state, script) == 0u &&
              events.empty(), "disabled water gates calls and time");
    state.loaded = 0u;
    Check(pal4::WaterEffect::Advance(state, 5.0) == 0u &&
              state.time == 2.25f, "unloaded water gates time");
}

void LoadingAndCleanup() {
    std::vector<std::string> events;
    ScriptRuntime script(events);
    WaterRuntime water(events);
    pal4::WaterEffectState state{0x1234u, 0u, 1u, {}, 5.0f, 0u};
    water.pixel_capable = false;
    Check(!pal4::WaterEffect::Load(state, 0x55u, 0x7000u, script, water) &&
              events == std::vector<std::string>({"reset", "pixel caps"}),
          "first capability stops loader");
    events.clear();
    water.pixel_capable = true;
    water.vertex_capable = false;
    Check(!pal4::WaterEffect::Load(state, 0x55u, 0x7000u, script, water) &&
              events == std::vector<std::string>({"reset", "pixel caps",
                                                "vertex caps"}),
          "second capability stops loader");
    events.clear();
    water.vertex_capable = true;
    script.file = 0u;
    Check(!pal4::WaterEffect::Load(state, 0x55u, 0x7000u, script, water) &&
              events == std::vector<std::string>({"reset", "pixel caps",
                  "vertex caps", "package", "open", "cleanup environment"}),
          "missing file cleans environment");
    events.clear();
    script.file = 0x8000u;
    script.buffer = 0u;
    bool allocation_failed = false;
    try {
        static_cast<void>(pal4::WaterEffect::Load(
            state, 0x55u, 0x7000u, script, water));
    } catch (const std::bad_alloc&) {
        allocation_failed = true;
    }
    Check(allocation_failed && events.back() == "cleanup environment" &&
              state.script == 0u, "allocation failure cleans environment");
    events.clear();
    script.buffer = 0xa000u;
    Check(pal4::WaterEffect::Load(state, 0x55u, 0x7000u, script, water) &&
              state.loaded == 1u && state.enabled == 0u &&
              state.script == 0x9000u && state.time == 5.0f,
          "successful load writes script at +12 and preserves timer");
    Check(events == std::vector<std::string>({"reset", "pixel caps",
              "vertex caps", "package", "open", "seek end", "size",
              "seek start", "allocate", "read", "close", "device",
              "load", "free buffer", "cleanup environment"}),
          "successful load ordering");
    events.clear();
    state.enabled = 1u;
    Check(pal4::WaterEffect::Cleanup(state, script) == 0xefu &&
              state.loaded == 0u && state.enabled == 0u &&
              state.script == 0u && state.time == 5.0f &&
              events == std::vector<std::string>({"release"}),
          "cleanup releases script and preserves timer");
    events.clear();
    Check(pal4::WaterEffect::Cleanup(state, script) == 0u && events.empty(),
          "unloaded cleanup is inert");
    state.loaded = 1u;
    state.enabled = 1u;
    Check(pal4::WaterEffect::Cleanup(state, script) == 0u &&
              state.enabled == 0u && events.empty(),
          "loaded cleanup with null script");
    script.load_result = 1u;
    events.clear();
    Check(!pal4::WaterEffect::Load(state, 0x55u, 0x7000u, script, water) &&
              state.loaded == 0u && state.script == 0u &&
              std::find(events.begin(), events.end(), "show error") != events.end() &&
              events[events.size()-3u] == "release environment" &&
              events[events.size()-2u] == "free buffer" &&
              events.back() == "cleanup environment",
          "compile failure reports and cleans");
}

void Mesh() {
    pal4::WaterMeshRenderer mesh;
    MeshRuntime runtime;
    pal4::WaterEffectState state{};
    Check(mesh.Render(state, 0x8000u, 0.0f, 0.0f, 640.0f, 480.0f,
                      0xffabcdefu, runtime) == 0u &&
              mesh.Vertices().empty() && runtime.events.empty(),
          "unloaded mesh does no work");
    state.loaded = 1u;
    Check(mesh.Render(state, 0x8000u, 0.0f, 0.0f, 640.0f, 480.0f,
                      0xffabcdefu, runtime) == 0x1234u,
          "mesh returns final D3D state result");
    const auto& vertices = mesh.Vertices();
    Check(vertices.size() == 6240u && runtime.first_vertices.size() == 48u &&
              runtime.first_vertices.front() == vertices.data() &&
              runtime.first_vertices[1] == vertices.data() + 130u &&
              runtime.first_vertices.back() == vertices.data() + 47u * 130u,
          "48 strips of 130 interleaved vertices");
    Check(vertices[0].x == 0.0f && vertices[0].y == 0.0f &&
              vertices[0].z == 0.5f && vertices[0].color == 0xffabcdefu &&
              vertices[0].u == 0.0f && vertices[0].v == 0.0f &&
              vertices[128].x == 640.0f && vertices[128].u == 1.0f &&
              vertices.back().y == 480.0f && vertices.back().v == 1.0f,
          "mesh boundary, depth, color and UV coordinates");
    Check(runtime.states == std::vector<std::pair<std::uint32_t,
          std::uint32_t>>({{15u, 0u}, {27u, 1u}, {19u, 5u}, {20u, 6u},
                          {7u, 1u}, {14u, 0u}, {7u, 1u}}),
          "D3D state sequence");
    Check(runtime.events.front() == "bind" && runtime.events[1] == "fvf" &&
              runtime.events[runtime.events.size()-2u] == "unbind" &&
              runtime.events.back() == "restore", "mesh bind/draw/restore order");
    MeshRuntime shifted;
    state.enabled = 0u;
    static_cast<void>(mesh.Render(state, 0x8000u, 10.0f, 20.0f,
                                  100.0f, 120.0f, 0x11223344u, shifted));
    Check(shifted.first_vertices.size() == 48u &&
              mesh.Vertices()[0].x == 10.0f && mesh.Vertices()[0].y == 20.0f &&
              mesh.Vertices()[128].x == 100.0f &&
              mesh.Vertices().back().y == 120.0f &&
              mesh.Vertices()[0].color == 0x11223344u,
          "loaded mesh draws while disabled and honors bounds/color");
    const auto before = vertices[130u + 20u].x;
    state.time = 1.0f;
    MeshRuntime again;
    static_cast<void>(mesh.Render(state, 0x8000u, 0.0f, 0.0f,
                                  640.0f, 480.0f, 0xffabcdefu, again));
    Check(std::abs(mesh.Vertices()[130u + 20u].x - before) > 0.1f,
          "time changes interior displacement");
    mesh.Cleanup();
    Check(mesh.Vertices().empty(), "mesh cleanup clears cached vertices");
    pal4::WaterFrameImage image{0u, 0x8000u};
    pal4::WaterFrameContext frame{{}, &image};
    MeshRuntime fullscreen;
    Check(mesh.RenderFullScreen(state, frame, 640, 480, fullscreen) == 1u &&
              fullscreen.first_vertices.size() == 48u &&
              mesh.Vertices().front().color == 0xffffffffu &&
              mesh.Vertices()[128u].x == 640.0f &&
              mesh.Vertices().back().y == 480.0f,
          "fullscreen wrapper uses nested raster, dimensions and fixed return");
}

}  // namespace

int main() {
    ControlAndApply();
    LoadingAndCleanup();
    Mesh();
    std::cout << "water effect tests passed\n";
}
