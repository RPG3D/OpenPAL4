#include "pal4/clump_object_control.h"
#include "pal4/clump_object_config.h"

#include <array>
#include <bit>
#include <cstdint>
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

struct ResetRuntime final : pal4::ClumpObjectConfigRuntimeStub {
    std::vector<std::string> calls;
    std::uint32_t hierarchy = 0u;

    void Rotate(std::uint32_t manager, const std::array<float, 3>& vector,
                std::uint32_t mode) override {
        Check(manager == 0x1000u && mode == 0u &&
                  vector == std::array<float, 3>{1.0f, 2.0f, 3.0f},
              "reset uses cached rotation");
        calls.push_back("rotate");
    }
    void Scale(std::uint32_t manager, const std::array<float, 3>& vector,
               std::uint32_t mode) override {
        Check(manager == 0x1000u && mode == 0u &&
                  vector == std::array<float, 3>{4.0f, 5.0f, 6.0f},
              "reset uses cached scale");
        calls.push_back("scale");
    }
    void Translate(std::uint32_t manager, const std::array<float, 3>& vector,
                   std::uint32_t mode) override {
        Check(manager == 0x1000u && mode == 0u &&
                  vector == std::array<float, 3>{7.0f, 8.0f, 9.0f},
              "reset uses cached position");
        calls.push_back("translate");
    }
    void SetColor(std::uint32_t manager, std::uint32_t color) override {
        Check(manager == 0x1000u && color == 0x12345678u,
              "reset uses cached color");
        calls.push_back("color");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return address == 0x1014u ? 0x2000u : 0x4000u;
    }
    std::uint32_t FindHierarchy(std::uint32_t) override {
        Check(false,"reset must query bound skin hierarchy instead of frame hierarchy");return 0u;
    }
    std::uint32_t FindSkinHierarchy(std::uint32_t clump) override {
        Check(clump == 0x2000u, "reset enumerates manager clump");
        calls.push_back("hierarchy");
        return hierarchy;
    }
    void SetCurrentTime(std::uint32_t interpolator, float time) override {
        Check(interpolator == 0x4000u && time == 0.0f,
              "reset zeroes hierarchy interpolator time");
        calls.push_back("time");
    }
};

void TestReset() {
    pal4::ClumpObjectState state{};
    ResetRuntime runtime;
    state.words[18u] = 0x1000u;
    state.words[67u] = 0x12345678u;
    for (std::uint32_t i = 0u; i < 9u; ++i)
        state.words[68u + i] = std::bit_cast<std::uint32_t>(float(i + 1u));
    state.words[2u] = 0xff000000u;
    Check(pal4::ClumpObject::Reset(state, runtime) == 0u &&
              runtime.calls.empty() && state.words[2u] == 0xff000000u,
          "inactive reset is a no-op");

    state.words[1u] = 1u;
    Check(pal4::ClumpObject::Reset(state, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "rotate", "scale", "translate", "color",
                  "read:4116", "hierarchy"} &&
              state.words[2u] == 0x00000000u,
          "reset order and absent hierarchy");
    runtime.calls.clear();
    runtime.hierarchy = 0x3000u;
    state.words[2u] = 0xaa123456u;
    Check(pal4::ClumpObject::Reset(state, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "rotate", "scale", "translate", "color",
                  "read:4116", "hierarchy", "read:12320", "time"} &&
              state.words[2u] == 0x00123456u,
          "hierarchy time and only timed byte cleared");
}

struct StopRuntime final : pal4::IClumpObjectControlRuntime {
    std::vector<std::string> calls;

    void StopFacet(pal4::ClumpObjectState& state,
                   std::uint32_t offset) override {
        Check((state.words[1u] & 0xffu) == 1u,
              "facet runs before active byte clears");
        calls.push_back("facet:" + std::to_string(offset));
    }
    void CleanupXml(pal4::ClumpObjectState&, std::uint32_t offset) override {
        calls.push_back("xml:" + std::to_string(offset));
    }
    void AssignEmptyText(pal4::ClumpObjectState&, std::uint32_t offset) override {
        calls.push_back("text:" + std::to_string(offset));
    }
    std::uint32_t SystemInstance() override {
        calls.push_back("system");
        return 0x5000u;
    }
    void CleanupClump(std::uint32_t manager, std::uint32_t world) override {
        Check(manager == 0x1000u && world == 0x5034u,
              "clump cleanup uses system world");
        calls.push_back("clump");
    }
    void CleanupTexture(std::uint32_t manager) override {
        Check(manager == 0x1000u, "texture cleanup manager");
        calls.push_back("texture");
    }
    void DeleteManager(std::uint32_t manager, std::uint32_t flag) override {
        Check(manager == 0x1000u && flag == 1u,
              "manager deleting destructor");
        calls.push_back("manager");
    }
    void FreeAnimation(std::uint32_t animation) override {
        Check(animation == 0x2000u, "animation release address");
        calls.push_back("animation");
    }
    void NotifyOwner(std::uint32_t owner,
                     pal4::ClumpObjectState& state) override {
        Check(owner == 0x3000u &&
                  (state.words[1u] & 0xff00u) == 0x100u,
              "owner notified before byte +5 clears");
        calls.push_back("notify");
    }
    std::uint32_t DeleteObject(pal4::ClumpObjectState& state,
                               std::uint32_t flag) override {
        Check(flag == 1u && (state.words[1u] & 0xffu) == 0u,
              "object deleting destructor flag and state");
        calls.push_back("delete");
        return 0xabcdef01u;
    }
};

void TestStop() {
    pal4::ClumpObjectState state{};
    StopRuntime runtime;
    state.words.fill(0xffffffffu);
    state.words[1u] = 0x101u;
    state.words[4u] = 0x3000u;
    state.words[18u] = 0x1000u;
    state.words[19u] = 0x2000u;
    Check(pal4::ClumpObject::StopAndDestroy(state, runtime) ==
              0xabcdef01u &&
              runtime.calls == std::vector<std::string>{
                  "facet:60", "facet:40", "xml:108", "text:220",
                  "text:236", "text:252", "system", "clump",
                  "texture", "manager", "animation", "notify", "delete"},
          "active stop follows native cleanup and deletion order");
    Check(state.words[1u] == 0u && state.words[17u] == 0xffffffffu &&
              state.words[18u] == 0u && state.words[19u] == 0u &&
              state.words[20u] == 0u && state.words[21u] == 0x3f800000u &&
              state.words[22u] == 0x3f800000u && state.words[23u] == 0u &&
              state.words[24u] == 0u && state.words[25u] == 0u &&
              state.words[26u] == 0xffffff00u &&
              state.words[42u] == 0xffffff00u &&
              state.words[52u] == 0xff000000u &&
              state.words[53u] == 0u && state.words[54u] == 0u,
          "stop resets exact byte and word fields");

    pal4::ClumpObjectState inactive{};
    runtime.calls.clear();
    Check(pal4::ClumpObject::StopAndDestroy(inactive, runtime) ==
              0xabcdef01u && runtime.calls == std::vector<std::string>{
                  "delete"},
          "inactive object still invokes deleting destructor");

    pal4::ClumpObjectState empty{};
    empty.words[1u] = 0x101u;
    empty.words[4u] = 0x3000u;
    runtime.calls.clear();
    Check(pal4::ClumpObject::StopAndDestroy(empty, runtime) ==
              0xabcdef01u &&
              runtime.calls == std::vector<std::string>{
                  "facet:60", "facet:40", "xml:108", "text:220",
                  "text:236", "text:252", "notify", "delete"},
          "active stop skips absent manager and animation");
}

}  // namespace

int main() {
    TestReset();
    TestStop();
}
