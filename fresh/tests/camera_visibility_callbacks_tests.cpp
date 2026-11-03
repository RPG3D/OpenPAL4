#include "pal4/camera_visibility_callbacks.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct VisibilityRuntime final : pal4::ICameraVisibilityRuntime {
    std::map<std::uint32_t, float> floats{
        {0x3060u, 9.0f}, {0x3064u, 19.0f}, {0x3068u, 29.0f},
        {0x306cu, 1.0f}, {0x3070u, 11.0f}, {0x3074u, 21.0f}};
    std::uint32_t sector_field{0xabcdu};
    std::vector<std::string> calls;
    bool sector_found{};
    bool throw_on_process{};

    void TraverseSectors(
        const std::uint32_t camera, std::uint32_t& candidate,
        std::uint32_t (*callback)(std::uint32_t,
                                  std::uint32_t&)) override {
        Check(camera == 0x5000u && candidate == 0x3000u,
              "sector traversal inputs");
        calls.push_back("traverse");
        if (sector_found) {
            Check(callback(0x3000u, candidate) == 0u && candidate == 0u,
                  "reconstructed list-removal callback");
        }
    }
    [[nodiscard]] float ReadFloat(const std::uint32_t address) override {
        calls.push_back("float");
        Check(floats.contains(address), "sector bounds address");
        return floats[address];
    }
    [[nodiscard]] std::uint32_t ReadWord(
        const std::uint32_t address) override {
        Check(address == 0x307cu, "sector override field");
        calls.push_back("read-field");
        return sector_field;
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        Check(address == 0x307cu, "sector override write address");
        calls.push_back("write-field");
        sector_field = value;
    }
    void ProcessGlobal() override {
        calls.push_back("process:" + std::to_string(sector_field));
        if (throw_on_process) throw std::runtime_error("global processor");
    }
};

pal4::CameraRenderState CameraState() {
    pal4::CameraRenderState state;
    state.camera = 0x5000u;
    state.second_pipeline = 0x7000u;
    state.maximum[0] = 8.0f;
    state.maximum[1] = 18.0f;
    state.maximum[2] = 28.0f;
    state.minimum[0] = 2.0f;
    state.minimum[1] = 12.0f;
    state.minimum[2] = 22.0f;
    return state;
}

void TestVisibility() {
    auto state = CameraState();
    VisibilityRuntime runtime;
    Check(pal4::CameraVisibilityCallback::Process(
              0x3000u, state, runtime) == 0x3000u &&
              runtime.calls == std::vector<std::string>{
                  "traverse", "process:43981"},
          "sector absent from camera traversal skips bounds");

    runtime.calls.clear();
    runtime.sector_found = true;
    Check(pal4::CameraVisibilityCallback::Process(
              0x3000u, state, runtime) == 0x3000u &&
              runtime.sector_field == 0xabcdu,
          "overlapping sector restores pipeline field");
    Check(runtime.calls == std::vector<std::string>{
              "traverse", "float", "float", "float", "float", "float",
              "float", "read-field", "write-field", "process:28672",
              "write-field"}, "bounds and override processing order");

    runtime.calls.clear();
    runtime.floats[0x306cu] = 8.0f;
    Check(pal4::CameraVisibilityCallback::Process(
              0x3000u, state, runtime) == 0x3000u &&
              runtime.calls == std::vector<std::string>{
                  "traverse", "float", "process:43981"},
          "touching upper boundary is not overlapping");

    runtime.floats[0x306cu] = 1.0f;
    runtime.throw_on_process = true;
    try {
        (void)pal4::CameraVisibilityCallback::Process(
            0x3000u, state, runtime);
        Check(false, "processor exception should propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.sector_field == 0xabcdu,
              "sector field restored after processor exception");
    }
}

struct AtomicRuntime final : pal4::ICameraAtomicUserDataRuntime {
    std::uint32_t flags{0xaau};
    std::int32_t count{3};
    std::int32_t current_index{};
    std::int32_t match_index{-1};
    std::int32_t throw_index{-1};
    std::vector<std::uint32_t> types{1u, 3u, 3u};
    std::vector<std::string> calls;

    [[nodiscard]] std::uint32_t ReadWord(
        const std::uint32_t address) override {
        if (address == 0x3018u) {
            calls.push_back("geometry");
            return 0x4000u;
        }
        if (address == 0x4008u) {
            calls.push_back("flags");
            return flags;
        }
        Check(address == 0x5004u +
                  static_cast<std::uint32_t>(current_index) * 0x100u,
              "user-data item type offset");
        calls.push_back("type");
        return types.at(static_cast<std::size_t>(current_index));
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        Check(address == 0x4008u, "geometry flags write offset");
        calls.push_back("write-flags");
        flags = value;
    }
    [[nodiscard]] std::int32_t UsedCount(
        const std::uint32_t geometry) override {
        Check(geometry == 0x4000u, "user-data count geometry");
        calls.push_back("count");
        return count;
    }
    [[nodiscard]] std::uint32_t GetItem(
        const std::uint32_t geometry,
        const std::int32_t index) override {
        Check(geometry == 0x4000u, "user-data item geometry");
        calls.push_back("item:" + std::to_string(index));
        current_index = index;
        return 0x5000u + static_cast<std::uint32_t>(index) * 0x100u;
    }
    [[nodiscard]] std::uint32_t GetText(
        const std::uint32_t item,
        const std::int32_t index) override {
        Check(item == 0x5000u + static_cast<std::uint32_t>(index) * 0x100u,
              "user-data text item and index");
        calls.push_back("text");
        return 0x6000u + static_cast<std::uint32_t>(index) * 0x100u;
    }
    [[nodiscard]] std::uint32_t InitializeString(
        const std::uint32_t text) override {
        Check(text == 0x6000u +
                  static_cast<std::uint32_t>(current_index) * 0x100u,
              "temporary string source");
        calls.push_back("initialize-string");
        return 0x7000u;
    }
    void LowercaseString(const std::uint32_t string) override {
        Check(string == 0x7000u, "lowercase temporary string");
        calls.push_back("lowercase");
        if (current_index == throw_index)
            throw std::runtime_error("lowercase");
    }
    [[nodiscard]] std::uint32_t GetClumpManager() override {
        calls.push_back("manager");
        return 0x8000u;
    }
    [[nodiscard]] std::uint32_t FindClump(
        const std::uint32_t manager,
        const std::uint32_t string) override {
        Check(manager == 0x8000u && string == 0x7000u,
              "clump lookup arguments");
        calls.push_back("find");
        return current_index == match_index ? 0x9000u : 0u;
    }
    void CleanupString(const std::uint32_t string) override {
        Check(string == 0x7000u, "temporary string cleanup");
        calls.push_back("cleanup-string");
    }
};

void TestAtomicCallbacks() {
    AtomicRuntime runtime;
    Check(pal4::CameraAtomicUserData::ClearUnmatchedFlags(
              0x3000u, runtime) == 0x3000u && runtime.flags == 0x8au,
          "first callback clears flag bits sixty after no match");
    Check(runtime.calls == std::vector<std::string>{
              "geometry", "flags", "count", "item:0", "type", "item:1",
              "type", "text", "initialize-string", "lowercase", "manager",
              "find", "cleanup-string", "item:2", "type", "text",
              "initialize-string", "lowercase", "manager", "find",
              "cleanup-string", "write-flags"},
          "first callback filters types and cleans every temporary");

    runtime.calls.clear();
    runtime.flags = 0x82u;
    Check(pal4::CameraAtomicUserData::SetUnmatchedFlags(
              0x3000u, runtime) == 0x3000u && runtime.flags == 0xe2u &&
              runtime.calls.back() == "write-flags",
          "second callback sets flag bits sixty after no match");

    runtime.calls.clear();
    runtime.flags = 0xaau;
    runtime.match_index = 1;
    Check(pal4::CameraAtomicUserData::ClearUnmatchedFlags(
              0x3000u, runtime) == 0x3000u && runtime.flags == 0xaau &&
              runtime.calls.back() == "cleanup-string",
          "matching item exits without changing geometry flags");

    runtime.calls.clear();
    runtime.match_index = -1;
    runtime.throw_index = 1;
    try {
        (void)pal4::CameraAtomicUserData::SetUnmatchedFlags(
            0x3000u, runtime);
        Check(false, "lowercase exception should propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.flags == 0xaau &&
                  runtime.calls.back() == "cleanup-string",
              "temporary string cleaned on failure before flag write");
    }

    runtime.calls.clear();
    runtime.throw_index = -1;
    runtime.count = 0;
    Check(pal4::CameraAtomicUserData::SetUnmatchedFlags(
              0x3000u, runtime) == 0x3000u && runtime.flags == 0xeau &&
              runtime.calls == std::vector<std::string>{
                  "geometry", "flags", "count", "write-flags"},
          "empty user-data array still updates flags");
}

}  // namespace

int main() {
    TestVisibility();
    TestAtomicCallbacks();
}
