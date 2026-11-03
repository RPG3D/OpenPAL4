#include "pal4/quick_cast_magic_sequence_task.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

constexpr std::uint32_t kTask = 0x1000u;
constexpr std::uint32_t kSource = 0x2000u;
constexpr std::uint32_t kTarget = 0x3000u;
constexpr std::uint32_t kIterator = 0x4000u;
constexpr std::uint32_t kDead = 0x5000u;

struct Runtime final : pal4::IQuickCastMagicCompletionRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 44u, kSource}, {kTask + 48u, kTarget}};
    std::map<std::uint32_t, bool> ready;
    std::vector<std::uint32_t> members;
    std::vector<std::string> calls;
    std::size_t cursor = 0u;
    bool active = true;
    bool state_three = false;
    bool throw_on_current = false;
    std::uint32_t iterator = kIterator;
    std::uint32_t dead_result = kDead;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    bool CombatActive() override {
        calls.push_back("active");
        return active;
    }
    bool SourceStateThree(std::uint32_t source) override {
        Check(source == kSource, "source-state actor");
        calls.push_back("state_three");
        return state_three;
    }
    std::uint32_t ProcessDead(std::uint32_t source,
                              std::uint32_t target) override {
        Check(source == kSource && target == kTarget,
              "dead action actor order");
        calls.push_back("process_dead");
        return dead_result;
    }
    std::uint32_t EndAction(std::uint32_t source) override {
        Check(source == kSource, "end-action source");
        calls.push_back("end_action");
        return 0x6000u;
    }
    std::uint32_t OpenTargetIterator(std::uint32_t target) override {
        Check(target == kTarget, "iterator target virtual +464");
        calls.push_back("open");
        return iterator;
    }
    bool IteratorAtEnd(std::uint32_t value) override {
        Check(value == kIterator, "iterator end identity");
        calls.push_back("at_end");
        return cursor >= members.size();
    }
    std::uint32_t IteratorCurrent(std::uint32_t value) override {
        Check(value == kIterator, "iterator current identity");
        calls.push_back("current");
        if (throw_on_current) throw std::runtime_error("current failed");
        return members.at(cursor);
    }
    void IteratorNext(std::uint32_t value) override {
        Check(value == kIterator, "iterator next identity");
        calls.push_back("next");
        ++cursor;
    }
    void DestroyIterator(std::uint32_t value,
                         std::uint32_t flags) override {
        Check(value == kIterator && flags == 1u,
              "deleting iterator flags");
        calls.push_back("destroy");
    }
    bool IsActorReady(std::uint32_t actor) override {
        calls.push_back("ready:" + std::to_string(actor));
        return ready[actor];
    }
    void NotifyUnreadyActor(std::uint32_t actor,
                            std::uint32_t source) override {
        Check(source == kSource && !ready[actor],
              "unready actor virtual +544");
        calls.push_back("notify:" + std::to_string(actor));
    }
};

bool Called(const Runtime& runtime, const std::string& call) {
    return std::find(runtime.calls.begin(), runtime.calls.end(), call) !=
           runtime.calls.end();
}

std::size_t Index(const Runtime& runtime, const std::string& call) {
    const auto it = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(it != runtime.calls.end(), "expected call");
    return static_cast<std::size_t>(it - runtime.calls.begin());
}

void StateBranches() {
    Runtime r;
    r.active = false;
    Check(pal4::QuickCastMagicSequenceTask::Complete(kTask, r) == 0u &&
              r.calls == std::vector<std::string>{"active"},
          "inactive combat exits without reading task");

    r = Runtime{};
    r.state_three = true;
    Check(pal4::QuickCastMagicSequenceTask::Complete(kTask, r) == kDead &&
              r.words[kDead + 36u] == 0x6000u &&
              r.calls == std::vector<std::string>{
                  "active", "read:4140", "state_three", "read:4144",
                  "read:4140", "process_dead", "read:4140",
                  "end_action", "write:20516"},
          "state three chains end action after dead action");

    r = Runtime{};
    r.state_three = true;
    r.dead_result = 0u;
    Check(pal4::QuickCastMagicSequenceTask::Complete(kTask, r) == 0u &&
              Called(r, "end_action") &&
              std::none_of(r.calls.begin(), r.calls.end(),
                           [](const std::string& call) {
                               return call.starts_with("write:");
                           }),
          "null dead action returns safely after end action");
}

void TargetTraversalAndCleanup() {
    Runtime r;
    r.members = {0x7000u, 0x8000u, 0x9000u};
    r.ready[0x7000u] = true;
    r.ready[0x9000u] = true;
    Check(pal4::QuickCastMagicSequenceTask::Complete(kTask, r) == 0x6000u &&
              r.cursor == 3u && Called(r, "notify:32768") &&
              !Called(r, "notify:28672") &&
              !Called(r, "notify:36864") &&
              Index(r, "end_action") < Index(r, "destroy"),
          "only unready actors notified and iterator deleted after end action");

    r = Runtime{};
    Check(pal4::QuickCastMagicSequenceTask::Complete(kTask, r) == 0x6000u &&
              r.cursor == 0u && Called(r, "destroy"),
          "empty iterator still releases");

    r = Runtime{};
    r.iterator = 0u;
    Check(pal4::QuickCastMagicSequenceTask::Complete(kTask, r) == 0x6000u &&
              !Called(r, "at_end") && !Called(r, "destroy"),
          "missing iterator safely ends action");

    r = Runtime{};
    r.members = {0x7000u};
    r.throw_on_current = true;
    bool caught = false;
    try { (void)pal4::QuickCastMagicSequenceTask::Complete(kTask, r); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && Called(r, "destroy") && !Called(r, "end_action"),
          "iterator cleanup runs during exception");
}

}  // namespace

int main() {
    try {
        StateBranches();
        TargetTraversalAndCleanup();
        std::cout << "QuickCastMagic completion tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
