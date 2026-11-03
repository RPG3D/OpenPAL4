#include "pal4/clump_object_update.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IClumpObjectUpdateRuntime {
    std::vector<std::string> calls;
    std::array<std::uint8_t,4> sample{0x12u, 0x34u, 0x56u, 5u};
    std::int32_t converted = 2;
    double conversion_input = 0.0;
    std::array<std::uint8_t, 4> bytes{};
    std::uint32_t hierarchy = 0x2000u;
    std::map<std::uint32_t, std::uint32_t> words{
        {0x1014u, 0x1800u}, {0x2020u, 0x3000u},
        {0x3000u, 0x4000u}};
    std::map<std::uint32_t, float> floats{
        {0x400cu, 3.0f}, {0x3004u, 1.0f}};
    float add_amount = 0.0f;

    std::array<std::uint8_t,4> QueryTimedFacet(
        pal4::ClumpObjectState&, std::uint32_t offset) override {
        Check(offset == 36u, "timed color facet offset");
        calls.push_back("query");
        return sample;
    }
    std::int32_t Ftol(double value) override {
        calls.push_back("ftol");
        conversion_input = value;
        return converted;
    }
    void ApplyTimedFacet(pal4::ClumpObjectState&, std::uint32_t offset,
                         const std::array<std::uint8_t, 4>& value) override {
        Check(offset == 36u, "apply uses same facet");
        calls.push_back("apply");
        bytes = value;
    }
    void UpdateManager(std::uint32_t manager, float delta) override {
        Check(manager == 0x1000u && delta == 0.5f,
              "manager receives converted frame time");
        calls.push_back("manager");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("word:" + std::to_string(address));
        return words.at(address);
    }
    float ReadFloat(std::uint32_t address) override {
        calls.push_back("float:" + std::to_string(address));
        return floats.at(address);
    }
    std::uint32_t FindSkinHierarchy(std::uint32_t clump) override {
        Check(clump == 0x1800u, "hierarchy search receives manager clump");
        calls.push_back("hierarchy");
        return hierarchy;
    }
    void AddAnimationTime(std::uint32_t interpolator,
                          float amount) override {
        Check(interpolator == 0x3000u, "interpolator for add time");
        calls.push_back("add");
        add_amount = amount;
    }
    void SetAnimationTime(std::uint32_t interpolator,
                          float time) override {
        Check(interpolator == 0x3000u && time == 0.0f,
              "wrap resets interpolator time");
        calls.push_back("set");
    }
    void UpdateHierarchyMatrices(std::uint32_t value) override {
        Check(value == 0x2000u, "matrix update receives hierarchy");
        calls.push_back("matrices");
    }
};

pal4::ClumpObjectState ActiveState() {
    pal4::ClumpObjectState state{};
    state.words[1u] = 0x00010001u;
    state.words[18u] = 0x1000u;
    return state;
}

void TestGatesAndTimedFacet() {
    auto state = ActiveState();
    Runtime runtime;
    state.words[1u] &= 0xffffff00u;
    pal4::ClumpObject::Update(state, 0.5, runtime);
    Check(runtime.calls.empty(), "inactive update has no callbacks");
    state.words[1u] = 1u;
    pal4::ClumpObject::Update(state, 0.5, runtime);
    Check(runtime.calls.empty(), "byte +6 gates update");

    state = ActiveState();
    pal4::ClumpObject::Update(state, 0.5, runtime);
    Check(runtime.calls == std::vector<std::string>{"manager"},
          "no timed mode or animation updates only manager");

    runtime.calls.clear();
    state.words[2u] = 0x01000000u;
    state.words[43u] = std::bit_cast<std::uint32_t>(2.0f);
    runtime.converted = 4;
    pal4::ClumpObject::Update(state, 0.5, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "query", "ftol", "apply", "manager"} &&
              runtime.conversion_input == 4.0 &&
              runtime.bytes == std::array<std::uint8_t, 4>{
                  0x12u, 0x34u, 0x56u, 4u},
          "timed facet preserves RGB while fading alpha");

    runtime.calls.clear();
    runtime.converted = -5;
    pal4::ClumpObject::Update(state, 0.5, runtime);
    Check(runtime.bytes == std::array<std::uint8_t,4>{0x12u,0x34u,0x56u,0u} && runtime.calls ==
              std::vector<std::string>{
                  "query", "ftol", "apply", "manager"},
          "negative adjusted count clamps to zero");
}

void TestAnimation() {
    auto state = ActiveState();
    state.words[19u] = 0x5000u;
    Runtime runtime;
    pal4::ClumpObject::Update(state, 0.5, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "manager", "word:4116", "hierarchy", "word:8224",
              "word:12288", "float:16396", "float:12292", "add",
              "matrices"} && runtime.add_amount == 0.5f,
          "animation below duration adds full frame time");

    runtime.calls.clear();
    runtime.floats[0x3004u] = 2.5f;
    pal4::ClumpObject::Update(state, 0.5, runtime);
    Check(runtime.calls.back() == "matrices" &&
              runtime.calls[runtime.calls.size() - 2u] == "set" &&
              runtime.add_amount == 0.0f,
          "duration boundary adds remainder then resets time");

    runtime.calls.clear();
    state.words[42u] = 1u;
    pal4::ClumpObject::Update(state, 0.5, runtime);
    Check(runtime.calls == std::vector<std::string>{"manager"},
          "byte +168 pauses animation while manager still updates");

    runtime.calls.clear();
    state.words[42u] = 0u;
    runtime.hierarchy = 0u;
    pal4::ClumpObject::Update(state, 0.5, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "manager", "word:4116", "hierarchy"},
          "missing hierarchy skips animation operations");
}

}  // namespace

int main() {
    TestGatesAndTimedFacet();
    TestAnimation();
}
