#include "pal4/loading_frame.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ILoadingFrameRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::vector<std::string> calls;
    std::map<std::uint32_t, std::uint32_t> words;
    std::uint32_t allocated = object;
    std::uint32_t registered = 0x9000u;
    std::string fail_at;
    std::uint32_t registered_manager = 0;
    std::uint32_t registered_id = 0;
    std::uint32_t registered_factory = 0;
    std::uint32_t registered_type = 0;
    std::string registered_name;

    void MaybeThrow(std::string_view stage) {
        if (fail_at == stage) throw std::runtime_error("injected failure");
    }
    std::uint32_t SingletonManager() override {
        calls.emplace_back("manager");
        return 0x5000u;
    }
    std::uint32_t RegisterSingleton(std::uint32_t manager,
                                     std::string_view name,
                                     std::uint32_t id,
                                     std::uint32_t factory,
                                     std::uint32_t type_info) override {
        calls.emplace_back("register");
        registered_manager = manager;
        registered_name = name;
        registered_id = id;
        registered_factory = factory;
        registered_type = type_info;
        return registered;
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == 0x328u, "LoadingFrame allocation size");
        calls.emplace_back("allocate");
        return allocated;
    }
    void Free(std::uint32_t address) override {
        Check(address == object, "factory frees allocated block");
        calls.emplace_back("free");
    }
    void CriticalSectionInitialize(std::uint32_t address) override {
        Check(address == object, "critical-section base");
        calls.emplace_back("critical init");
        MaybeThrow("critical init");
    }
    void CriticalSectionDestroy(std::uint32_t address) override {
        Check(address == object, "critical-section destruction base");
        calls.emplace_back("critical destroy");
    }
    void BoneConstruct(std::uint32_t address) override {
        Check(address == object + 0x28u, "bone offset");
        calls.emplace_back("bone");
        MaybeThrow("bone");
    }
    void MovieInitialize(std::uint32_t address) override {
        Check(address == object + 0x3cu, "movie offset");
        calls.emplace_back("movie init");
        MaybeThrow("movie init");
    }
    void MovieCleanup(std::uint32_t address) override {
        Check(address == object + 0x3cu, "movie cleanup offset");
        calls.emplace_back("movie cleanup");
        MaybeThrow("movie cleanup");
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        Check(address == object && value == 0x8493e8u,
              "LoadingFrame vtable write");
        calls.emplace_back("vtable");
        words[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        Check(address == object + 0x38u && value == 0,
              "LoadingFrame stop byte");
        calls.emplace_back("stop byte");
        words[address] = value;
    }
};

struct CameraRuntime final : pal4::ILoadingFrameCameraRuntime {
    std::vector<std::string> calls;
    std::string cached;
    std::uint32_t camera_handle = 0x8000u;
    std::uint32_t bound_camera = 0;
    std::int32_t width = 0;
    std::int32_t height = 0;
    float near_depth = 0;
    float far_depth = 0;
    char* Allocate(std::size_t size) override {
        calls.emplace_back("allocate path");
        return new char[size];
    }
    int Release(char* path) override {
        calls.emplace_back("release path");
        delete[] path;
        return 1;
    }
    float GameDepth() override {
        calls.emplace_back("game depth");
        return 8.0f;
    }
    float GameFarDepth() override {
        calls.emplace_back("game far depth");
        return 700.0f;
    }
    float WindowScale() override { return 1.5f; }
    std::uint32_t WindowFlags() override { return 9u; }
    pal4::LoadingFrameViewport Viewport() override {
        calls.emplace_back("viewport");
        return {10, 20, 410, 320};
    }
    void CameraInitialize(std::uint32_t camera,
                          std::int32_t x, std::int32_t y,
                          std::uint32_t mode, float near_value,
                          float far_value) override {
        calls.emplace_back("camera init");
        Check(camera == Runtime::object + 0x3cu && mode == 1u,
              "camera address and mode");
        width = x;
        height = y;
        near_depth = near_value;
        far_depth = far_value;
    }
    void CameraAdjustViewWindow(std::uint32_t camera,
                                std::int32_t left, std::int32_t top,
                                float scale, std::uint32_t flags) override {
        calls.emplace_back("adjust window");
        Check(camera == Runtime::object + 0x3cu && left == 10 &&
                  top == 20 && scale == 1.5f && flags == 9u,
              "camera view window arguments");
    }
    void CameraSetParameters(std::uint32_t camera,
                             const std::uint32_t (&zeroes)[3],
                             float first, float second,
                             std::uint32_t mode, float last) override {
        calls.emplace_back("camera parameters");
        Check(camera == Runtime::object + 0x3cu && zeroes[0] == 0u &&
                  zeroes[1] == 0u && zeroes[2] == 0u && first == 225.0f &&
                  second == 30.0f && mode == 0u && last == 350.0f,
              "camera parameters and zero vector");
    }
    std::uint32_t CameraHandle(std::uint32_t camera) override {
        calls.emplace_back("camera handle");
        Check(camera == Runtime::object + 0x3cu, "camera handle field");
        return camera_handle;
    }
    int CameraDestroy(std::uint32_t camera) override {
        calls.emplace_back("camera destroy");
        Check(camera == Runtime::object + 0x3cu, "camera destroy field");
        return 27;
    }
    void CachePath(const char* path) override {
        calls.emplace_back("cache path");
        cached = path;
    }
    void Open2D(std::uint32_t value) override {
        Check(value == 0u, "Rt2d open parameter");
        calls.emplace_back("rt2d open");
    }
    void OpenAnimation() override { calls.emplace_back("open animation"); }
    void SetFlatNear(float value) override {
        calls.emplace_back("flat near");
        Check(value == 9.0f, "flat near depth");
    }
    void SetFlatFar(float value) override {
        calls.emplace_back("flat far");
        Check(value == 32.0f, "flat far depth");
    }
    int SetCamera(std::uint32_t camera) override {
        calls.emplace_back("rt2d camera");
        bound_camera = camera;
        return 31;
    }
    void CloseAnimation() override {
        calls.emplace_back("close animation");
    }
    int Close2D() override {
        calls.emplace_back("close 2d");
        return 8;
    }
};

struct LoopRuntime final : pal4::ILoadingFrameLoopRuntime {
    std::vector<std::string> calls;
    bool active = true;
    bool stop = false;
    bool destroy = false;
    bool stop_after_update = false;
    bool deactivate_after_sleep = false;
    int updates = 0;

    std::uint8_t ReadByte(std::uint32_t address) override {
        if (address == Runtime::object + 0x1du) {
            calls.emplace_back("active");
            return active;
        }
        if (address == Runtime::object + 0x1cu) {
            calls.emplace_back("stop");
            return stop;
        }
        Check(address == Runtime::object + 0x38u, "loop flag address");
        calls.emplace_back("destroy flag");
        return destroy;
    }
    void UpdateGameFrame() override {
        calls.emplace_back("update");
        ++updates;
        if (stop_after_update) stop = true;
    }
    void SleepMilliseconds(std::uint32_t duration) override {
        calls.emplace_back("sleep");
        Check(duration == 30u, "loop sleeps for 30 milliseconds");
        if (deactivate_after_sleep) active = false;
    }
    void DestroyMaestro(std::uint32_t address) override {
        calls.emplace_back("destroy maestro");
        Check(address == Runtime::object + 0x28u,
              "maestro subobject offset");
    }
};

struct RenderRuntime final : pal4::ILoadingFrameRenderRuntime {
    std::vector<std::string> calls;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> states;
    std::map<std::uint32_t, std::uint32_t> words;
    std::uint32_t begin_result = 0x7777u;
    std::uint32_t new_maestro = 0;
    float delta = 0;
    int registrations = 0;

    std::uint32_t SingletonManager() override {
        calls.emplace_back("singleton manager");
        return 0x5000u;
    }
    std::uint32_t RegisterSingleton(std::uint32_t manager,
                                     std::string_view name,
                                     std::uint32_t id,
                                     std::uint32_t factory,
                                     std::uint32_t type_info) override {
        Check(manager == 0x5000u && name == "palLoadingFrame" &&
                  id == 1048u && factory == 0x5aa3b0u &&
                  type_info == 0x7de5d0u, "render singleton registration");
        calls.emplace_back("register");
        ++registrations;
        return Runtime::object;
    }
    std::uint32_t Allocate(std::uint32_t) override { return 0; }
    void Free(std::uint32_t) override {}
    void CriticalSectionInitialize(std::uint32_t) override {}
    void CriticalSectionDestroy(std::uint32_t) override {}
    void BoneConstruct(std::uint32_t) override {}
    void MovieInitialize(std::uint32_t) override {}
    void MovieCleanup(std::uint32_t) override {}
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.emplace_back("clear dirty");
        Check(address == Runtime::object + 0x34u && value == 0u,
              "maestro dirty flag address");
        words[address] = value;
    }
    void WriteByte(std::uint32_t, std::uint8_t) override {}
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    float GameAnimationScale() override {
        calls.emplace_back("scale");
        return 2.5f;
    }
    void AddMaestroDeltaTime(std::uint32_t maestro, float value) override {
        Check(maestro == 0x4000u, "maestro update handle");
        calls.emplace_back("add delta");
        delta = value;
        if (new_maestro != 0u) words[Runtime::object + 0x28u] = new_maestro;
    }
    int UpdateMaestroAnimations(std::uint32_t maestro) override {
        calls.emplace_back("update animations");
        Check(maestro == (new_maestro == 0u ? 0x4000u : new_maestro),
              "maestro handle reread after delta update");
        return -9;
    }
    void ClearCamera(std::uint32_t camera, std::uint32_t colour,
                     std::uint32_t flags) override {
        calls.emplace_back("clear camera");
        Check(camera == 0x6000u && colour == 0x8fdde8u && flags == 3u,
              "camera clear arguments");
    }
    std::uint32_t BeginCameraUpdate(std::uint32_t camera) override {
        calls.emplace_back("begin camera");
        Check(camera == 0x6000u, "camera begin handle");
        return begin_result;
    }
    void EndCameraUpdate(std::uint32_t camera) override {
        calls.emplace_back("end camera");
        Check(camera == begin_result, "camera end uses begin result");
    }
    int ShowGameRaster(std::uint32_t camera) override {
        calls.emplace_back("show raster");
        Check(camera == begin_result, "raster uses begin result");
        return 59;
    }
    void SetRenderState(std::uint32_t key,
                        std::uint32_t value) override {
        calls.emplace_back("render state");
        states.emplace_back(key, value);
    }
    void PushTransform() override { calls.emplace_back("push transform"); }
    int PopTransform() override {
        calls.emplace_back("pop transform");
        return 37;
    }
    void ObjectTransformChanged(std::uint32_t object) override {
        calls.emplace_back("transform changed");
        Check(object == 0x5000u, "maestro object handle");
    }
    void RenderMaestro(std::uint32_t maestro) override {
        calls.emplace_back("render maestro");
        Check(maestro == 0x4000u, "maestro render handle");
    }
};

void TestSingleton() {
    Runtime runtime;
    pal4::LoadingFrameSingletonState state{0x80u, 0};
    Check(pal4::LoadingFrame::Get(state, runtime) == runtime.registered,
          "first singleton lookup registers");
    Check(state.guard == 0x81u && state.instance == runtime.registered,
          "guard and instance stored");
    Check(runtime.registered_manager == 0x5000u &&
              runtime.registered_name == "palLoadingFrame" &&
              runtime.registered_id == 1048u &&
              runtime.registered_factory == pal4::LoadingFrame::kFactory &&
              runtime.registered_type == pal4::LoadingFrame::kTypeInfo,
          "singleton registration parameters");
    runtime.registered = 0;
    Check(pal4::LoadingFrame::Get(state, runtime) == 0x9000u &&
              runtime.calls.size() == 2u, "guarded lookup reuses instance");
}

void TestLifecycle() {
    Runtime runtime;
    Check(pal4::LoadingFrame::Create(runtime) == Runtime::object,
          "factory returns constructed object");
    Check(runtime.calls == std::vector<std::string>({
              "allocate", "critical init", "bone", "movie init",
              "vtable", "stop byte"}), "constructor order");
    Check(runtime.words[Runtime::object] == pal4::LoadingFrame::kVtable &&
              runtime.words[Runtime::object + 0x38u] == 0u,
          "constructor fields");
    pal4::LoadingFrame::Destruct(Runtime::object, runtime);
    Check(runtime.calls[runtime.calls.size() - 3] == "vtable" &&
              runtime.calls[runtime.calls.size() - 2] == "movie cleanup" &&
              runtime.calls.back() == "critical destroy",
          "destructor order");

    Runtime empty;
    empty.allocated = 0;
    Check(pal4::LoadingFrame::Create(empty) == 0 &&
              empty.calls == std::vector<std::string>({"allocate"}),
          "allocation failure skips construction");
}

void TestUnwind() {
    for (const std::string stage : {"critical init", "bone", "movie init"}) {
        Runtime runtime;
        runtime.fail_at = stage;
        try {
            static_cast<void>(pal4::LoadingFrame::Create(runtime));
            throw std::runtime_error("expected construction failure");
        } catch (const std::runtime_error& error) {
            Check(std::string(error.what()) == "injected failure",
                  "constructor propagated injected failure");
        }
        Check(runtime.calls.back() == "free", "factory releases failed object");
        const bool had_critical = stage != "critical init";
        Check((runtime.calls[runtime.calls.size() - 2] == "critical destroy") ==
                  had_critical, "constructed critical section unwound");
    }

    Runtime runtime;
    runtime.fail_at = "movie cleanup";
    try {
        pal4::LoadingFrame::Destruct(Runtime::object, runtime);
        throw std::runtime_error("expected cleanup failure");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "injected failure",
              "destructor propagated cleanup failure");
    }
    Check(runtime.calls == std::vector<std::string>({
              "vtable", "movie cleanup", "critical destroy"}),
          "destructor unwinds critical section");
}

void TestCameraLifecycle() {
    CameraRuntime runtime;
    Check(pal4::LoadingFrame::InitializeCamera(Runtime::object, runtime) == 31,
          "camera initialization returns Rt2d camera result");
    Check(runtime.width == 400 && runtime.height == 300 &&
              runtime.near_depth == 8.0f && runtime.far_depth == 700.0f,
          "viewport extents and game depth passed to camera");
    Check(runtime.cached == "gamedata\\animation\\font\\" &&
              runtime.bound_camera == runtime.camera_handle,
          "font path and camera handle forwarded");
    Check(runtime.calls == std::vector<std::string>({
              "viewport", "game far depth", "game depth", "camera init",
              "adjust window", "camera parameters", "rt2d open",
              "open animation", "game depth", "flat near", "flat far",
              "allocate path", "cache path", "release path",
              "camera handle", "rt2d camera"}),
          "camera and Rt2d setup order");
    Check(pal4::LoadingFrame::CloseCamera(Runtime::object, runtime) == 27,
          "camera closure returns camera destroy result");
    Check(runtime.calls[runtime.calls.size() - 3] == "close animation" &&
              runtime.calls[runtime.calls.size() - 2] == "close 2d" &&
              runtime.calls.back() == "camera destroy",
          "Rt2d closes before camera destruction");
}

void TestProcessLoop() {
    LoopRuntime inactive;
    inactive.active = false;
    Check(pal4::LoadingFrame::ProcessLoop(Runtime::object, inactive) == 0 &&
              inactive.calls == std::vector<std::string>({"active"}),
          "inactive frame returns before checking stop");

    LoopRuntime stopped;
    stopped.stop = true;
    Check(pal4::LoadingFrame::ProcessLoop(Runtime::object, stopped) == 0 &&
              stopped.calls == std::vector<std::string>({"active", "stop"}),
          "stop flag skips frame update");

    LoopRuntime destroying;
    destroying.destroy = true;
    Check(pal4::LoadingFrame::ProcessLoop(Runtime::object, destroying) == 0 &&
              destroying.calls == std::vector<std::string>({
                  "active", "stop", "update", "destroy flag",
                  "destroy maestro"}),
          "destroy flag bypasses sleep");

    LoopRuntime deactivating;
    deactivating.deactivate_after_sleep = true;
    Check(pal4::LoadingFrame::ProcessLoop(Runtime::object, deactivating) == 0 &&
              deactivating.calls == std::vector<std::string>({
                  "active", "stop", "update", "destroy flag",
                  "sleep", "active"}),
          "inactive after sleep exits loop");

    LoopRuntime stopping;
    stopping.stop_after_update = true;
    Check(pal4::LoadingFrame::ProcessLoop(Runtime::object, stopping) == 0 &&
              stopping.updates == 1 &&
              stopping.calls == std::vector<std::string>({
                  "active", "stop", "update", "destroy flag",
                  "sleep", "active", "stop"}),
          "stop after update is checked on next iteration");
}

void TestMaestroThunks() {
    RenderRuntime runtime;
    Check(pal4::LoadingFrame::UpdateMaestro(Runtime::object, 1.25f,
                                             runtime) == 0 &&
              runtime.calls.empty(), "empty maestro skips animation update");
    runtime.words[Runtime::object + 0x28u] = 0x4000u;
    runtime.new_maestro = 0x4100u;
    Check(pal4::LoadingFrame::UpdateMaestro(Runtime::object, 1.25f,
                                             runtime) == -9 &&
              runtime.delta == 1.25f &&
              runtime.calls == std::vector<std::string>({
                  "add delta", "update animations"}),
          "maestro thunk forwards delta and rereads handle");

    RenderRuntime empty;
    Check(pal4::LoadingFrame::RenderContent(Runtime::object, 0x7777u,
                                             empty) == 0 &&
              empty.calls.empty(), "inactive content skips render states");
    empty.words[Runtime::object + 0x30u] = 1u;
    Check(pal4::LoadingFrame::RenderContent(Runtime::object, 0x7777u,
                                             empty) == 37 &&
              empty.calls.back() == "pop transform",
          "active content pops transform without maestro");
    Check(empty.states == std::vector<std::pair<std::uint32_t, std::uint32_t>>({
              {8u, 0u}, {6u, 0u}, {7u, 2u}, {9u, 2u}, {12u, 1u},
              {20u, 1u}, {10u, 5u}, {11u, 6u}, {9u, 2u}, {12u, 1u}}),
          "exact RenderWare state sequence");

    RenderRuntime dirty;
    dirty.words[Runtime::object + 0x30u] = 1u;
    dirty.words[Runtime::object + 0x28u] = 0x4000u;
    dirty.words[Runtime::object + 0x2cu] = 0x5000u;
    dirty.words[Runtime::object + 0x34u] = 1u;
    Check(pal4::LoadingFrame::RenderContent(Runtime::object, 0x7777u,
                                             dirty) == 37 &&
              dirty.words[Runtime::object + 0x34u] == 0u,
          "dirty transform cleared before rendering maestro");
    Check(dirty.calls[dirty.calls.size() - 4] == "transform changed" &&
              dirty.calls[dirty.calls.size() - 3] == "clear dirty" &&
              dirty.calls[dirty.calls.size() - 2] == "render maestro" &&
              dirty.calls.back() == "pop transform", "dirty render order");
}

void TestRenderCallback() {
    RenderRuntime runtime;
    runtime.words[Runtime::object + 0x28u] = 0x4000u;
    pal4::LoadingFrameSingletonState state;
    Check(pal4::LoadingFrame::Render(0.4, state, runtime) ==
              static_cast<int>(Runtime::object + 0x3cu),
          "null camera returns camera field address");
    Check(runtime.registrations == 1 && runtime.delta == 1.0f &&
              runtime.calls == std::vector<std::string>({
                  "singleton manager", "register", "scale", "add delta",
                  "update animations"}), "render advances animation before camera gate");

    runtime.calls.clear();
    runtime.words[Runtime::object + 0x3cu] = 0x6000u;
    runtime.begin_result = 0u;
    Check(pal4::LoadingFrame::Render(0.4, state, runtime) == 0 &&
              runtime.calls.back() == "begin camera" &&
              runtime.registrations == 1, "failed camera begin skips drawing");

    runtime.calls.clear();
    runtime.begin_result = 0x7777u;
    runtime.words[Runtime::object + 0x30u] = 1u;
    runtime.words[Runtime::object + 0x28u] = 0x4000u;
    Check(pal4::LoadingFrame::Render(0.4, state, runtime) == 59,
          "successful render returns raster result");
    Check(runtime.calls[runtime.calls.size() - 2] == "end camera" &&
              runtime.calls.back() == "show raster",
          "camera update ends before presenting raster");
    Check(runtime.calls[runtime.calls.size() - 3] == "pop transform" &&
              runtime.states.size() == 10u,
          "successful render invokes content thunk");
}

}  // namespace

int main() {
    try {
        TestSingleton();
        TestLifecycle();
        TestUnwind();
        TestCameraLifecycle();
        TestProcessLoop();
        TestMaestroThunks();
        TestRenderCallback();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
