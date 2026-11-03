#include "pal4/combat_use_property_sequence_task.h"

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
constexpr std::uint32_t kProperty = 0x5000u;

struct Runtime final : pal4::IUsePropertyActionRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 44u, kOwner}, {kTask + 48u, kTarget},
        {kTask + 52u, kProperty}, {kProperty, 7u},
        {kProperty + 4u, 9u}, {kSource + 804u, 1u},
        {kTarget + 804u, 2u}};
    std::map<std::uint32_t, bool> ready{
        {kSource, true}, {kTarget, true}};
    std::vector<std::uint32_t> primary;
    std::vector<std::uint32_t> secondary;
    std::map<std::uint32_t, std::size_t> cursor;
    std::vector<std::string> calls;
    bool exists = false;
    bool property_slot = false;
    bool null_iterator = false;
    bool throw_on_current = false;
    std::uint32_t action_property = 0u;
    std::uint32_t action_source = 0u;
    std::uint32_t action_target = 0u;

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
              "iterator owner and virtual offset");
        calls.push_back("open:" + std::to_string(offset));
        return null_iterator ? 0u : offset;
    }
    bool IteratorAtEnd(std::uint32_t iterator) override {
        return cursor[iterator] >=
               (iterator == 464u ? primary.size() : secondary.size());
    }
    std::uint32_t IteratorCurrent(std::uint32_t iterator) override {
        if (throw_on_current) throw std::runtime_error("current failed");
        return (iterator == 464u ? primary : secondary).at(cursor[iterator]);
    }
    void IteratorNext(std::uint32_t iterator) override {
        ++cursor[iterator];
    }
    void DestroyIterator(std::uint32_t iterator,
                         std::uint32_t flags) override {
        Check(flags == 1u, "iterator deleting flag");
        calls.push_back("destroy:" + std::to_string(iterator));
    }
    bool PropertyExists(std::uint32_t first,
                        std::uint32_t second) override {
        Check(first == 7u && second == 9u,
              "property database key words");
        calls.emplace_back("exists");
        return exists;
    }
    bool ActorPropertySlot8(std::uint32_t actor,
                            std::uint32_t index) override {
        Check(actor == kSource && index == 4u,
              "actor property slot four");
        calls.emplace_back("slot");
        return property_slot;
    }
    std::uint32_t UsePropertyAction(std::uint32_t property,
                                    std::uint32_t source,
                                    std::uint32_t target) override {
        calls.emplace_back("use_action");
        action_property = property;
        action_source = source;
        action_target = target;
        return 0xaaaau;
    }
    std::uint32_t NormalAction(std::uint32_t source,
                               std::uint32_t target,
                               std::uint32_t mode) override {
        Check(mode == 0u, "fallback ordinary mode zero");
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
    Runtime r;
    r.ready[kSource] = false;
    Check(pal4::UsePropertySequenceTask::CreateAction(
              kTask, kSource, r) == 0u &&
              r.calls == std::vector<std::string>{"ready:8192"},
          "unready source exits before task reads");
    r = Runtime{};
    Check(pal4::UsePropertySequenceTask::CreateAction(
              kTask, kSource, r) == 0xbbbbu &&
              r.action_target == kTarget && !Called(r, "slot"),
          "missing property falls back to normal action");
    r = Runtime{};
    r.exists = true;
    r.property_slot = true;
    Check(pal4::UsePropertySequenceTask::CreateAction(
              kTask, kSource, r) == 0xaaaau &&
              r.action_property == kProperty &&
              r.action_source == kSource &&
              r.action_target == kTarget && Called(r, "slot"),
          "inventory record and actor slot create property action");
    r.property_slot = false;
    Check(pal4::UsePropertySequenceTask::CreateAction(
              kTask, kSource, r) == 0xbbbbu,
          "failed actor property gate falls back");
}

void CandidateScans() {
    Runtime r;
    r.ready[kTarget] = false;
    r.primary = {kSource, 0x6000u, 0x7000u};
    r.ready[0x7000u] = true;
    r.words[0x7000u + 804u] = 2u;
    Check(pal4::UsePropertySequenceTask::CreateAction(
              kTask, kSource, r) == 0xbbbbu &&
              r.action_target == 0x7000u &&
              Called(r, "destroy:464"),
          "target scan skips source and unready actors");
    r = Runtime{};
    r.ready[kTarget] = false;
    Check(pal4::UsePropertySequenceTask::CreateAction(
              kTask, kSource, r) == 0u &&
              Called(r, "destroy:464") && !Called(r, "exists"),
          "empty first scan stops before inventory lookup");
    r = Runtime{};
    r.words[kTarget + 804u] = 1u;
    r.secondary = {kSource, 0x8000u};
    r.ready[0x8000u] = true;
    Check(pal4::UsePropertySequenceTask::CreateAction(
              kTask, kSource, r) == 0xbbbbu &&
              r.action_target == 0x8000u &&
              Called(r, "destroy:472"),
          "same-side target scans owner virtual +472");
    r = Runtime{};
    r.null_iterator = true;
    r.ready[kTarget] = false;
    Check(pal4::UsePropertySequenceTask::CreateAction(
              kTask, kSource, r) == 0u &&
              !Called(r, "destroy:464"),
          "null iterator exits without destructor");
    r = Runtime{};
    r.ready[kTarget] = false;
    r.primary = {0x7000u};
    r.throw_on_current = true;
    bool caught = false;
    try {
        (void)pal4::UsePropertySequenceTask::CreateAction(
            kTask, kSource, r);
    } catch (const std::runtime_error&) { caught = true; }
    Check(caught && Called(r, "destroy:464"),
          "iterator releases after candidate callback failure");
}

}  // namespace

int main() {
    try {
        GatesAndDirectAction();
        CandidateScans();
        std::cout << "Combat UseProperty action tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
