#include "pal4/rw_plugin_8450.h"

#include <cstdint>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IRwPlugin8450Runtime {
public:
    std::int32_t RegisterEngine(
        std::uint32_t size, std::uint32_t id,
        pal4::RwPlugin8450Callback construct,
        pal4::RwPlugin8450Callback destruct) override {
        using C = pal4::RwPlugin8450Callback;
        Check(size == 8 && id == 8450 && construct == C::EngineConstruct &&
                  destruct == C::EngineDestruct, "engine registration args");
        calls.push_back("engine");
        return engine_result;
    }
    std::int32_t RegisterAtomic(
        std::uint32_t size, std::uint32_t id,
        pal4::RwPlugin8450Callback construct,
        pal4::RwPlugin8450Callback destruct,
        pal4::RwPlugin8450Callback copy) override {
        using C = pal4::RwPlugin8450Callback;
        Check(size == 12 && id == 8450 && construct == C::AtomicConstruct &&
                  destruct == C::AtomicDestruct && copy == C::AtomicCopy,
              "atomic registration args");
        calls.push_back("atomic");
        return atomic_result;
    }
    std::int32_t RegisterAtomicStream(
        std::uint32_t id, pal4::RwPlugin8450Callback read,
        pal4::RwPlugin8450Callback write,
        pal4::RwPlugin8450Callback size) override {
        using C = pal4::RwPlugin8450Callback;
        Check(id == 8450 && read == C::StreamReadIdentity &&
                  write == C::StreamWriteIdentity && size == C::StreamSize,
              "stream registration args");
        calls.push_back("stream");
        return stream_result;
    }
    std::uint32_t EngineBase() override {
        calls.push_back("base");
        return 0x5000;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words[address];
    }
    std::uint32_t RenderCallbackAddress() override {
        calls.push_back("render_callback");
        return render_callback;
    }
    std::uint32_t DefaultRenderCallbackAddress() override {
        calls.push_back("default_callback");
        return 0x8888;
    }
    std::uint32_t ForAllAtomics(
        std::uint32_t clump, pal4::RwPlugin8450Callback callback,
        std::uint32_t user_data) override {
        Check(clump == 0x900 && user_data == 0,
              "clump traversal args");
        if (callback == pal4::RwPlugin8450Callback::ProcessAtomic)
            calls.push_back("atomics");
        else if (callback == pal4::RwPlugin8450Callback::ResetAtomic)
            calls.push_back("reset_atomics");
        else
            Check(false, "clump callback identity");
        return 0xabcu;
    }
    void ForAllAtomicsWithOutput(
        std::uint32_t clump, pal4::RwPlugin8450Callback callback,
        std::uint32_t& output) override {
        Check(clump == 0x900 &&
                  callback == pal4::RwPlugin8450Callback::QueryAtomic,
              "clump output traversal args");
        calls.push_back("query_atomics");
        output = query_output;
    }
    void ResyncAtomic(std::uint32_t atomic) override {
        Check(atomic == 0x200, "resync atomic");
        calls.push_back("resync");
    }
    std::uint32_t ObjectTransform(std::uint32_t object) override {
        calls.push_back("transform:" + std::to_string(object));
        return object + 0x100;
    }
    std::array<float, 3> CalculatePosition(
        std::uint32_t center, std::uint32_t transform) override {
        Check(center == 0x21c && transform == 0xd00,
              "sphere center and object transform");
        calls.push_back("position");
        return {10.0f, 20.0f, 30.0f};
    }
    double ApproximateMagnitude(const std::array<float, 3>& delta) override {
        Check(delta == std::array<float, 3>{9.0f, 18.0f, 27.0f},
              "camera-relative delta");
        calls.push_back("magnitude");
        return distance;
    }
    std::int32_t FloatToLong(double value) override {
        Check(std::abs(value - 128.0) < 0.1, "fade scaling");
        calls.push_back("ftol");
        return 126;
    }
    void InvokeCallback(std::uint32_t callback,
                        std::uint32_t atomic) override {
        Check(callback == 0x1234 && atomic == 0x200,
              "saved render callback invocation");
        calls.push_back("invoke");
    }

    std::int32_t engine_result = 8;
    std::int32_t atomic_result = 12;
    std::int32_t stream_result = 12;
    std::uint32_t render_callback = 0x9999;
    double distance = 75.0;
    std::uint32_t query_output = 1;
    std::vector<std::string> calls;
    std::map<std::uint32_t, std::uint32_t> words;
};

}  // namespace

int main() {
    Runtime runtime;
    pal4::RwPlugin8450State state;
    Check(pal4::RwPlugin8450::Register(state, runtime) &&
              state.engine_offset == 8 && state.atomic_offset == 12 &&
              runtime.calls == std::vector<std::string>{
                  "engine", "atomic", "stream"},
          "successful registration order and stored offsets");
    runtime.calls.clear();
    runtime.engine_result = -1;
    Check(!pal4::RwPlugin8450::Register(state, runtime) &&
              state.engine_offset == -1 && state.atomic_offset == 12 &&
              runtime.calls == std::vector<std::string>{"engine"},
          "engine failure stops before atomic");
    runtime.calls.clear();
    runtime.engine_result = 8;
    runtime.atomic_result = -1;
    Check(!pal4::RwPlugin8450::Register(state, runtime) &&
              state.atomic_offset == -1 &&
              runtime.calls == std::vector<std::string>{
                  "engine", "atomic"},
          "atomic failure stops before stream");
    runtime.calls.clear();
    runtime.atomic_result = 12;
    runtime.stream_result = 13;
    Check(!pal4::RwPlugin8450::Register(state, runtime) &&
              runtime.calls == std::vector<std::string>{
                  "engine", "atomic", "stream"},
          "stream offset mismatch fails");

    runtime.calls.clear();
    Check(pal4::RwPlugin8450::EngineConstruct(0x100, state, runtime) == 0x100 &&
              runtime.words[0x5008] == 0 && runtime.words[0x500c] == 1 &&
              runtime.calls == std::vector<std::string>{
                  "base", "write:20488", "write:20492"},
          "engine constructor writes global engine plugin words");
    runtime.calls.clear();
    Check(pal4::RwPlugin8450::EngineDestruct(0x100, state, runtime) == 0x100 &&
              runtime.words[0x500c] == 0,
          "engine destructor clears second plugin word");
    runtime.calls.clear();
    state.engine_offset = 0;
    Check(pal4::RwPlugin8450::EngineConstruct(0x100, state, runtime) == 0x100 &&
              runtime.calls.empty(), "zero engine offset bypasses writes");

    runtime.calls.clear();
    Check(pal4::RwPlugin8450::AtomicConstruct(0x200, state, runtime) ==
                  0x200 && runtime.words[0x20c] == 0 &&
                  runtime.words[0x210] == 0 && runtime.words[0x214] == 1,
          "atomic constructor initializes twelve bytes");
    runtime.words[0x214] = 7;
    Check(pal4::RwPlugin8450::AtomicDestruct(0x200, state, runtime) ==
                  0x200 && runtime.words[0x214] == 1,
          "atomic destructor restores same defaults");
    runtime.calls.clear();
    runtime.words[0x30c] = 9;
    runtime.words[0x310] = 10;
    runtime.words[0x314] = 11;
    Check(pal4::RwPlugin8450::AtomicCopy(0x200, 0x300, state, runtime) ==
                  0x200 && runtime.words[0x20c] == 9 &&
                  runtime.words[0x210] == 10 && runtime.words[0x214] == 11 &&
                  runtime.calls == std::vector<std::string>{
                      "read:780", "write:524", "read:784", "write:528",
                      "read:788", "write:532"},
          "copy preserves all three words in read/write order");
    runtime.calls.clear();
    state.atomic_offset = 0;
    Check(pal4::RwPlugin8450::AtomicConstruct(0x200, state, runtime) ==
                  0x200 && runtime.calls.empty(),
          "zero atomic offset bypasses constructor");

    Check(pal4::RwPlugin8450::ProcessClump(0, runtime) == 0 &&
              runtime.calls.empty(), "null clump bypasses traversal");
    Check(pal4::RwPlugin8450::ProcessClump(0x900, runtime) == 0xabc &&
              runtime.calls == std::vector<std::string>{"atomics"},
          "clump traversal result forwarded");

    runtime.calls.clear();
    state.atomic_offset = 12;
    state.engine_offset = 8;
    Check(pal4::RwPlugin8450::SetPluginData(
              0x200, 0x1234, state, runtime) == 0x200 &&
              runtime.words[0x20c] == 0x1234 &&
              runtime.words[0x210] == 1 && runtime.words[0x214] == 1,
          "plugin data and flags stored at atomic offset");
    runtime.calls.clear();
    state.engine_offset = 0;
    Check(pal4::RwPlugin8450::SetPluginData(
              0x200, 9, state, runtime) == 0 && runtime.calls.empty(),
          "engine offset gate returns zero without writes");
    state.atomic_offset = 0;
    Check(!pal4::RwPlugin8450::SetPluginData(
               0x200, 9, state, runtime).has_value(),
          "atomic offset gate has undefined native return");

    runtime.calls.clear();
    runtime.words[0x218] = 0x400;
    runtime.words[0x408] = 8;
    runtime.words[0x424] = 2;
    runtime.words[0x420] = 0x600;
    runtime.words[0x600] = 0x700;
    runtime.words[0x604] = 0x800;
    Check(pal4::RwPlugin8450::SetMaterialColor(
              0x200, 0xaabbccdd, runtime) == 2 &&
              runtime.words[0x408] == 0x48 &&
              runtime.words[0x704] == 0xaabbccdd &&
              runtime.words[0x804] == 0xaabbccdd,
          "geometry flag and all material colors updated");
    runtime.words[0x424] = 0xffffffffu;
    runtime.calls.clear();
    Check(pal4::RwPlugin8450::SetMaterialColor(
              0x200, 7, runtime) == 0 &&
              runtime.words[0x408] == 0x48,
          "nonpositive material count still sets geometry flag");

    runtime.words[0x424] = 2;
    runtime.words[0x408] = 8;
    runtime.words[0x248] = 0x1234;
    runtime.words[0x210] = 0;
    state.engine_offset = 8;
    state.atomic_offset = 12;
    runtime.calls.clear();
    Check(pal4::RwPlugin8450::ProcessAtomic(
              0x200, state, runtime) == 0x200 &&
              runtime.words[0x20c] == 0x1234 &&
              runtime.words[0x210] == 1 &&
              runtime.words[0x248] == 0x9999 &&
              runtime.words[0x704] == 0xffffffffu,
          "atomic callback saves old callback and makes materials white");
    runtime.calls.clear();
    Check(pal4::RwPlugin8450::ProcessAtomic(
              0x200, state, runtime) == 0x200 &&
              runtime.calls == std::vector<std::string>{
                  "read:584", "render_callback"},
          "already installed render callback bypasses updates");
    runtime.words[0x248] = 0x1234;
    runtime.render_callback = 0;
    runtime.words[0x210] = 0;
    runtime.calls.clear();
    Check(pal4::RwPlugin8450::ProcessAtomic(
              0x200, state, runtime) == 0x200 &&
              runtime.words[0x248] == 0x8888,
          "missing plugin render callback selects default callback");

    runtime.calls.clear();
    Check(pal4::RwPlugin8450::GetPluginData(0x200, state, runtime) ==
                  0x1234 &&
              runtime.calls == std::vector<std::string>{"read:524"},
          "plugin getter reads saved callback");
    runtime.calls.clear();
    state.engine_offset = 0;
    Check(pal4::RwPlugin8450::GetPluginData(0x200, state, runtime) == 0 &&
              runtime.calls.empty(), "plugin getter requires both offsets");
    state.engine_offset = 8;

    runtime.words[0x24c] = 2;
    runtime.words[0x204] = 0xc00;
    runtime.words[0x5000] = 0x900;
    runtime.words[0x904] = 0xa00;
    runtime.words[0xb30] = std::bit_cast<std::uint32_t>(1.0f);
    runtime.words[0xb34] = std::bit_cast<std::uint32_t>(2.0f);
    runtime.words[0xb38] = std::bit_cast<std::uint32_t>(3.0f);
    runtime.calls.clear();
    Check(pal4::RwPlugin8450::CalculateDistanceToCamera(
              0x200, runtime) == 75.0 &&
              runtime.calls[1] == "resync" &&
              runtime.calls.back() == "magnitude",
          "distance helper resyncs and delegates approximate magnitude");

    runtime.words[0x5008] = std::bit_cast<std::uint32_t>(100.0f);
    runtime.words[0x500c] = 1;
    runtime.words[0x214] = 1;
    runtime.distance = 110.0;
    runtime.calls.clear();
    Check(pal4::RwPlugin8450::RenderAtomic(0x200, state, runtime) ==
                  0x200 && runtime.words[0x214] == 0 &&
              runtime.calls.back() == "write:532",
          "beyond limit clears flag and skips original callback");

    runtime.distance = 75.0;
    runtime.calls.clear();
    Check(pal4::RwPlugin8450::RenderAtomic(0x200, state, runtime) ==
                  0x200 && runtime.words[0x704] == 0x7e7e7e7eu &&
              runtime.words[0x214] == 0 &&
              runtime.calls.back() == "invoke",
          "fade interval writes grayscale and invokes old callback");

    runtime.distance = 40.0;
    runtime.calls.clear();
    Check(pal4::RwPlugin8450::RenderAtomic(0x200, state, runtime) ==
                  0x200 && runtime.words[0x704] == 0xffffffffu &&
              runtime.words[0x214] == 1 &&
              runtime.calls.back() == "invoke",
          "near interval writes white once");
    runtime.calls.clear();
    Check(pal4::RwPlugin8450::RenderAtomic(0x200, state, runtime) ==
                  0x200 && runtime.words[0x214] == 1 &&
              runtime.calls.back() == "invoke",
          "near interval skips repeat color write");

    runtime.words[0x500c] = 0;
    runtime.calls.clear();
    Check(pal4::RwPlugin8450::RenderAtomic(0x200, state, runtime) ==
                  0x200 &&
              runtime.calls == std::vector<std::string>{
                  "base", "read:20492", "read:524", "invoke"},
          "disabled distance gate calls saved callback directly");

    runtime.calls.clear();
    Check(pal4::RwPlugin8450::ResetClump(0, runtime) == 0 &&
              runtime.calls.empty(), "null reset clump bypasses traversal");
    Check(pal4::RwPlugin8450::ResetClump(0x900, runtime) == 0xabc &&
              runtime.calls == std::vector<std::string>{"reset_atomics"},
          "reset clump uses reset callback");
    runtime.words[0x210] = 1;
    runtime.words[0x214] = 1;
    runtime.words[0x20c] = 0x1234;
    runtime.calls.clear();
    Check(pal4::RwPlugin8450::ResetAtomic(0x200, state, runtime) ==
                  0x200 && runtime.words[0x248] == 0x1234 &&
              runtime.words[0x210] == 0 && runtime.words[0x214] == 0 &&
              runtime.words[0x704] == 0xffffffffu,
          "reset restores original callback and clears plugin flags");
    runtime.calls.clear();
    Check(pal4::RwPlugin8450::ResetAtomic(0x200, state, runtime) ==
                  0x200 && runtime.calls ==
                  std::vector<std::string>{"read:528"},
          "already reset atomic is unchanged");
    state.engine_offset = 0;
    runtime.words[0x210] = 1;
    runtime.calls.clear();
    Check(pal4::RwPlugin8450::ResetAtomic(0x200, state, runtime) ==
                  0x200 && runtime.words[0x248] == 0x8888,
          "missing saved callback uses default render callback");
    state.engine_offset = 8;

    runtime.words[0x210] = 0;
    std::uint32_t output = 77;
    Check(pal4::RwPlugin8450::QueryAtomic(
              0x200, output, state, runtime) == 0 && output == 1,
          "unhandled atomic reports one and stops traversal");
    runtime.words[0x210] = 1;
    runtime.words[0x214] = 0;
    Check(pal4::RwPlugin8450::QueryAtomic(
              0x200, output, state, runtime) == 0x200 && output == 0,
          "handled visible atomic keeps traversal running");
    runtime.words[0x214] = 1;
    Check(pal4::RwPlugin8450::QueryAtomic(
              0x200, output, state, runtime) == 0 && output == 1,
          "handled hidden atomic stops traversal");
    runtime.calls.clear();
    Check(pal4::RwPlugin8450::QueryClump(0, runtime) == 0 &&
              runtime.calls.empty(), "null clump query bypass");
    Check(pal4::RwPlugin8450::QueryClump(0x900, runtime) == 1 &&
              runtime.calls == std::vector<std::string>{"query_atomics"},
          "clump query returns callback output");
    runtime.calls.clear();
    Check(pal4::RwPlugin8450::QueryDirect(0, state, runtime) == 0 &&
              runtime.calls.empty(), "null direct query bypass");
    Check(pal4::RwPlugin8450::QueryDirect(0x200, state, runtime) == 1 &&
              runtime.calls == std::vector<std::string>{
                  "read:528", "read:532"},
          "direct query treats input as atomic without traversal");

    runtime.calls.clear();
    Check(pal4::RwPlugin8450::SetEngineFloat(12.5f, state, runtime) ==
                  0x5000 && runtime.words[0x5008] ==
                  std::bit_cast<std::uint32_t>(12.5f) &&
              runtime.calls == std::vector<std::string>{
                  "base", "write:20488"},
          "engine float setter writes raw bits and returns global base");
    runtime.calls.clear();
    Check(pal4::RwPlugin8450::SetEngineInt(37, state, runtime) == 37 &&
              runtime.words[0x500c] == 37 &&
              runtime.calls == std::vector<std::string>{
                  "base", "write:20492"},
          "engine integer setter writes second word and returns input");
}
