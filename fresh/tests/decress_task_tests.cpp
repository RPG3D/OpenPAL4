#include "pal4/decress_task.h"

#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(const bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

std::uint32_t Bits(const float value) {
    return std::bit_cast<std::uint32_t>(value);
}

struct ObjectRuntime final : pal4::IDecressTaskRuntime {
    std::vector<std::string> calls;
    void ConstructBase(pal4::DecressTaskState& state) override {
        calls.emplace_back("construct");
        state.words[1u] = 0x12345678u;
    }
    void DestroyBase(pal4::DecressTaskState& state) override {
        Check(state.words[0u] == pal4::DecressTask::kVtable,
              "derived vtable restored before base destructor");
        calls.emplace_back("destroy");
    }
};

struct FacetRuntime final : pal4::IDecressTaskFacetRuntime {
    std::vector<std::string> calls;
    float duration = 0.0f;
    std::uint8_t increase = 0u;
    std::uint32_t ResolveFacet(
        const std::uint32_t object,
        const std::string_view class_name) override {
        Check(object == 0x5000u && class_name == "DecressTask",
              "facet lookup uses owner and native class name");
        calls.emplace_back("resolve");
        return 0x6000u;
    }
    void StopFacet(const std::uint32_t facet) override {
        Check(facet == 0x6000u, "stop uses resolved facet");
        calls.emplace_back("stop");
    }
    void StartFacet(const std::uint32_t facet, const float time,
                    const std::uint8_t flag) override {
        Check(facet == 0x6000u, "start uses resolved facet");
        calls.emplace_back("start");
        duration = time;
        increase = flag;
    }
};

struct ConfigRuntime final : pal4::IDecressTaskConfigRuntime {
    std::map<std::string, std::int32_t> ints;
    std::map<std::string, float> floats;
    std::vector<std::string> keys;
    std::int32_t ReadInt(const std::uint32_t config,
                         const std::string_view key,
                         const std::int32_t fallback) override {
        Check(config == 0x7000u && fallback == 0,
              "integer read passes config and zero default");
        keys.emplace_back(key);
        const auto it = ints.find(std::string(key));
        return it == ints.end() ? fallback : it->second;
    }
    float ReadFloat(const std::uint32_t config,
                    const std::string_view key,
                    const float fallback) override {
        Check(config == 0x7000u && fallback == 1.0f,
              "time read passes config and one default");
        keys.emplace_back(key);
        const auto it = floats.find(std::string(key));
        return it == floats.end() ? fallback : it->second;
    }
};

}  // namespace

int main() {
    pal4::DecressTaskState state{};
    state.words.fill(0xdeadbeefu);
    ObjectRuntime object;
    auto* const original = &state;
    Check(&pal4::DecressTask::Construct(state, object) == original &&
              state.words[0u] == 0x84ac20u &&
              state.words[1u] == 0x12345678u &&
              state.words[18u] == 0u &&
              state.words[19u] == 0xdead0001u &&
              state.words[20u] == Bits(1.0f) &&
              state.words[17u] == 0xdeadbeefu &&
              pal4::DecressTask::ClassName() == "DecressTask",
          "factory object layout, native typo and constructor ordering");

    FacetRuntime facet;
    state.words[16u] = 0x5000u;
    state.words[19u] = 0x101u;
    state.words[20u] = Bits(2.5f);
    Check(pal4::DecressTask::Start(state, facet) == 1u &&
              state.words[18u] == 0x6000u &&
              facet.calls == std::vector<std::string>{"resolve", "start"} &&
              facet.duration == 2.5f && facet.increase == 1u,
          "enabled branch forwards time and increase flag");
    facet.calls.clear();
    state.words[19u] = 0x100u;
    Check(pal4::DecressTask::Start(state, facet) == 1u &&
              facet.calls == std::vector<std::string>{"resolve", "stop"},
          "disabled branch invokes facet stop");
    Check(pal4::DecressTask::Reset(state) == 1u &&
              state.words[18u] == 0u,
          "reset clears resolved facet only");

    ConfigRuntime config;
    state.words[19u] = 0xaabbccdd;
    state.words[15u] = 0xdeadbeefu;
    Check(!pal4::DecressTask::LoadConfig(state, 0u, config) &&
              config.keys.empty() && state.words[19u] == 0xaabbccddu,
          "null config preserves object");
    config.ints = {{"Attr_Task_Decress_Decress", 2},
                   {"Attr_Task_Decress_Incress", 1},
                   {"Attr_Inherit_Pre_Task", 3}};
    config.floats = {{"Attr_Task_Decress_Time", 3.25f}};
    Check(pal4::DecressTask::LoadConfig(state, 0x7000u, config) &&
              config.keys == std::vector<std::string>{
                                 "Attr_Task_Decress_Decress",
                                 "Attr_Task_Decress_Incress",
                                 "Attr_Task_Decress_Time",
                                 "Attr_Inherit_Pre_Task"} &&
              state.words[19u] == 0xaabb0101u &&
              state.words[20u] == Bits(3.25f) &&
              state.words[15u] == 0xdead01efu,
          "enabled config reads four keys in order and writes exact offsets");

    config.keys.clear();
    config.ints.clear();
    config.floats.clear();
    Check(pal4::DecressTask::LoadConfig(state, 0x7000u, config) &&
              config.keys == std::vector<std::string>{
                                 "Attr_Task_Decress_Decress",
                                 "Attr_Inherit_Pre_Task"} &&
              state.words[19u] == 0xaabb0000u &&
              state.words[20u] == 0u &&
              state.words[15u] == 0xdead00efu,
          "disabled config skips increase/time and clears them");

    state.words[0u] = 0u;
    Check(&pal4::DecressTask::Destroy(state, object) == original &&
              state.words[0u] == 0x84ac20u &&
              object.calls == std::vector<std::string>{"construct", "destroy"},
          "destructor restores derived vtable before base cleanup");
}
