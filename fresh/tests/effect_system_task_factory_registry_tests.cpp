#include "pal4/effect_system_task_factory_registry.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

using Descriptor = std::pair<std::string_view, std::uint32_t>;
constexpr std::array<Descriptor, 24u> kVerifiedFactories{{
    {"ShowobjTask", 0x640960u},
    {"HideobjTask", 0x6409c0u},
    {"ScaleTask", 0x640a20u},
    {"PositionTask", 0x640a80u},
    {"RotateTask", 0x640ae0u},
    {"RevoluteTask", 0x640b40u},
    {"ChangeColorTask", 0x640ba0u},
    {"PlaySound3DTask", 0x640c00u},
    {"StopSound3DTask", 0x640c60u},
    {"UVRotateTask", 0x640fc0u},
    {"UVTranslateTask", 0x641020u},
    {"PlayEffectTask", 0x640cc0u},
    {"PauseEffectTask", 0x640d80u},
    {"RewindEffectTask", 0x640de0u},
    {"StopEffectTask", 0x640d20u},
    {"TrackTask", 0x640e40u},
    {"StartCameraTask", 0x640ea0u},
    {"StopCameraTask", 0x640f00u},
    {"RumpShahdowTask", 0x640f60u},
    {"DecressTask", 0x641080u},
    {"MoveToTargetTask", 0x6410e0u},
    {"WobbleTask", 0x641140u},
    {"MessageTask", 0x6411a0u},
    {"SlowMotionTask", 0x6412e0u},
}};

struct Runtime final : pal4::IEffectSystemTaskFactoryRegistryRuntime {
    static constexpr std::uint32_t kSystem = 0x2000u;
    static constexpr std::uint32_t kTree = kSystem + 8u;
    std::uint32_t count = 0u;
    bool duplicate_hide = false;
    std::vector<std::string> calls;
    std::vector<Descriptor> inserted;
    std::vector<Descriptor> helper;
    std::vector<std::string_view> released;
    std::vector<std::string_view> searched;

    std::uint32_t RegisterEffectSystem() override {
        calls.emplace_back("register");
        return kSystem;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == kSystem + 20u, "registration count offset");
        calls.emplace_back("count");
        return count;
    }
    void ClearRegistry(std::uint32_t tree) override {
        Check(tree == kTree, "clear targets embedded task registry");
        calls.emplace_back("clear");
    }
    bool HasFactory(std::uint32_t tree,
                    std::string_view name) override {
        Check(tree == kTree, "manual search targets task registry");
        searched.push_back(name);
        return duplicate_hide && name == "HideobjTask";
    }
    void InsertFactory(std::uint32_t tree,
                       std::string_view name,
                       std::uint32_t creator) override {
        Check(tree == kTree, "manual insertion targets task registry");
        inserted.emplace_back(name, creator);
    }
    void ProcessEvent(std::uint32_t tree,
                      std::string_view name,
                      std::uint32_t creator) override {
        Check(tree == kTree, "event helper targets task registry");
        helper.emplace_back(name, creator);
    }
    std::uint8_t ReleaseName(std::string_view name) override {
        released.push_back(name);
        return name == "SlowMotionTask" ? 0x5au : 0u;
    }
};

void TestDescriptors() {
    const auto& actual = pal4::EffectSystemTaskFactoryRegistry::kFactories;
    for (std::size_t index = 0u; index < actual.size(); ++index) {
        Check(actual[index].name == kVerifiedFactories[index].first &&
                  actual[index].creator == kVerifiedFactories[index].second &&
                  actual[index].via_event_helper == (index >= 11u),
              "IDA task names, creator addresses and helper boundary");
        Check(actual[index].name != "LocalModelTask",
              "unreferenced LocalModelTask string is not registered");
    }
}

void TestRegistrationPaths() {
    Runtime runtime;
    runtime.count = 2u;
    runtime.duplicate_hide = true;
    Check(pal4::EffectSystemTaskFactoryRegistry::Register(runtime) == 0x5au &&
              runtime.calls == std::vector<std::string>{
                  "register", "count", "clear"} &&
              runtime.searched.size() == 11u &&
              runtime.inserted.size() == 10u &&
              runtime.helper.size() == 13u &&
              runtime.released.size() == 24u,
          "populated registry clears, manually checks first eleven and delegates thirteen");
    Check(runtime.inserted[0] == kVerifiedFactories[0] &&
              runtime.inserted[1] == kVerifiedFactories[2] &&
              runtime.helper.front() == kVerifiedFactories[11] &&
              runtime.helper.back() == kVerifiedFactories[23] &&
              runtime.released.back() == "SlowMotionTask",
          "duplicate does not disturb later order or final cleanup return");
    for (std::size_t index = 0u; index < 24u; ++index)
        Check(runtime.released[index] == kVerifiedFactories[index].first,
              "temporary names are released in registration order");

    Runtime empty;
    Check(pal4::EffectSystemTaskFactoryRegistry::Register(empty) == 0x5au &&
              empty.calls == std::vector<std::string>{
                  "register", "count"} &&
              empty.inserted.size() == 11u &&
              empty.helper.size() == 13u,
          "zero-count registry does not clear before registration");
}

}  // namespace

int main() {
    TestDescriptors();
    TestRegistrationPaths();
}
