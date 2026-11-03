#include "pal4/ai_action_task.h"

#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kTask = 0x4000u;
constexpr std::uint32_t kActor = 0x5000u;
constexpr std::uint32_t kTarget = 0x6000u;

struct Runtime final : pal4::IAiActionTaskRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 4u, kActor}, {kTarget + 8u, 0u},
        {kTarget + 12u, 0u}};
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, float> floats;
    std::vector<std::string> calls;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;
    std::uint32_t selected_target = kTarget;
    std::uint32_t random = 500u;
    std::uint32_t dispatched_slot = 0xffffffffu;
    std::uint32_t dispatched_target = 0u;
    std::uint32_t posted_event = 0u;
    bool post_result = true;
    std::string logged_source;
    std::string logged_message;
    std::uint8_t logged_animation = 0u;
    int error_initializations = 0;
    int cleanup_registrations = 0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read_word");
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        writes.emplace_back(address, value);
        calls.emplace_back("write_word");
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == 0x978b10u,
              "error-string guard uses IDA global");
        return bytes[address];
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        Check(address == 0x978b10u,
              "error-string guard writes IDA global");
        bytes[address] = value;
        calls.emplace_back("write_guard");
    }
    float ReadFloat(std::uint32_t address) override {
        calls.emplace_back("weight");
        return floats[address];
    }
    void PrepareActor(std::uint32_t actor) override {
        Check(actor == kActor, "preparation uses task actor +4");
        calls.emplace_back("prepare");
    }
    std::uint32_t SelectTarget(std::uint32_t task) override {
        Check(task == kTask, "selection receives AI task");
        calls.emplace_back("select");
        return selected_target;
    }
    std::uint32_t RandomNumber() override {
        calls.emplace_back("random");
        return random;
    }
    void InitializeErrorString() override {
        ++error_initializations;
        calls.emplace_back("initialize_error");
    }
    void RegisterErrorStringCleanup() override {
        ++cleanup_registrations;
        calls.emplace_back("register_cleanup");
    }
    std::uint8_t AnimationId(std::uint32_t task) override {
        Check(task == kTask, "animation id reads AI task");
        calls.emplace_back("animation");
        return 7u;
    }
    void LogAction(std::uint32_t task,
                   std::string_view source,
                   std::string_view message,
                   std::uint8_t animation_id) override {
        Check(task == kTask, "log receives AI task");
        logged_source = source;
        logged_message = message;
        logged_animation = animation_id;
        calls.emplace_back("log");
    }
    std::uint32_t DispatchAction(std::uint32_t task,
                                 std::uint32_t virtual_slot,
                                 std::uint32_t target) override {
        Check(task == kTask, "dispatch receives AI task");
        dispatched_slot = virtual_slot;
        dispatched_target = target;
        calls.emplace_back("dispatch");
        return 0xa000u + virtual_slot;
    }
    bool PostEvent(std::uint32_t event) override {
        posted_event = event;
        calls.emplace_back("post");
        return post_result;
    }
};

void GuardsAndFacet() {
    Runtime runtime;
    Check(pal4::AiActionLocaleFacet::Construct(
              0x7000u, 9u, runtime) == 0x7000u &&
          runtime.words[0x7000u] ==
              pal4::AiActionLocaleFacet::kVtable &&
          runtime.words[0x7004u] == 9u,
          "facet constructor writes vtable and reference count");

    runtime.calls.clear();
    runtime.writes.clear();
    Check(pal4::AiActionDerivedLocaleFacet::Construct(
              0x7000u, 11u, runtime) == 0x7000u &&
          runtime.words[0x7000u] ==
              pal4::AiActionDerivedLocaleFacet::kVtable &&
          runtime.words[0x7004u] == 11u &&
          runtime.writes ==
              std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {0x7000u, pal4::AiActionLocaleFacet::kVtable},
                  {0x7004u, 11u},
                  {0x7000u, pal4::AiActionDerivedLocaleFacet::kVtable}},
          "derived facet constructs base then replaces vtable");
    runtime.calls.clear();
    runtime.writes.clear();
    pal4::AiActionDerivedLocaleFacet::Destruct(0x7000u, runtime);
    Check(runtime.words[0x7000u] ==
              pal4::AiActionLocaleFacet::kVtable &&
          runtime.words[0x7004u] == 11u &&
          runtime.writes ==
              std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {0x7000u, pal4::AiActionDerivedLocaleFacet::kVtable},
                  {0x7000u, pal4::AiActionLocaleFacet::kVtable}},
          "derived facet destructor restores base vtable");

    runtime.calls.clear();
    runtime.writes.clear();
    Check(pal4::AiActionSecondaryLocaleFacet::Construct(
              0x7000u, 13u, runtime) == 0x7000u &&
          runtime.words[0x7000u] ==
              pal4::AiActionSecondaryLocaleFacet::kVtable &&
          runtime.words[0x7004u] == 13u &&
          runtime.writes ==
              std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {0x7000u, pal4::AiActionLocaleFacet::kVtable},
                  {0x7004u, 13u},
                  {0x7000u, pal4::AiActionSecondaryLocaleFacet::kVtable}},
          "secondary facet constructs base then writes vtable 0x8458cc");
    runtime.calls.clear();
    runtime.writes.clear();
    pal4::AiActionSecondaryLocaleFacet::Destruct(0x7000u, runtime);
    Check(runtime.words[0x7000u] ==
              pal4::AiActionLocaleFacet::kVtable &&
          runtime.words[0x7004u] == 13u &&
          runtime.writes ==
              std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {0x7000u, pal4::AiActionSecondaryLocaleFacet::kVtable},
                  {0x7000u, pal4::AiActionLocaleFacet::kVtable}},
          "secondary facet destructor restores base vtable");

    runtime = Runtime{};
    runtime.words[kTask + 4u] = 0u;
    Check(!pal4::AiActionTask::Process(kTask, runtime) &&
          runtime.calls.size() == 1u &&
          runtime.dispatched_slot == 0xffffffffu,
          "missing actor exits before preparation");
    runtime = Runtime{};
    runtime.selected_target = 0u;
    Check(!pal4::AiActionTask::Process(kTask, runtime) &&
          runtime.calls.size() == 3u &&
          runtime.calls[1] == "prepare" &&
          runtime.calls[2] == "select",
          "target selection follows actor preparation");
}

void EveryWeightedBranch() {
    for (const auto& branch : pal4::AiActionTask::kBranches) {
        Runtime runtime;
        runtime.floats[kTarget + branch.weight_offset] = 1.0f;
        Check(pal4::AiActionTask::Process(kTask, runtime) &&
              runtime.dispatched_slot == branch.virtual_slot &&
              runtime.dispatched_target == kTarget &&
              runtime.posted_event ==
                  0xa000u + branch.virtual_slot &&
              runtime.logged_source == "cbActorAi::takeAction" &&
              runtime.logged_message == branch.message &&
              runtime.logged_animation == 7u &&
              runtime.calls[runtime.calls.size() - 3u] == "log" &&
              runtime.calls[runtime.calls.size() - 2u] == "dispatch" &&
              runtime.calls.back() == "post",
              "each weighted action selects its IDA virtual slot and log");
    }
    Runtime runtime;
    runtime.floats[kTarget + 24u] = 0.5f;
    Check(pal4::AiActionTask::Process(kTask, runtime) &&
          runtime.dispatched_slot == 0u,
          "first threshold includes equality at random 500");
    runtime = Runtime{};
    runtime.floats[kTarget + 24u] = 0.1f;
    Check(!pal4::AiActionTask::Process(kTask, runtime) &&
          runtime.dispatched_slot == 0xffffffffu,
          "unallocated probability leaves action unposted");
    runtime = Runtime{};
    runtime.random = 1500u;
    runtime.floats[kTarget + 24u] = 0.5f;
    Check(pal4::AiActionTask::Process(kTask, runtime) &&
          runtime.dispatched_slot == 0u,
          "random number is reduced modulo 1000");
    runtime.post_result = false;
    Check(!pal4::AiActionTask::Process(kTask, runtime),
          "event router result controls process return");
    runtime = Runtime{};
    runtime.floats[kTarget + 44u] =
        std::numeric_limits<float>::quiet_NaN();
    Check(pal4::AiActionTask::Process(kTask, runtime) &&
          runtime.dispatched_slot == 28u,
          "unordered final cumulative comparison takes cast-skill path");
}

void ErrorStringGuard() {
    Runtime runtime;
    runtime.floats[kTarget + 24u] = 1.0f;
    runtime.words[kTarget + 12u] = 4u;
    runtime.words[kTarget + 8u] = 8u;
    runtime.bytes[0x978b10u] = 0x80u;
    Check(pal4::AiActionTask::Process(kTask, runtime) &&
          runtime.bytes[0x978b10u] == 0x81u &&
          runtime.error_initializations == 1 &&
          runtime.cleanup_registrations == 1,
          "eligible state initializes guarded error string once");
    Check(pal4::AiActionTask::Process(kTask, runtime) &&
          runtime.error_initializations == 1 &&
          runtime.cleanup_registrations == 1,
          "repeated action preserves guarded initialization");
    runtime = Runtime{};
    runtime.floats[kTarget + 24u] = 1.0f;
    runtime.words[kTarget + 12u] = 3u;
    runtime.words[kTarget + 8u] = 7u;
    Check(pal4::AiActionTask::Process(kTask, runtime) &&
          runtime.error_initializations == 0,
          "subthreshold AI type and state skip error string");
}

}  // namespace

int main() {
    try {
        GuardsAndFacet();
        EveryWeightedBranch();
        ErrorStringGuard();
        std::cout << "ai_action_task_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
