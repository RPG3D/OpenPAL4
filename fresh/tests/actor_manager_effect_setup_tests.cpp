#include "pal4/actor_manager_effect_setup.h"

#include <array>
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

constexpr std::uint32_t actor = 0x1000u;
constexpr std::uint32_t context = 0x2000u;
constexpr std::uint32_t child = 0x3000u;

struct Runtime final : pal4::IActorManagerEffectSetupRuntime {
    std::vector<std::string> trace;
    std::map<std::uint32_t, pal4::ActorEffectHandle> handles;
    std::uint32_t child_value = child;
    std::uint32_t model_id = 2592u;
    std::uint32_t next_handle = 1u;

    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        Check(address == actor + 892u && value == 0u,
              "clear dirty byte");
        trace.emplace_back("dirty:0");
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        Check((address == actor + 832u && value == 0u) ||
              (address == actor + 828u && value == 1u),
              "clear counter and set waiting state");
        trace.push_back(address == actor + 832u ? "counter:0" : "state:1");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == actor + 808u, "child field");
        trace.emplace_back("read-child");
        return child_value;
    }
    void InvokeActorSlot(std::uint32_t subject, std::uint32_t slot,
                         std::uint32_t argument) override {
        Check(subject == actor &&
              ((slot == 24u || slot == 572u) ? argument == context :
               slot == 396u ? argument == 1u : argument == 0u),
              "actor virtual argument order");
        trace.push_back("slot:" + std::to_string(slot));
    }
    void ResetEffect(std::uint32_t subject,
                     std::uint32_t address) override {
        Check(subject == actor &&
              (address == actor + 868u || address == actor + 876u ||
               address == actor + 884u),
              "reset old effect address");
        trace.push_back("reset:" + std::to_string(address - actor));
    }
    pal4::ActorEffectHandle LoadBottomEffect(
        std::uint32_t subject, std::uint32_t frame_address,
        std::string_view hook) override {
        Check(subject == actor && frame_address == child + 496u &&
              hook == "hook_ef_bottom", "bottom effect source and hook");
        trace.emplace_back("load-bottom");
        return {next_handle++, 0xa000u};
    }
    pal4::ActorEffectHandle LoadNamedEffect(
        std::uint32_t subject, std::string_view name,
        std::string_view hook) override {
        Check(subject == actor, "named effect actor");
        trace.push_back("load:" + std::string(name) + ":" +
                        std::string(hook));
        return {next_handle++, 0xa000u};
    }
    void StoreEffect(std::uint32_t address,
                     pal4::ActorEffectHandle handle) override {
        Check(address == actor + 868u || address == actor + 876u ||
              address == actor + 884u,
              "stored effect slot");
        Check(handle.id != 0u && handle.owner == 0xa000u,
              "stored handle pair");
        handles[address - actor] = handle;
        trace.push_back("store:" + std::to_string(address - actor));
    }
    void ResetTemporary(pal4::ActorEffectHandle handle) override {
        Check(handle.id != 0u && handle.owner == 0xa000u,
              "temporary effect pair");
        trace.emplace_back("release-temp");
    }
    std::uint32_t ModelId(std::uint32_t subject) override {
        Check(subject == actor, "model virtual receiver");
        trace.emplace_back("model");
        return model_id;
    }
};

const std::vector<std::string> preamble{
    "dirty:0", "counter:0", "state:1", "slot:24", "slot:584",
    "slot:276", "slot:572", "slot:396", "slot:548",
    "reset:868", "reset:876", "reset:884", "read-child"};

void NoChildOrUnknownModel() {
    Runtime runtime;
    runtime.child_value = 0u;
    pal4::ActorManagerEffectSetup::Initialize(actor, context, runtime);
    Check(runtime.trace == preamble && runtime.handles.empty(),
          "missing child stops after old-effect cleanup");

    runtime = {};
    runtime.model_id = 2593u;
    pal4::ActorManagerEffectSetup::Initialize(actor, context, runtime);
    auto expected = preamble;
    expected.insert(expected.end(), {"load-bottom", "store:868",
                                     "release-temp", "model"});
    Check(runtime.trace == expected && runtime.handles.size() == 1u,
          "unsupported model retains only bottom effect");
}

void PairedHands() {
    constexpr std::array models{2592u, 2603u, 2604u, 2605u, 2606u};
    constexpr std::array names{"EFF20", "EFF21", "EFF22", "EFF23",
                               "EFF24"};
    for (std::size_t i = 0; i < models.size(); ++i) {
        Runtime runtime;
        runtime.model_id = models[i];
        pal4::ActorManagerEffectSetup::Initialize(actor, context, runtime);
        auto expected = preamble;
        expected.insert(expected.end(), {
            "load-bottom", "store:868", "release-temp", "model",
            std::string("load:") + names[i] + ":hook_ef_lhand",
            "store:876", "release-temp",
            std::string("load:") + names[i] + ":hook_ef_rhand",
            "store:884", "release-temp"});
        Check(runtime.trace == expected && runtime.handles.size() == 3u &&
              runtime.handles.at(868u).id == 1u &&
              runtime.handles.at(876u).id == 2u &&
              runtime.handles.at(884u).id == 3u,
              "paired model loads bottom, left and right in order");
    }
}

void SingleHand() {
    constexpr std::array models{2598u, 2599u};
    constexpr std::array names{"H_B20", "H_B21"};
    for (std::size_t i = 0; i < models.size(); ++i) {
        Runtime runtime;
        runtime.model_id = models[i];
        pal4::ActorManagerEffectSetup::Initialize(actor, context, runtime);
        auto expected = preamble;
        expected.insert(expected.end(), {
            "load-bottom", "store:868", "release-temp", "model",
            std::string("load:") + names[i] + ":hook_ef_bottom",
            "store:876", "release-temp"});
        Check(runtime.trace == expected && runtime.handles.size() == 2u &&
              runtime.handles.at(876u).id == 2u,
              "single-hand model writes left slot with bottom hook");
    }
}

}  // namespace

int main() {
    try {
        NoChildOrUnknownModel();
        PairedHands();
        SingleHand();
        std::cout << "actor_manager_effect_setup_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
