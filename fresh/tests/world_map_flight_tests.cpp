#include "pal4/renderware_vector2.h"
#include "pal4/world_map_flight.h"

#include <array>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(const bool value, const char* message) {
    if (!value) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct VectorRuntime final : pal4::IRenderWareVector2 {
    double result = 2.5;
    std::uint32_t calls = 0u;
    double Length(const float x, const float y) override {
        Check(x == 1.5f && y == 2.0f, "traveled vector passed to RenderWare");
        ++calls;
        return result;
    }
    std::pair<float, float> Normalize(float, float) override {
        return {0.0f, 0.0f};
    }
};

struct Runtime final : pal4::IWorldMapFlightRuntime {
    static constexpr std::uint32_t kFrame = 0x5000u;
    std::map<std::uint32_t, float> floats{
        {kFrame + 0x240u, 3.0f}, {kFrame + 0x244u, 4.0f},
        {kFrame + 0x268u, 0.5f}, {kFrame + 0x250u, 0.0f},
        {kFrame + 0x254u, 0.0f}, {kFrame + 0x248u, 10.0f},
        {kFrame + 0x24cu, 20.0f}, {kFrame + 0x23cu, 5.0f}};
    std::map<std::uint32_t, std::uint32_t> words{
        {kFrame + 0x238u, 0x6000u}, {0x7000u + 0x1a0u, 0x7100u},
        {kFrame, 0x843ca8u}, {0x843ca8u + 4u, 0x484e20u}};
    std::vector<std::uint32_t> float_reads;
    std::vector<std::string> calls;
    std::uint8_t reference_count = 2u;
    std::uint32_t interpreter = 0x9000u;
    bool module_added = false;
    bool fail_create = false;
    bool strict_marker = true;
    std::uint32_t engine_calls = 0u;

    float ReadFloat(const std::uint32_t address) override {
        float_reads.push_back(address);
        return floats.at(address);
    }
    void WriteFloat(const std::uint32_t address,
                    const float value) override {
        calls.emplace_back("write float");
        floats[address] = value;
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.emplace_back("read word");
        return words.at(address);
    }
    std::uint8_t ReadByte(const std::uint32_t address) override {
        Check(address == 0x8100u, "script-name reference header");
        calls.emplace_back("read byte");
        return reference_count;
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        Check(address == 0x8100u, "script-name reference write");
        calls.emplace_back("write byte");
        reference_count = value;
    }
    std::uint32_t Free(const std::uint32_t address) override {
        Check(address == 0x8100u, "script-name free address");
        calls.emplace_back("free");
        return 0xdeadu;
    }
    void SetPosition(const std::uint32_t window,
                     const float x, const float y) override {
        Check(window == 0x6000u &&
                  (strict_marker ? x == 11.5f && y == 22.0f :
                   x == floats.at(kFrame + 0x248u) && y == floats.at(kFrame + 0x24cu)),
              "updated marker position");
        calls.emplace_back("position");
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x7000u;
    }
    void RemoveFlyMember(const std::uint32_t collection,
                         const std::uint32_t name_address) override {
        Check(collection == 0x7100u && name_address == kFrame + 0x258u,
              "fly member removal arguments");
        calls.emplace_back("remove");
    }
    void InitializeScripts() override {
        calls.emplace_back("script init");
    }
    std::uint32_t ScriptMode() override {
        calls.emplace_back("mode");
        return 3u;
    }
    std::uint32_t EmptyText() override {
        calls.emplace_back("empty text");
        return 0x8200u;
    }
    std::uint32_t CreateInterpreter(
        const std::uint32_t mode, const std::string_view object_name,
        const std::uint32_t script_name,
        const std::uint32_t full_path,
        const std::string_view file,
        const std::uint32_t reader) override {
        Check(mode == 3u && object_name == "WorldMap" &&
                  script_name == full_path &&
                  (script_name == 0x8101u || script_name == 0x8200u) &&
                  file == "gamedata\\editData\\script\\worldMap.csb" &&
                  reader == 0x600d50u,
              "world-map arrival script arguments");
        calls.emplace_back("create");
        if (fail_create) throw std::runtime_error("create");
        return interpreter;
    }
    std::uint32_t ScriptEngine() override {
        calls.emplace_back("engine");
        return 0x8000u + engine_calls++;
    }
    bool AddScriptModule(const std::uint32_t context,
                 const std::uint32_t value) override {
        Check(context == 0x8054u && value == interpreter,
              "arrival script context");
        calls.emplace_back("add module");
        return module_added;
    }
    std::uint32_t FunctionName(const std::uint32_t value) override {
        Check(value == interpreter, "script function name source");
        calls.emplace_back("function name");
        return 0xa000u;
    }
    std::uint32_t FullPath(const std::uint32_t value) override {
        Check(value == interpreter, "script full-path source");
        calls.emplace_back("full path");
        return 0xa004u;
    }
    void PushPreparedModule(const std::uint32_t context,
                               const std::uint32_t full_path,
                               const std::uint32_t function_name) override {
        Check(context == 0x8055u && full_path == 0xa004u &&
                  function_name == 0xa000u,
              "prepared destination queue arguments");
        calls.emplace_back("push prepared module");
    }
    void ReleaseInterpreter(const std::uint32_t value) override {
        Check(value == interpreter, "arrival interpreter release");
        calls.emplace_back("release");
    }
    void CallHide(const std::uint32_t frame,
                  const std::uint32_t function) override {
        Check(frame == kFrame && function == 0x484e20u,
              "hide through frame vtable slot one");
        calls.emplace_back("hide");
    }
    std::uint32_t CleanupAllScripts() override {
        calls.emplace_back("cleanup");
        return 0x1234u;
    }
};

void CheckMotion(const Runtime& runtime) {
    Check(runtime.floats.at(Runtime::kFrame + 0x250u) == 1.5f &&
              runtime.floats.at(Runtime::kFrame + 0x254u) == 2.0f &&
              runtime.floats.at(Runtime::kFrame + 0x248u) == 11.5f &&
              runtime.floats.at(Runtime::kFrame + 0x24cu) == 22.0f &&
              runtime.float_reads[0] == Runtime::kFrame + 0x240u &&
              runtime.float_reads[1] == Runtime::kFrame + 0x268u &&
              runtime.float_reads[2] == Runtime::kFrame + 0x244u &&
              runtime.float_reads[3] == Runtime::kFrame + 0x268u,
          "velocity, speed, travel and position updates");
}

void TestFlying() {
    Runtime runtime;
    VectorRuntime renderware;
    Check(pal4::WorldMapFlight::Advance(Runtime::kFrame, 0u,
                                         runtime, renderware) == 0x100u &&
              renderware.calls == 1u &&
              runtime.calls.back() == "position",
          "below threshold moves marker without arrival script");
    CheckMotion(runtime);

    Runtime named;
    renderware.result = std::numeric_limits<double>::quiet_NaN();
    Check(pal4::WorldMapFlight::Advance(Runtime::kFrame, 0x8101u,
                                         named, renderware) == 0x8100u &&
              named.reference_count == 1u &&
              named.calls.back() == "write byte",
          "unordered RenderWare length skips arrival and releases name");
}

void TestCapturedQ06ToM11Rounding() {
    // Pair127 original confirm/fly-start map snapshots, 2026-11-01:
    // six real WorldMapFly callbacks at interval .01. The endpoint is captured;
    // intermediate operation references expose where extended dx first diverges.
    Runtime runtime;
    runtime.strict_marker = false;
    runtime.floats[Runtime::kFrame + 0x240u] = std::bit_cast<float>(1035887067u);
    runtime.floats[Runtime::kFrame + 0x244u] = std::bit_cast<float>(1024913159u);
    runtime.floats[Runtime::kFrame + 0x268u] = std::bit_cast<float>(1008981770u);
    runtime.floats[Runtime::kFrame + 0x23cu] = std::bit_cast<float>(1041815292u);
    runtime.floats[Runtime::kFrame + 0x248u] = std::bit_cast<float>(1044130038u);
    runtime.floats[Runtime::kFrame + 0x24cu] = std::bit_cast<float>(1046786430u);
    struct MotionVector final : pal4::IRenderWareVector2 {
        std::uint32_t calls{};
        double Length(float, float) override { ++calls; return 0.; }
        std::pair<float,float> Normalize(float,float) override {
            throw std::logic_error("flight advance does not normalize velocity");
        }
    } renderware;
    constexpr std::array<std::array<std::uint32_t,4>,6> expected{{
        {980660340u,968962548u,1044192422u,1046811159u},
        {989048948u,977351156u,1044254806u,1046835888u},
        {993444951u,982574455u,1044317190u,1046860617u},
        {997437556u,985739764u,1044379574u,1046885346u},
        {999837256u,988905073u,1044441958u,1046910075u},
        {1001833558u,990963063u,1044504342u,1046934804u}}};
    constexpr std::array<std::uint32_t,4> offsets{0x250u,0x254u,0x248u,0x24cu};
    for (const auto& tick : expected) {
        Check(pal4::WorldMapFlight::Advance(Runtime::kFrame,0u,runtime,renderware) == 0x100u,
              "captured flight remains before arrival");
        for (std::size_t component=0;component<offsets.size();++component)
            Check(std::bit_cast<std::uint32_t>(runtime.floats.at(Runtime::kFrame+offsets[component])) == tick[component],
                  "original Q06-to-M11 travel and marker words match each rounded tick");
    }
    Check(renderware.calls == 6u && runtime.engine_calls == 0u,
          "six flight ticks use RenderWare length without premature destination registration");
}

void TestArrival() {
    Runtime runtime;
    runtime.floats[Runtime::kFrame + 0x23cu] = 2.5f;
    VectorRuntime renderware;
    Check(pal4::WorldMapFlight::Advance(Runtime::kFrame, 0u,
                                         runtime, renderware) == 0x1234u &&
              runtime.calls.back() == "cleanup" &&
              runtime.engine_calls == 1u,
          "equal length completes arrival and attempts module registration");
    CheckMotion(runtime);

    runtime = Runtime{};
    runtime.floats[Runtime::kFrame + 0x23cu] = 2.0f;
    runtime.interpreter = 0u;
    Check(pal4::WorldMapFlight::Advance(Runtime::kFrame, 0u,
                                         runtime, renderware) == 0x1234u &&
              runtime.engine_calls == 0u &&
              runtime.calls.back() == "cleanup",
          "null interpreter still cleans script resources");
}

void TestPreparedDestinationQueue() {
    Runtime runtime;
    runtime.floats[Runtime::kFrame + 0x23cu] = 2.0f;
    runtime.module_added = true;
    runtime.reference_count = 0u;
    VectorRuntime renderware;
    Check(pal4::WorldMapFlight::Advance(Runtime::kFrame, 0x8101u,
                                         runtime, renderware) == 0xdeadu &&
              runtime.engine_calls == 2u &&
              runtime.calls.back() == "free",
          "registered destination queues, hides and frees name");
    const std::vector<std::string> expected{
        "function name", "full path", "push prepared module", "release", "read word",
        "read word", "hide", "cleanup", "read byte", "free"};
    Check(std::vector<std::string>(runtime.calls.end() -
              static_cast<std::ptrdiff_t>(expected.size()),
              runtime.calls.end()) == expected,
          "prepared destination queue and cleanup order");
}

void TestRuntimeExceptionCleanup() {
    Runtime runtime;
    runtime.floats[Runtime::kFrame + 0x23cu] = 2.0f;
    runtime.fail_create = true;
    VectorRuntime renderware;
    bool caught = false;
    try {
        (void)pal4::WorldMapFlight::Advance(Runtime::kFrame, 0x8101u,
                                             runtime, renderware);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.reference_count == 1u &&
              runtime.calls[runtime.calls.size() - 2u] == "read byte" &&
              runtime.calls.back() == "write byte",
          "arrival exception releases input script name");
}

}  // namespace

int main() {
    TestFlying();
    TestCapturedQ06ToM11Rounding();
    TestArrival();
    TestPreparedDestinationQueue();
    TestRuntimeExceptionCleanup();
    return EXIT_SUCCESS;
}
