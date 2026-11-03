#include "pal4/quick_attack_task.h"

#include <algorithm>
#include <array>
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
constexpr std::uint32_t kCandidate = 0x4000u;
constexpr std::uint32_t kChild = 0x5000u;

struct Runtime final : pal4::IQuickAttackStartRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 44u, kSource}, {kTask + 48u, kTarget},
        {0x8f9694u, 0x7777u}};
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    bool has_candidate = false;
    bool candidate_eligible = true;
    bool source_blocks = false;
    std::uint32_t child = kChild;

    std::int32_t ActorState(std::uint32_t actor) override {
        calls.push_back("state:" + std::to_string(actor));
        return actor == kTarget ? 1 : 0;
    }
    bool SourceBlocksEvent(std::uint32_t actor) override {
        Check(actor == kSource, "selection checks original source");
        calls.push_back("source_blocks");
        return source_blocks;
    }
    std::uint32_t OpenTargetIterator(std::uint32_t target) override {
        Check(target == kTarget, "iterator belongs to target");
        calls.push_back("open_iterator");
        return 0x6000u;
    }
    bool IteratorAtEnd(std::uint32_t iterator) override {
        Check(iterator == 0x6000u, "iterator identity");
        calls.push_back("at_end");
        return !has_candidate;
    }
    std::uint32_t IteratorCurrent(std::uint32_t iterator) override {
        Check(iterator == 0x6000u, "current uses iterator");
        calls.push_back("current");
        return kCandidate;
    }
    void IteratorNext(std::uint32_t iterator) override {
        Check(iterator == 0x6000u, "next uses iterator");
        calls.push_back("next");
        has_candidate = false;
    }
    bool CandidateStateNotThree(std::uint32_t actor) override {
        Check(actor == kCandidate, "candidate state checks candidate");
        calls.push_back("candidate_state");
        return candidate_eligible;
    }
    bool CandidateCanAffect(std::uint32_t actor,
                            std::uint32_t target) override {
        Check(actor == kCandidate && target == kTarget,
              "candidate eligibility uses actor pair");
        calls.push_back("candidate_affects");
        return true;
    }
    std::uint32_t GenerateRandomNumber() override {
        calls.push_back("random");
        return 17u;
    }
    void DeleteIterator(std::uint32_t iterator,
                        std::uint32_t flags) override {
        Check(iterator == 0x6000u && flags == 1u,
              "iterator cleanup uses deleting flag");
        calls.push_back("delete_iterator");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.push_back("write_byte:" + std::to_string(address));
        bytes[address] = value;
    }
    void SetSelectedTarget(std::uint32_t selected,
                           std::uint32_t source,
                           std::uint32_t target) override {
        Check(selected == kCandidate && source == kCandidate &&
                  target == kTarget,
              "selected actor virtual +552 gets selected twice and target");
        calls.push_back("set_target");
    }
    void NotifySource(std::uint32_t actor) override {
        Check(actor == words.at(kTask + 44u),
              "source virtual +476 receives current source");
        calls.push_back("notify");
    }
    std::uint32_t CreateChildTask(
        const pal4::QuickAttackChildFactorySlot& factory) override {
        Check(factory.vtable == pal4::QuickAttackChildFactory::kVtable,
              "child factory vtable");
        calls.push_back("create_child");
        return child;
    }
    void AddTaskAndInitialize(
        std::uint32_t container, std::uint32_t added,
        std::uint32_t source_text,
        const std::array<std::uint32_t, 3>& timing) override {
        Check(container == kTask + 4u && added == kChild &&
                  source_text == (words.at(0x8f9694u) == 0u
                                      ? 0x841964u : 0x7777u) &&
                  timing == std::array<std::uint32_t, 3>{1u, 0u, 0u},
              "GOM add receives container, text and timing");
        calls.push_back("add_child");
    }
};

std::size_t Index(const Runtime& runtime, const std::string& call) {
    const auto it = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(it != runtime.calls.end(), "expected call");
    return static_cast<std::size_t>(it - runtime.calls.begin());
}

void NoCandidate() {
    Runtime runtime;
    Check(!pal4::QuickAttackTask::Start(kTask, runtime),
          "no candidate returns false");
    Check(runtime.words.at(kTask + 44u) == kSource &&
              runtime.words.at(kChild + 60u) == kSource &&
              runtime.words.at(kChild + 64u) == kTarget &&
              runtime.bytes.at(kChild + 72u) == 0u &&
              runtime.bytes.at(kChild + 73u) == 0u,
          "original actors and zero flags are forwarded");
    Check(Index(runtime, "delete_iterator") < Index(runtime, "notify") &&
              Index(runtime, "notify") < Index(runtime, "create_child") &&
              Index(runtime, "add_child") <
                  Index(runtime, "write:" + std::to_string(kChild + 60u)),
          "selection, notification, creation and child fields follow IDA");
}

void SelectedCandidate() {
    Runtime runtime;
    runtime.has_candidate = true;
    Check(pal4::QuickAttackTask::Start(kTask, runtime),
          "eligible candidate returns true");
    Check(runtime.words.at(kTask + 44u) == kCandidate &&
              runtime.words.at(kChild + 60u) == kCandidate &&
              runtime.words.at(kChild + 64u) == kTarget &&
              runtime.bytes.at(kChild + 72u) == 1u &&
              runtime.bytes.at(kChild + 73u) == 0u,
          "selection rewrites parent source and child fields");
    Check(Index(runtime, "random") < Index(runtime, "set_target") &&
              Index(runtime, "set_target") < Index(runtime, "notify"),
          "selected virtual call precedes source notification");
}

void RejectedAndBlockedCandidate() {
    Runtime runtime;
    runtime.has_candidate = true;
    runtime.candidate_eligible = false;
    Check(!pal4::QuickAttackTask::Start(kTask, runtime) &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "random") == runtime.calls.end(),
          "ineligible candidate is not selected");
    runtime = Runtime{};
    runtime.source_blocks = true;
    Check(!pal4::QuickAttackTask::Start(kTask, runtime) &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "open_iterator") == runtime.calls.end(),
          "source gate skips candidate iterator");
}

void TextFallbackAndFailedChild() {
    Runtime runtime;
    runtime.words[0x8f9694u] = 0u;
    Check(!pal4::QuickAttackTask::Start(kTask, runtime),
          "empty text fallback preserves normal no-candidate result");
    runtime = Runtime{};
    runtime.has_candidate = true;
    runtime.child = 0u;
    Check(!pal4::QuickAttackTask::Start(kTask, runtime) &&
              runtime.words.at(kTask + 44u) == kCandidate &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "add_child") == runtime.calls.end(),
          "failed child creation keeps prior actor side effect and stops");
    pal4::QuickAttackChildFactorySlot slot;
    Check(pal4::QuickAttackChildFactory::Initialize(&slot) == &slot &&
              slot.vtable == pal4::QuickAttackChildFactory::kVtable,
          "factory initializer preserves slot identity");
}

}  // namespace

int main() {
    NoCandidate();
    SelectedCandidate();
    RejectedAndBlockedCandidate();
    TextFallbackAndFailedChild();
    std::cout << "QuickAttack start tests passed\n";
}
