#include "pal4/combat_stunt_sequence_task.h"
#include "pal4/check_protect_task.h"
#include "pal4/start_attack_task.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kTask = 0x1000u;
constexpr std::uint32_t kSource = 0x2000u;
constexpr std::uint32_t kTarget = 0x3000u;
constexpr std::uint32_t kStunt = 0x4000u;
constexpr std::uint32_t kHeader = 0x7000u;
constexpr std::uint32_t kText = 0x8088u;

struct AddedTask {
    std::uint32_t child;
    std::uint32_t text;
    std::array<std::uint32_t, 3> timing;
};

struct RunTarget {
    std::uint32_t child;
    std::uint32_t source;
    std::array<float, 3> position;
};

struct Runtime final : pal4::IStuntSequenceRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 44u, kSource}, {kTask + 48u, kTarget},
        {kTask + 52u, kStunt}, {kSource + 808u, 1u},
        {kSource + 804u, 1u}, {kTarget + 804u, 2u},
        {kStunt + 132u, 0u}, {0x8f9784u, 0xa000u},
        {0x8f97acu, 0xa100u}, {0x8f9798u, 0xa200u},
        {0x8f97c0u, 0xa300u}, {kHeader + 4u, kText},
        {kHeader + 8u, 5u}, {kHeader + 12u, 8u}};
    std::map<std::uint32_t, std::uint8_t> bytes{{kText - 1u, 1u}};
    std::vector<std::uint32_t> factories;
    std::vector<AddedTask> added;
    std::vector<RunTarget> runs;
    std::vector<std::string> calls;
    std::uint32_t movement_mode = 1u;
    std::uint32_t fail_factory = 0u;
    bool allocate_text = true;
    bool mutate_target_on_first_add = false;
    std::uint32_t target_position_actor = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.emplace_back("read_byte");
        return bytes.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        calls.emplace_back("write_byte");
        bytes[address] = value;
    }
    std::array<float, 3> ActorChildPosition(
        std::uint32_t actor) override {
        calls.emplace_back("child_position");
        if (actor == kSource) return {5.0f, 6.0f, 7.0f};
        Check(actor == kTarget, "Stunt position actor");
        return {1.0f, 2.0f, 3.0f};
    }
    void NormalizeVector(std::array<float, 3>& vector) override {
        Check(vector == std::array<float, 3>{4.0f, 4.0f, 4.0f} ||
                  vector == std::array<float, 3>{0.0f, 0.0f, 0.0f},
              "Stunt normalizes source minus target");
        vector = vector[0] == 0.0f
            ? std::array<float, 3>{0.0f, 0.0f, 0.0f}
            : std::array<float, 3>{0.5f, 0.5f, 0.5f};
        calls.emplace_back("normalize");
    }
    float ActorClampedChildSize(std::uint32_t actor) override {
        Check(actor == kTarget || actor == kSource,
              "Stunt clamped size actor");
        calls.emplace_back("clamp");
        return 2.0f;
    }
    void NotifySource(std::uint32_t actor) override {
        Check(actor == kSource, "Stunt notify source actor");
        calls.emplace_back("notify");
    }
    std::uint32_t StuntMovementMode(std::uint32_t stunt) override {
        Check(stunt == kStunt, "Stunt movement mode object");
        calls.emplace_back("movement_mode");
        return movement_mode;
    }
    std::array<float, 3> ActorTargetPosition(
        std::uint32_t actor) override {
        Check(actor == kTarget || actor == 0x3100u,
              "Stunt target position actor");
        target_position_actor = actor;
        calls.emplace_back("target_position");
        return {9.0f, 8.0f, 7.0f};
    }
    std::uint32_t CreateChildTask(std::uint32_t factory) override {
        factories.push_back(factory);
        calls.emplace_back("create");
        return factory == fail_factory ? 0u :
               0x9000u + static_cast<std::uint32_t>(factories.size()) * 0x100u;
    }
    void AddTaskAndInitialize(
        std::uint32_t container, std::uint32_t child,
        std::uint32_t text,
        const std::array<std::uint32_t, 3>& timing) override {
        Check(container == kTask + 4u &&
                  timing == std::array<std::uint32_t, 3>{1u, 0u, 0u},
              "Stunt GOM arguments");
        added.push_back({child, text, timing});
        calls.emplace_back("add");
        if (mutate_target_on_first_add && added.size() == 1u)
            words[kTask + 48u] = 0x3100u;
    }
    void SetApproachTaskTarget(
        std::uint32_t child, std::uint32_t source,
        const std::array<float, 3>& position) override {
        Check(source == kSource, "Stunt run source");
        runs.push_back({child, source, position});
        calls.emplace_back("set_run_target");
    }
    std::uint32_t ActorStuntText(std::uint32_t source,
                                 std::uint32_t stunt) override {
        Check(source == kSource && stunt == kStunt,
              "Stunt text virtual arguments");
        calls.emplace_back("stunt_text");
        return kHeader;
    }
    void SetTextErrorFlag(std::uint32_t header,
                          std::uint8_t flag) override {
        Check(header == added.back().child + 64u && flag == 1u,
              "Stunt shared text destination");
        calls.emplace_back("text_flag");
    }
    bool AllocateText(std::uint32_t header, std::uint32_t,
                      std::uint8_t flag) override {
        Check(header == added.back().child + 64u && flag == 1u,
              "Stunt text allocation arguments");
        calls.emplace_back("allocate_text");
        if (allocate_text) words[header + 4u] = 0xb000u;
        return allocate_text;
    }
    void CopyText(std::uint32_t header, std::uint32_t source,
                  std::uint32_t length) override {
        Check(header == added.back().child + 64u &&
                  source == kText && length == 5u,
              "Stunt text copy arguments");
        calls.emplace_back("copy_text");
    }
    void DetachText(std::uint32_t) override {
        throw std::runtime_error("unexpected Stunt text alias");
    }
    void MoveTextBytes(std::uint32_t, std::uint32_t,
                       std::uint32_t) override {
        throw std::runtime_error("unexpected Stunt text move");
    }
    void SetTextSize(std::uint32_t, std::uint32_t) override {
        throw std::runtime_error("unexpected Stunt text resize");
    }
    std::uint8_t EraseText(std::uint32_t, std::uint32_t,
                           std::uint32_t) override {
        throw std::runtime_error("unexpected Stunt text erase");
    }
};

void FactoryAndInactive() {
    pal4::StuntChildFactorySlot slot;
    Check(pal4::StuntChildFactory::Initialize(&slot) == &slot &&
              slot.vtable == 0x845d74u,
          "Stunt child factory slot identity and vtable");
    Runtime runtime;
    runtime.words[kSource + 808u] = 0u;
    Check(pal4::StuntSequenceTask::Start(kTask, runtime) ==
              static_cast<std::uint8_t>(kSource) &&
              runtime.factories.empty() && runtime.added.empty() &&
              runtime.calls == std::vector<std::string>{
                  "read:4140", "read:9000"},
          "inactive source returns its low byte without scheduling");
}

void MainFields(const Runtime& runtime) {
    const auto child = runtime.added.back().child;
    Check(runtime.words.at(child + 320u) == kTarget &&
              runtime.words.at(child + 316u) == kSource &&
              runtime.words.at(child + 324u) == kStunt &&
              runtime.words.at(child + 60u) == kSource &&
              runtime.words.at(child + 68u) == kText &&
              runtime.words.at(child + 72u) == 5u &&
              runtime.words.at(child + 76u) == 8u &&
              runtime.bytes.at(child + 332u) == 1u,
          "Stunt main child fields and shared text");
}

void BasicModes() {
    for (const auto mode : {1u, 3u, 4u}) {
        Runtime runtime;
        runtime.movement_mode = mode;
        Check(pal4::StuntSequenceTask::Start(kTask, runtime) == 0x88u &&
                  runtime.factories == std::vector<std::uint32_t>{
                      pal4::StartAttackApproachFactory::kPrimaryVtable,
                      pal4::StuntChildFactory::kVtable} &&
                  runtime.added.at(0).text == 0xa000u &&
                  runtime.added.at(1).text == 0xa300u &&
                  runtime.runs.at(0).position == (mode == 1u
                      ? std::array<float, 3>{2.0f, 3.0f, 4.0f}
                      : std::array<float, 3>{9.0f, 8.0f, 7.0f}),
              "Stunt movement mode selects adjusted or actor target position");
        MainFields(runtime);
    }
}

void EffectModes() {
    Runtime runtime;
    runtime.words[kStunt + 132u] = 1u;
    Check(pal4::StuntSequenceTask::Start(kTask, runtime) == 0x88u &&
              runtime.factories == std::vector<std::uint32_t>{
                  pal4::StartAttackApproachFactory::kPrimaryVtable,
                  pal4::StartAttackApproachFactory::kSecondaryVtable,
                  pal4::StuntChildFactory::kVtable} &&
              runtime.runs.at(1).position ==
                  std::array<float, 3>{2.0f, 3.0f, 4.0f} &&
              runtime.added.at(1).text == 0xa100u,
          "mode one adds secondary run for opposing sides");
    MainFields(runtime);

    runtime = Runtime{};
    runtime.words[kStunt + 132u] = 2u;
    Check(pal4::StuntSequenceTask::Start(kTask, runtime) == 0x88u &&
              runtime.factories == std::vector<std::uint32_t>{
                  pal4::StartAttackApproachFactory::kPrimaryVtable,
                  pal4::StartAttackApproachFactory::kPrimaryVtable,
                  pal4::CheckProtectSubtaskFactory::kPositionVtable,
                  pal4::StuntChildFactory::kVtable} &&
              runtime.runs.size() == 2u &&
              runtime.runs.at(1).position ==
                  std::array<float, 3>{2.0f, 3.0f, 4.0f} &&
              runtime.added.at(2).text == 0xa200u,
          "mode two adds primary run and position child");
    const auto position_child = runtime.added.at(2).child;
    Check(runtime.words.at(position_child + 72u) == kSource &&
              runtime.words.at(position_child + 60u) ==
                  std::bit_cast<std::uint32_t>(2.0f) &&
              runtime.words.at(position_child + 64u) ==
                  std::bit_cast<std::uint32_t>(3.0f) &&
              runtime.words.at(position_child + 68u) ==
                  std::bit_cast<std::uint32_t>(4.0f),
          "position child receives source and adjusted vector");
    MainFields(runtime);
}

void SameSideAndFallbacks() {
    Runtime runtime;
    runtime.words[kTarget + 804u] = 1u;
    runtime.words[kStunt + 132u] = 2u;
    runtime.words[0x8f9784u] = 0u;
    runtime.words[0x8f97c0u] = 0u;
    Check(pal4::StuntSequenceTask::Start(kTask, runtime) == 0x88u &&
              runtime.factories.size() == 2u &&
              runtime.runs.at(0).position ==
                  std::array<float, 3>{1.0f, 2.0f, 3.0f} &&
              runtime.added.at(0).text == 0x841964u &&
              runtime.added.at(1).text == 0x841964u,
          "same-side Stunt skips extra mode child and uses fallback text");
    runtime = Runtime{};
    runtime.words[kTask + 48u] = kSource;
    runtime.words[kSource + 804u] = 1u;
    Check(pal4::StuntSequenceTask::Start(kTask, runtime) == 0x88u &&
              runtime.factories == std::vector<std::uint32_t>{
                  pal4::StuntChildFactory::kVtable},
          "same actor skips movement child");
}

void CallbackSensitiveTargetRead() {
    Runtime runtime;
    runtime.movement_mode = 3u;
    runtime.mutate_target_on_first_add = true;
    Check(pal4::StuntSequenceTask::Start(kTask, runtime) == 0x88u &&
              runtime.target_position_actor == 0x3100u &&
              runtime.words.at(runtime.added.back().child + 320u) == 0x3100u,
          "Stunt rereads target after first GOM add callback");
    const auto added = std::find(runtime.calls.begin(),
                                 runtime.calls.end(), "add");
    const auto positioned = std::find(runtime.calls.begin(),
                                      runtime.calls.end(), "target_position");
    Check(added < positioned &&
              std::count(runtime.calls.begin(), runtime.calls.end(),
                         "read:13092") == 1,
          "mode three obtains position after add and effect mode zero skips second side check");
}

void TextAndNullChildren() {
    Runtime runtime;
    runtime.bytes[kText - 1u] = 0xfeu;
    Check(pal4::StuntSequenceTask::Start(kTask, runtime) == 5u &&
              runtime.bytes.at(0xb005u) == 0u &&
              runtime.bytes.at(runtime.added.back().child + 332u) == 1u,
          "saturated text is copied and null terminated");
    runtime = Runtime{};
    runtime.bytes[kText - 1u] = 0xfeu;
    runtime.allocate_text = false;
    Check(pal4::StuntSequenceTask::Start(kTask, runtime) == 0u &&
              runtime.bytes.at(runtime.added.back().child + 332u) == 1u,
          "text allocation failure still marks child complete");
    for (const auto factory : {
             pal4::StartAttackApproachFactory::kPrimaryVtable,
             pal4::StuntChildFactory::kVtable,
             pal4::StartAttackApproachFactory::kSecondaryVtable,
             pal4::CheckProtectSubtaskFactory::kPositionVtable}) {
        runtime = Runtime{};
        runtime.fail_factory = factory;
        if (factory == pal4::StartAttackApproachFactory::kSecondaryVtable)
            runtime.words[kStunt + 132u] = 1u;
        if (factory == pal4::CheckProtectSubtaskFactory::kPositionVtable)
            runtime.words[kStunt + 132u] = 2u;
        Check(pal4::StuntSequenceTask::Start(kTask, runtime) == 0u &&
                  runtime.factories.back() == factory,
              "null Stunt child exits before native invalid write");
    }
}

}  // namespace

int main() {
    try {
        FactoryAndInactive();
        BasicModes();
        EffectModes();
        SameSideAndFallbacks();
        CallbackSensitiveTargetRead();
        TextAndNullChildren();
        std::cout << "Combat Stunt start tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
