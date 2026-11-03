#include "pal4/throw_property_task.h"

#include <array>
#include <cmath>
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

constexpr std::uint32_t kTask = 0x4000u;
constexpr std::uint32_t kSource = 0x5000u;
constexpr std::uint32_t kTarget = 0x6000u;
constexpr std::uint32_t kProperty = 0x7000u;
constexpr std::uint32_t kComponent = 0x8000u;

struct Runtime final : pal4::IThrowPropertyTaskRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 60u, kSource}, {kTask + 64u, kTarget},
        {kTask + 68u, kProperty}};
    std::map<std::uint32_t, float> floats;
    std::vector<std::string> calls;
    std::vector<pal4::ThrowPropertyPosition> positions;
    pal4::ThrowPropertyPosition length_input{};
    pal4::ThrowPropertyPosition normalize_input{};
    int source_samples = 0;
    int target_samples = 0;
    int effects_reset = 0;
    int component_deletes = 0;
    bool fail_on_texture = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write:" + std::to_string(address));
    }
    float ReadFloat(std::uint32_t address) override {
        return floats[address];
    }
    void WriteFloat(std::uint32_t address,
                    float value) override {
        floats[address] = value;
        calls.emplace_back("float:" + std::to_string(address));
    }
    void ConstructBase(std::uint32_t address) override {
        Check(address == kTask, "base constructor receives task");
        calls.emplace_back("construct_base");
    }
    void DestroyBase(std::uint32_t address) override {
        Check(address == kTask, "base destructor receives task");
        calls.emplace_back("destroy_base");
    }
    void FreeTask(std::uint32_t address) override {
        Check(address == kTask, "deleting destructor frees task");
        calls.emplace_back("free_task");
    }
    pal4::PropertyTaskEffectHandle CreateActorEffect(
        std::uint32_t actor, std::string_view effect_name,
        std::uint32_t hook_address) override {
        Check(actor == kSource && effect_name == "EFF13" &&
              hook_address == pal4::ThrowPropertyTask::kEffectHook,
              "throw effect uses source, EFF13 and global hook");
        calls.emplace_back("create_effect");
        return {1u, 2u};
    }
    void ResetEffect(pal4::PropertyTaskEffectHandle& effect) override {
        Check(effect.first == 1u && effect.second == 2u,
              "temporary effect reset receives created pair");
        ++effects_reset;
        calls.emplace_back("reset_effect");
    }
    void LoadScripts() override { calls.emplace_back("load_scripts"); }
    std::uint32_t CreateComponent() override {
        calls.emplace_back("create_component");
        return kComponent;
    }
    std::string_view PropertyName(std::uint32_t property,
                                  std::uint32_t offset) override {
        Check(property == kProperty, "component names use property");
        return offset == 84u ? "model" : "texture";
    }
    void LoadComponentClump(std::uint32_t component,
                            std::string_view prefix,
                            std::string_view name) override {
        Check(component == kComponent && prefix == "palobject" &&
              name == "model", "component clump uses palobject model");
        calls.emplace_back("load_clump");
    }
    void SetComponentTexture(std::uint32_t component,
                             std::string_view name) override {
        Check(component == kComponent && name == "texture",
              "component texture uses property offset 100");
        calls.emplace_back("texture");
        if (fail_on_texture) throw std::runtime_error("texture failure");
    }
    pal4::ThrowPropertyPosition ActorPosition(
        std::uint32_t actor) override {
        if (actor == kSource) {
            ++source_samples;
            calls.emplace_back("source_position");
            return source_samples == 1 ?
                pal4::ThrowPropertyPosition{1.0f, 2.0f, 3.0f} :
                pal4::ThrowPropertyPosition{2.0f, 3.0f, 4.0f};
        }
        Check(actor == kTarget, "position samples target actor");
        ++target_samples;
        calls.emplace_back("target_position");
        return {8.0f, 6.0f, 4.0f};
    }
    float ActorBoundingHeight(std::uint32_t actor) override {
        Check(actor == kSource, "bounding height uses source actor");
        calls.emplace_back("height");
        return 10.0f;
    }
    void SetComponentPosition(
        std::uint32_t component,
        const pal4::ThrowPropertyPosition& position) override {
        Check(component == kComponent,
              "component position uses created object");
        positions.push_back(position);
        calls.emplace_back("set_position");
    }
    float VectorLength(
        const pal4::ThrowPropertyPosition& vector) override {
        length_input = vector;
        calls.emplace_back("length");
        return 10.0f;
    }
    pal4::ThrowPropertyPosition Normalize(
        const pal4::ThrowPropertyPosition& vector) override {
        normalize_input = vector;
        calls.emplace_back("normalize");
        return {0.6f, 0.8f, 0.0f};
    }
    void CleanupScripts() override {
        calls.emplace_back("cleanup_scripts");
    }
    void DeleteComponent(std::uint32_t component) override {
        Check(component == kComponent, "cleanup deletes owned component");
        ++component_deletes;
        calls.emplace_back("delete_component");
    }
};

bool Near(float actual, float expected) {
    return std::fabs(actual - expected) < 0.0001f;
}

void ConstructionAndLifetime() {
    Runtime runtime;
    Check(pal4::ThrowPropertyTask::Construct(kTask, runtime) == kTask &&
          runtime.calls.front() == "construct_base" &&
          runtime.words[kTask] == pal4::ThrowPropertyTask::kVtable &&
          runtime.words[kTask + 60u] == 0u &&
          runtime.words[kTask + 64u] == 0u &&
          runtime.words[kTask + 68u] == 0u &&
          runtime.words[kTask + 84u] == 0u,
          "constructor installs vtable and clears four fields");
    Check(pal4::ThrowPropertyTask::SetActorsAndProperty(
              kTask, kSource, kTarget, kProperty, runtime) == kProperty &&
          runtime.words[kTask + 60u] == kSource &&
          runtime.words[kTask + 64u] == kTarget &&
          runtime.words[kTask + 68u] == kProperty,
          "setter stores three arguments and returns property");
    runtime.words[kTask + 84u] = kComponent;
    Check(pal4::ThrowPropertyTask::Cleanup(kTask, runtime) == 1u &&
          runtime.component_deletes == 1 &&
          runtime.words[kTask + 84u] == 0u,
          "cleanup deletes and clears owned component");
    pal4::ThrowPropertyTask::CleanupInternal(kTask, runtime);
    Check(runtime.component_deletes == 1,
          "repeated cleanup does not delete null component");
    runtime.calls.clear();
    runtime.words[kTask + 84u] = kComponent;
    Check(pal4::ThrowPropertyTask::Delete(kTask, 0u, runtime) == kTask &&
          runtime.calls[runtime.calls.size() - 1u] == "destroy_base" &&
          runtime.component_deletes == 2,
          "nondeleting destructor runs cleanup then base");
    runtime.calls.clear();
    Check(pal4::ThrowPropertyTask::Delete(kTask, 3u, runtime) == kTask &&
          runtime.calls.back() == "free_task",
          "deleting destructor frees when low flag bit is set");
}

void ProcessAndMovement() {
    for (const auto missing : {60u, 64u, 68u}) {
        Runtime runtime;
        runtime.words[kTask + missing] = 0u;
        Check(pal4::ThrowPropertyTask::Process(kTask, runtime) == 1u &&
              runtime.calls.empty(),
              "missing actor or property exits before effect creation");
    }
    Runtime runtime;
    Check(pal4::ThrowPropertyTask::Process(kTask, runtime) == 1u &&
          runtime.effects_reset == 1 &&
          runtime.words[kTask + 84u] == kComponent &&
          runtime.source_samples == 2 && runtime.target_samples == 1 &&
          runtime.positions.size() == 1u &&
          runtime.positions[0] ==
              pal4::ThrowPropertyPosition{1.0f, 8.0f, 3.0f} &&
          runtime.length_input ==
              pal4::ThrowPropertyPosition{6.0f, 3.0f, 0.0f} &&
          runtime.normalize_input == runtime.length_input &&
          Near(runtime.floats[kTask + 72u], 360.0f) &&
          Near(runtime.floats[kTask + 76u], 480.0f) &&
          Near(runtime.floats[kTask + 80u], 0.0f) &&
          Near(runtime.floats[kTask + 20u], 0.016666667f) &&
          runtime.calls.back() == "cleanup_scripts",
          "process creates throw component and converts direction to velocity");
    runtime.floats[kComponent + 172u] = 10.0f;
    runtime.floats[kComponent + 176u] = 20.0f;
    runtime.floats[kComponent + 180u] = 30.0f;
    Check(pal4::ThrowPropertyTask::UpdatePosition(
              kTask, 0.5f, runtime) == 1u &&
          runtime.positions.back() ==
              pal4::ThrowPropertyPosition{190.0f, 260.0f, 30.0f},
          "update adds velocity times delta to component position");
    runtime.words[kTask + 84u] = 0u;
    const auto before = runtime.positions.size();
    Check(pal4::ThrowPropertyTask::UpdatePosition(
              kTask, 1.0f, runtime) == 1u &&
          runtime.positions.size() == before,
          "update skips missing component");
    Runtime failing;
    failing.fail_on_texture = true;
    bool threw = false;
    try {
        (void)pal4::ThrowPropertyTask::Process(kTask, failing);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && failing.effects_reset == 1 &&
          failing.words[kTask + 84u] == kComponent &&
          failing.calls.back() == "texture",
          "texture failure retains component for task cleanup");
}

}  // namespace

int main() {
    try {
        ConstructionAndLifetime();
        ProcessAndMovement();
        std::cout << "throw_property_task_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
