#include "pal4/combat_stunt_sequence_task.h"

#include <algorithm>
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
constexpr std::uint32_t kOwner = 0x4000u;
constexpr std::uint32_t kStunt = 0x5000u;

struct Runtime final : pal4::IStuntActionRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 44u, kOwner}, {kTask + 48u, kTarget},
        {kTask + 52u, kStunt}, {kSource + 804u, 1u},
        {kTarget + 804u, 2u}};
    std::map<std::uint32_t, bool> ready{
        {kSource, true}, {kTarget, true}};
    std::vector<std::uint32_t> primary;
    std::vector<std::uint32_t> secondary;
    std::map<std::uint32_t, std::size_t> cursor;
    std::vector<std::string> calls;
    bool can_stunt = false;
    bool property = false;
    bool throw_on_current = false;
    bool null_iterator = false;
    std::uint32_t action_source = 0u;
    std::uint32_t action_target = 0u;
    std::uint32_t action_stunt = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    bool IsActorReady(std::uint32_t actor) override {
        calls.push_back("ready:" + std::to_string(actor));
        return ready[actor];
    }
    std::uint32_t OpenIterator(std::uint32_t actor,
                               std::uint32_t offset) override {
        Check((offset == 464u && actor == kTarget) ||
                  (offset == 472u && actor == kOwner),
              "Stunt iterator owner and virtual offset");
        calls.push_back("open:" + std::to_string(offset));
        return null_iterator ? 0u : offset;
    }
    bool IteratorAtEnd(std::uint32_t iterator) override {
        calls.emplace_back("end");
        return cursor[iterator] >=
               (iterator == 464u ? primary.size() : secondary.size());
    }
    std::uint32_t IteratorCurrent(std::uint32_t iterator) override {
        calls.emplace_back("current");
        if (throw_on_current) throw std::runtime_error("current failed");
        return (iterator == 464u ? primary : secondary).at(cursor[iterator]);
    }
    void IteratorNext(std::uint32_t iterator) override {
        calls.emplace_back("next");
        ++cursor[iterator];
    }
    void DestroyIterator(std::uint32_t iterator,
                         std::uint32_t flags) override {
        Check((iterator == 464u || iterator == 472u) && flags == 1u,
              "Stunt iterator deleting destructor flag");
        calls.push_back("destroy:" + std::to_string(iterator));
    }
    bool CanUseStunt(std::uint32_t source,
                     std::uint32_t stunt) override {
        Check(source == kSource && stunt == kStunt,
              "Stunt virtual +436 arguments");
        calls.emplace_back("can_stunt");
        return can_stunt;
    }
    bool ActorPropertySlot8(std::uint32_t actor,
                            std::uint32_t index) override {
        Check(actor == kSource && index == 2u,
              "Stunt property slot 8 index two");
        calls.emplace_back("property");
        return property;
    }
    std::uint32_t StuntAction(std::uint32_t stunt,
                              std::uint32_t source,
                              std::uint32_t target) override {
        calls.emplace_back("stunt_action");
        action_stunt = stunt;
        action_source = source;
        action_target = target;
        return 0xaaaau;
    }
    std::uint32_t NormalAction(std::uint32_t source,
                               std::uint32_t target,
                               std::uint32_t mode) override {
        Check(mode == 0u, "Stunt ordinary attack mode zero");
        calls.emplace_back("normal_action");
        action_source = source;
        action_target = target;
        return 0xbbbbu;
    }
};

bool Called(const Runtime& runtime, const std::string& call) {
    return std::find(runtime.calls.begin(), runtime.calls.end(), call) !=
           runtime.calls.end();
}

void GatesAndDirectAction() {
    Runtime runtime;
    runtime.ready[kSource] = false;
    Check(pal4::StuntSequenceTask::CreateAction(
              kTask, kSource, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"ready:8192"},
          "unready Stunt source exits before task reads");
    runtime = Runtime{};
    Check(pal4::StuntSequenceTask::CreateAction(
              kTask, kSource, runtime) == 0xbbbbu &&
              runtime.action_target == kTarget &&
              !Called(runtime, "property") && !Called(runtime, "open:464"),
          "ready opposing target receives ordinary attack");
    runtime = Runtime{};
    runtime.can_stunt = true;
    runtime.property = true;
    Check(pal4::StuntSequenceTask::CreateAction(
              kTask, kSource, runtime) == 0xaaaau &&
              runtime.action_stunt == kStunt &&
              runtime.action_source == kSource &&
              runtime.action_target == kTarget &&
              Called(runtime, "property"),
          "both gates create Stunt action");
    runtime.property = false;
    Check(pal4::StuntSequenceTask::CreateAction(
              kTask, kSource, runtime) == 0xbbbbu,
          "failed property gate falls back to ordinary attack");
}

void CandidateScans() {
    Runtime runtime;
    runtime.ready[kTarget] = false;
    runtime.primary = {kSource, 0x6000u, 0x7000u};
    runtime.ready[0x7000u] = true;
    runtime.words[0x7000u + 804u] = 2u;
    Check(pal4::StuntSequenceTask::CreateAction(
              kTask, kSource, runtime) == 0xbbbbu &&
              runtime.action_target == 0x7000u &&
              runtime.cursor[464u] == 2u &&
              Called(runtime, "destroy:464"),
          "target iterator skips source and unready candidates");
    runtime = Runtime{};
    runtime.ready[kTarget] = false;
    Check(pal4::StuntSequenceTask::CreateAction(
              kTask, kSource, runtime) == 0u &&
              Called(runtime, "destroy:464") &&
              !Called(runtime, "can_stunt"),
          "empty target iterator exits before action gates");
    runtime = Runtime{};
    runtime.words[kTarget + 804u] = 1u;
    runtime.secondary = {kSource, 0x8000u, 0x9000u};
    runtime.ready[0x9000u] = true;
    Check(pal4::StuntSequenceTask::CreateAction(
              kTask, kSource, runtime) == 0xbbbbu &&
              runtime.action_target == 0x9000u &&
              runtime.cursor[472u] == 2u &&
              Called(runtime, "destroy:472"),
          "same-side target scans owner iterator");
    runtime = Runtime{};
    runtime.words[kTask + 48u] = kSource;
    Check(pal4::StuntSequenceTask::CreateAction(
              kTask, kSource, runtime) == 0u &&
              Called(runtime, "destroy:472"),
          "self target scans owner iterator and returns zero when empty");
}

void IteratorFailureCleanup() {
    Runtime runtime;
    runtime.ready[kTarget] = false;
    runtime.null_iterator = true;
    Check(pal4::StuntSequenceTask::CreateAction(
              kTask, kSource, runtime) == 0u &&
              !Called(runtime, "destroy:464"),
          "null iterator exits without destructor");
    runtime = Runtime{};
    runtime.ready[kTarget] = false;
    runtime.primary = {0x7000u};
    runtime.throw_on_current = true;
    bool caught = false;
    try {
        (void)pal4::StuntSequenceTask::CreateAction(
            kTask, kSource, runtime);
    } catch (const std::runtime_error&) { caught = true; }
    Check(caught && Called(runtime, "destroy:464"),
          "target iterator releases on exception");
    runtime = Runtime{};
    runtime.words[kTarget + 804u] = 1u;
    runtime.secondary = {0x9000u};
    runtime.throw_on_current = true;
    caught = false;
    try {
        (void)pal4::StuntSequenceTask::CreateAction(
            kTask, kSource, runtime);
    } catch (const std::runtime_error&) { caught = true; }
    Check(caught && Called(runtime, "destroy:472"),
          "owner iterator releases on exception");
}

}  // namespace

int main() {
    try {
        GatesAndDirectAction();
        CandidateScans();
        IteratorFailureCleanup();
        std::cout << "Combat Stunt action tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
