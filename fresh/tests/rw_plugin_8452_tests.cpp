#include "pal4/rw_plugin_8452.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

class Runtime final : public pal4::RwPlugin8452RuntimeStub {
public:
    using C = pal4::RwPlugin8452Callback;
    std::vector<std::string> calls;
    std::map<std::uint32_t, std::uint32_t> words;
    std::int32_t engine_result = 0x20;
    std::int32_t geometry_result = 0x30;
    std::int32_t stream_result = 0x30;
    std::uint32_t custom = 0x6000u;
    std::uint32_t fallback = 0x7000u;
    std::uint32_t camera = 0;
    std::uint32_t invoked_callback = 0;
    std::uint32_t invoked_atomic = 0;
    std::uint32_t last_clump = 0;
    C last_clump_callback = C::Identity;
    pal4::RwPlugin8452State query_state{0x20, 0x30};
    std::vector<std::uint32_t> query_atomics;
    std::size_t query_visits = 0;
    pal4::RwPlugin8452Sphere before_transform{};
    pal4::RwPlugin8452Sphere frustum_sphere{};

    std::int32_t RegisterEngine(std::uint32_t size, std::uint32_t id,
                                C construct, C destruct) override {
        Check(size == 1u && id == 8452u &&
                  construct == C::Identity && destruct == C::Identity,
              "engine registration parameters");
        calls.push_back("engine");
        return engine_result;
    }
    std::int32_t RegisterGeometry(std::uint32_t size, std::uint32_t id,
                                  C construct, C destruct,
                                  C copy) override {
        Check(size == 12u && id == 8452u &&
                  construct == C::GeometryConstruct &&
                  destruct == C::GeometryDestruct &&
                  copy == C::GeometryCopy,
              "geometry registration parameters");
        calls.push_back("geometry");
        return geometry_result;
    }
    std::int32_t RegisterGeometryStream(std::uint32_t id, C read,
                                        C write, C size) override {
        Check(id == 8452u && read == C::Identity &&
                  write == C::Identity && size == C::StreamSize,
              "stream registration parameters");
        calls.push_back("stream");
        return stream_result;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read");
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("write");
        words[address] = value;
    }
    std::uint32_t ForAllAtomics(std::uint32_t clump,
                                C callback) override {
        calls.push_back("iterate");
        last_clump = clump;
        last_clump_callback = callback;
        return 0;
    }
    void ForAllAtomicsQuery(std::uint32_t clump, C callback,
                            std::uint32_t& result) override {
        calls.push_back("query");
        Check(clump == 0x5000u && callback == C::QueryAtomic,
              "query enumerates atomics with query callback");
        for (const auto atomic : query_atomics) {
            ++query_visits;
            if (pal4::RwPlugin8452::QueryAtomic(
                    atomic, result, query_state, *this) == 0)
                break;
        }
    }
    std::uint32_t CustomRenderCallback() override { return custom; }
    std::uint32_t DefaultRenderCallback() override { return fallback; }
    std::uint32_t ActiveCamera() override {
        calls.push_back("camera");
        return camera;
    }
    std::uint32_t GetObjectTransform(std::uint32_t frame) override {
        calls.push_back("transform");
        Check(frame == 0x777u, "render passes atomic frame");
        return 0x888u;
    }
    void ResyncAtomicSphere(std::uint32_t atomic) override {
        calls.push_back("resync");
        Check(atomic == 0x1000u, "render resyncs current atomic");
    }
    void CalculateSphere(pal4::RwPlugin8452Sphere& sphere,
                         std::uint32_t transform) override {
        calls.push_back("calculate");
        Check(transform == 0x888u, "render uses object transform");
        before_transform = sphere;
        sphere.x += 10.0f;
    }
    std::int32_t FrustumTestSphere(
        std::uint32_t active_camera,
        const pal4::RwPlugin8452Sphere& sphere) override {
        calls.push_back("frustum");
        Check(active_camera == camera, "render uses active camera");
        frustum_sphere = sphere;
        return -2;
    }
    void InvokeRender(std::uint32_t callback,
                      std::uint32_t atomic) override {
        calls.push_back("invoke");
        invoked_callback = callback;
        invoked_atomic = atomic;
    }
};

}  // namespace

int main() {
    Runtime runtime;
    pal4::RwPlugin8452State state{};
    runtime.engine_result = -1;
    Check(!pal4::RwPlugin8452::Register(state, runtime) &&
              state.engine_offset == -1 &&
              runtime.calls == std::vector<std::string>{"engine"},
          "engine failure stops registration");
    runtime.calls.clear();
    runtime.engine_result = 0x20;
    runtime.geometry_result = -1;
    Check(!pal4::RwPlugin8452::Register(state, runtime) &&
              state.engine_offset == 0x20 &&
              state.geometry_offset == -1 &&
              runtime.calls == std::vector<std::string>{"engine", "geometry"},
          "geometry failure stops before stream registration");
    runtime.calls.clear();
    runtime.geometry_result = 0x30;
    runtime.stream_result = 0x31;
    Check(!pal4::RwPlugin8452::Register(state, runtime) &&
              state.geometry_offset == 0x30 &&
              runtime.calls == std::vector<std::string>{
                  "engine", "geometry", "stream"},
          "stream offset must match geometry offset");
    runtime.calls.clear();
    runtime.stream_result = 0x30;
    Check(pal4::RwPlugin8452::Register(state, runtime) &&
              state.engine_offset == 0x20 &&
              state.geometry_offset == 0x30 &&
              pal4::RwPlugin8452::PluginSize() == 12u,
          "successful registration retains both offsets and plugin size");

    runtime.calls.clear();
    runtime.words[0x1030u] = 1;
    runtime.words[0x1034u] = 2;
    runtime.words[0x1038u] = 3;
    Check(pal4::RwPlugin8452::GeometryConstruct(
              0x1000u, state, runtime) == 0x1000u &&
              runtime.words[0x1030u] == 0 &&
              runtime.words[0x1034u] == 0 &&
              runtime.words[0x1038u] == 0,
          "geometry construction clears all three plugin words");
    runtime.words[0x1030u] = 4;
    runtime.words[0x1034u] = 5;
    runtime.words[0x1038u] = 6;
    Check(pal4::RwPlugin8452::GeometryDestruct(
              0x1000u, state, runtime) == 0x1000u &&
              runtime.words[0x1030u] == 0 &&
              runtime.words[0x1034u] == 0 &&
              runtime.words[0x1038u] == 0,
          "geometry destruction clears all three plugin words");
    runtime.words[0x2030u] = 7;
    runtime.words[0x2034u] = 8;
    runtime.words[0x2038u] = 9;
    Check(pal4::RwPlugin8452::GeometryCopy(
              0x1000u, 0x2000u, state, runtime) == 0x1000u &&
              runtime.words[0x1030u] == 7 &&
              runtime.words[0x1034u] == 8 &&
              runtime.words[0x1038u] == 9,
          "geometry copy preserves each of the twelve bytes");
    state.geometry_offset = 0;
    runtime.calls.clear();
    Check(pal4::RwPlugin8452::GeometryConstruct(
              0x1000u, state, runtime) == 0x1000u &&
              runtime.calls.empty(),
          "zero offset suppresses geometry initialization");
    state.geometry_offset = 0x30;

    runtime.calls.clear();
    Check(pal4::RwPlugin8452::ProcessClump(0x5000u, runtime) == 1u &&
              runtime.last_clump == 0x5000u &&
              runtime.last_clump_callback == Runtime::C::ProcessAtomic &&
              runtime.calls == std::vector<std::string>{"iterate"},
          "clump process enumerates atomics and returns one");
    runtime.calls.clear();
    Check(pal4::RwPlugin8452::ResetClump(0x5000u, runtime) == 1u &&
              runtime.last_clump_callback == Runtime::C::ResetAtomic &&
              runtime.calls == std::vector<std::string>{"iterate"},
          "clump reset enumerates the reset callback");

    runtime.calls.clear();
    runtime.words[0x1030u] = 0;
    runtime.words[0x1038u] = 0;
    runtime.words[0x1048u] = 0x9000u;
    Check(pal4::RwPlugin8452::ProcessAtomic(
              0x1000u, state, runtime) == 0x1000u &&
              runtime.words[0x1030u] == 0x9000u &&
              runtime.words[0x1034u] == 0 &&
              runtime.words[0x1038u] == 1 &&
              runtime.words[0x1048u] == 0x6000u,
          "atomic processing saves original callback and installs custom one");
    runtime.calls.clear();
    Check(pal4::RwPlugin8452::ProcessAtomic(
              0x1000u, state, runtime) == 0x1000u &&
              runtime.words[0x1048u] == 0x6000u &&
              runtime.calls == std::vector<std::string>{"read"},
          "active plugin is not installed twice");

    runtime.words[0x1034u] = 9;
    Check(pal4::RwPlugin8452::ResetAtomic(
              0x1000u, state, runtime) == 0x1000u &&
              runtime.words[0x1048u] == 0x9000u &&
              runtime.words[0x1030u] == 0x9000u &&
              runtime.words[0x1034u] == 0 &&
              runtime.words[0x1038u] == 0,
          "reset restores saved callback and clears status fields");
    runtime.words[0x1030u] = 0;
    runtime.words[0x1038u] = 1;
    Check(pal4::RwPlugin8452::ResetAtomic(
              0x1000u, state, runtime) == 0x1000u &&
              runtime.words[0x1048u] == 0x7000u,
          "reset uses default render callback when saved callback is null");

    runtime.words[0x1030u] = 0x9000u;
    runtime.camera = 0;
    runtime.calls.clear();
    Check(pal4::RwPlugin8452::RenderAtomic(
              0x1000u, state, runtime) == 0x1000u &&
              runtime.invoked_callback == 0x9000u &&
              runtime.invoked_atomic == 0x1000u &&
              runtime.calls == std::vector<std::string>{
                  "camera", "read", "invoke"},
          "no-camera render forwards to saved callback without frustum work");

    runtime.camera = 0x4000u;
    runtime.words[0x1004u] = 0x777u;
    runtime.words[0x104cu] = 2u;
    runtime.words[0x101cu] = std::bit_cast<std::uint32_t>(1.0f);
    runtime.words[0x1020u] = std::bit_cast<std::uint32_t>(2.0f);
    runtime.words[0x1024u] = std::bit_cast<std::uint32_t>(3.0f);
    runtime.words[0x1028u] = std::bit_cast<std::uint32_t>(4.0f);
    runtime.calls.clear();
    Check(pal4::RwPlugin8452::RenderAtomic(
              0x1000u, state, runtime) == 0x1000u &&
              runtime.before_transform.x == 1.0f &&
              runtime.before_transform.y == 2.0f &&
              runtime.before_transform.z == 3.0f &&
              runtime.before_transform.radius == 4.0f &&
              runtime.frustum_sphere.x == 11.0f &&
              runtime.words[0x1034u] == 0xfffffffeu &&
              runtime.invoked_callback == 0x9000u,
          "camera render transforms sphere, records test, then invokes callback");
    Check(runtime.calls == std::vector<std::string>{
              "camera", "read", "transform", "read", "resync",
              "read", "read", "read", "read", "calculate",
              "frustum", "write", "read", "invoke"},
          "render operations follow native order");

    runtime.calls.clear();
    state.engine_offset = 0;
    Check(pal4::RwPlugin8452::SetPluginData(
              0x1000u, 0x55u, state, runtime) == 0u &&
              runtime.calls.empty(),
          "engine offset guard prevents plugin data writes");
    state.engine_offset = 0x20;
    state.geometry_offset = 0;
    Check(pal4::RwPlugin8452::SetPluginData(
              0x1000u, 0x55u, state, runtime) == 0u &&
              runtime.calls.empty(),
          "geometry offset guard prevents plugin data writes");

    state.geometry_offset = 0x30;
    runtime.words[0x1034u] = 0xfffffffeu;
    runtime.words[0x1038u] = 1u;
    Check(pal4::RwPlugin8452::GetFrustumResult(
              0x1000u, state, runtime) == 0xfffffffeu &&
              pal4::RwPlugin8452::GetActiveFlag(
                  0x1000u, state, runtime) == 1u,
          "plugin accessors read frustum and active words");
    std::uint32_t query_result = 7u;
    runtime.words[0x1034u] = 0;
    Check(pal4::RwPlugin8452::QueryAtomic(
              0x1000u, query_result, state, runtime) == 0x1000u &&
              query_result == 0u,
          "active invisible atomic continues traversal with false result");
    runtime.words[0x1034u] = 2u;
    Check(pal4::RwPlugin8452::QueryAtomic(
              0x1000u, query_result, state, runtime) == 0u &&
              query_result == 1u,
          "active visible atomic stops traversal with true result");
    runtime.words[0x1038u] = 0;
    Check(pal4::RwPlugin8452::QueryAtomic(
              0x1000u, query_result, state, runtime) == 0u &&
              query_result == 1u,
          "inactive atomic stops traversal with true result");

    runtime.calls.clear();
    runtime.query_atomics.clear();
    runtime.query_visits = 0;
    Check(pal4::RwPlugin8452::QueryClump(
              0x5000u, runtime) == 0u &&
              runtime.query_visits == 0u &&
              runtime.calls == std::vector<std::string>{"query"},
          "empty clump query returns initialized false result");
    runtime.query_atomics = {0x1000u, 0x2000u, 0x3000u};
    runtime.words[0x1038u] = 1;
    runtime.words[0x1034u] = 0;
    runtime.words[0x2038u] = 1;
    runtime.words[0x2034u] = 4;
    runtime.query_visits = 0;
    Check(pal4::RwPlugin8452::QueryClump(
              0x5000u, runtime) == 1u &&
              runtime.query_visits == 2u,
          "clump query stops at first active visible atomic");
    runtime.words[0x2034u] = 0;
    runtime.words[0x3038u] = 1;
    runtime.words[0x3034u] = 0;
    runtime.query_visits = 0;
    Check(pal4::RwPlugin8452::QueryClump(
              0x5000u, runtime) == 0u &&
              runtime.query_visits == 3u,
          "all active invisible atomics leave clump query false");

    std::cout << "rw_plugin_8452_tests passed\n";
}
