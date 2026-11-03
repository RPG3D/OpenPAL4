#include "pal4/combat_stunt_sequence_task.h"
#include "pal4/combat_dead_sequence_task.h"
#include "pal4/combat_end_action_task.h"

#include <algorithm>
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

constexpr std::uint32_t kTask = 0x1000u;
constexpr std::uint32_t kSource = 0x2000u;
constexpr std::uint32_t kTarget = 0x3000u;
constexpr std::uint32_t kStunt = 0x4000u;
constexpr std::uint32_t kFirstObject = 0x5000u;
constexpr std::uint32_t kSecondObject = 0x6000u;

struct Runtime final : pal4::IQuickStuntTransitionRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 44u, kSource}, {kTask + 48u, kTarget},
        {kTask + 52u, kStunt}};
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::string> names;
    std::vector<std::uint32_t> allocations;
    std::vector<std::string> calls;
    bool state_three = false;
    bool inactive = false;
    bool name_allocation = true;
    bool fail_container = false;
    bool fail_copy = false;
    bool mutate_task_on_allocate = false;
    bool combat_active = true;
    bool null_iterator = false;
    bool throw_on_current = false;
    std::size_t fail_allocation = 99u;
    std::vector<std::uint32_t> candidates;
    std::size_t cursor = 0u;
    std::map<std::uint32_t, bool> ready;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    bool SourceStateThree(std::uint32_t source) override {
        Check(source == kSource, "Stunt transition source state actor");
        calls.emplace_back("state_three");
        return state_three;
    }
    bool CombatActive() override {
        calls.emplace_back("combat_active");
        return combat_active;
    }
    bool IsActorReady(std::uint32_t actor) override {
        calls.push_back("ready:" + std::to_string(actor));
        return ready[actor];
    }
    std::uint32_t OpenIterator(std::uint32_t actor,
                               std::uint32_t offset) override {
        Check(actor == kTarget && offset == 464u,
              "QuickStunt target iterator virtual slot");
        calls.emplace_back("open_iterator");
        cursor = 0u;
        return null_iterator ? 0u : 0x8000u;
    }
    bool IteratorAtEnd(std::uint32_t iterator) override {
        Check(iterator == 0x8000u, "QuickStunt iterator identity");
        return cursor >= candidates.size();
    }
    std::uint32_t IteratorCurrent(std::uint32_t iterator) override {
        Check(iterator == 0x8000u, "QuickStunt iterator current identity");
        if (throw_on_current) throw std::runtime_error("current failure");
        return candidates.at(cursor);
    }
    void IteratorNext(std::uint32_t iterator) override {
        Check(iterator == 0x8000u, "QuickStunt iterator next identity");
        ++cursor;
    }
    void DestroyIterator(std::uint32_t iterator,
                         std::uint32_t flags) override {
        Check(iterator == 0x8000u && flags == 1u,
              "QuickStunt iterator deleting destructor");
        calls.emplace_back("destroy_iterator");
    }
    void NotifyUnreadyActor(std::uint32_t actor,
                            std::uint32_t source) override {
        Check(actor == 0x7100u && source == kSource,
              "QuickStunt unready actor virtual +544 arguments");
        calls.emplace_back("notify_unready");
    }
    bool CombatInactive() override {
        calls.emplace_back("combat_inactive");
        return inactive;
    }
    std::uint32_t Allocate(std::uint32_t bytes_requested) override {
        allocations.push_back(bytes_requested);
        calls.push_back("allocate:" + std::to_string(bytes_requested));
        if (allocations.size() == fail_allocation) return 0u;
        if (mutate_task_on_allocate) {
            words[kTask + 44u] = 0x2100u;
            words[kTask + 48u] = 0x3100u;
            words[kTask + 52u] = 0x4100u;
        }
        return allocations.size() == 1u ? kFirstObject : kSecondObject;
    }
    void Free(std::uint32_t object) override {
        calls.push_back("free:" + std::to_string(object));
    }
    void InitializeContainer(std::uint32_t container) override {
        calls.push_back("init_container:" + std::to_string(container));
        if (fail_container) throw std::runtime_error("container failure");
    }
    void DestroyContainer(std::uint32_t container) override {
        calls.push_back("destroy_container:" + std::to_string(container));
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.push_back("byte:" + std::to_string(address));
        bytes[address] = value;
    }
    void SetNameErrorFlag(std::uint32_t header,
                          std::uint8_t flag) override {
        calls.push_back("name_flag:" + std::to_string(flag));
        Check(header == kFirstObject + 20u ||
                  header == kSecondObject + 20u,
              "Stunt name header");
    }
    bool AllocateName(std::uint32_t header,
                      std::uint32_t length,
                      std::uint8_t flag) override {
        calls.emplace_back("allocate_name");
        Check(flag == 1u && header >= kFirstObject + 20u &&
                  (length == 8u || length == 4u || length == 9u),
              "Stunt action name allocation arguments");
        return name_allocation;
    }
    void CopyName(std::uint32_t header,
                  std::string_view value) override {
        calls.emplace_back("copy_name");
        if (fail_copy) throw std::runtime_error("copy failure");
        names[header] = std::string{value};
    }
    void CleanupName(std::uint32_t header) override {
        calls.push_back("cleanup_name:" + std::to_string(header));
    }
};

bool Called(const Runtime& runtime, const std::string& call) {
    return std::find(runtime.calls.begin(), runtime.calls.end(), call) !=
           runtime.calls.end();
}

std::size_t Index(const Runtime& runtime, const std::string& call) {
    const auto it = std::find(runtime.calls.begin(),
                              runtime.calls.end(), call);
    Check(it != runtime.calls.end(), "missing Stunt transition call");
    return static_cast<std::size_t>(it - runtime.calls.begin());
}

std::size_t LastIndex(const Runtime& runtime,
                      const std::string& call) {
    const auto it = std::find(runtime.calls.rbegin(),
                              runtime.calls.rend(), call);
    Check(it != runtime.calls.rend(), "missing final Stunt transition call");
    return runtime.calls.size() - 1u -
           static_cast<std::size_t>(it - runtime.calls.rbegin());
}

void EndStuntLayout() {
    Runtime runtime;
    Check(pal4::StuntSequenceTask::CreateNext(kTask, runtime) ==
              kFirstObject &&
              runtime.allocations == std::vector<std::uint32_t>{56u} &&
              runtime.names.at(kFirstObject + 20u) == "EndStunt" &&
              runtime.words.at(kFirstObject) ==
                  pal4::EndStuntSequenceTask::kVtable &&
              runtime.words.at(kFirstObject + 36u) == 0u &&
              runtime.words.at(kFirstObject + 44u) == kSource &&
              runtime.words.at(kFirstObject + 48u) == kTarget &&
              runtime.words.at(kFirstObject + 52u) == kStunt &&
              runtime.bytes.at(kFirstObject + 20u) == 0u &&
              runtime.bytes.at(kFirstObject + 40u) == 1u &&
              !Called(runtime, "combat_inactive"),
          "EndStunt exact size, name, vtable and fields");
    Check(Index(runtime, "state_three") <
              Index(runtime, "allocate:56") &&
              Index(runtime, "init_container:20484") <
                  Index(runtime, "name_flag:1") &&
              Index(runtime, "copy_name") <
                  Index(runtime, "write:20528"),
          "EndStunt construction order");
    runtime = Runtime{};
    runtime.mutate_task_on_allocate = true;
    Check(pal4::StuntSequenceTask::CreateNext(kTask, runtime) ==
              kFirstObject &&
              runtime.words.at(kFirstObject + 44u) == 0x2100u &&
              runtime.words.at(kFirstObject + 48u) == 0x3100u &&
              runtime.words.at(kFirstObject + 52u) == 0x4100u,
          "EndStunt snapshots actor and stunt fields after allocation");
}

void EndStuntFailureCleanup() {
    Runtime runtime;
    runtime.fail_allocation = 1u;
    Check(pal4::StuntSequenceTask::CreateNext(kTask, runtime) == 0u &&
              runtime.calls.back() == "allocate:56",
          "EndStunt null allocation exits before initialization");
    runtime = Runtime{};
    runtime.fail_container = true;
    bool caught = false;
    try { (void)pal4::StuntSequenceTask::CreateNext(kTask, runtime); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && runtime.calls.back() == "free:20480" &&
              !Called(runtime, "destroy_container:20484"),
          "container failure frees raw EndStunt storage");
    runtime = Runtime{};
    runtime.name_allocation = false;
    caught = false;
    try { (void)pal4::StuntSequenceTask::CreateNext(kTask, runtime); }
    catch (const std::bad_alloc&) { caught = true; }
    Check(caught && Index(runtime, "allocate_name") <
                        Index(runtime, "cleanup_name:20500") &&
              Index(runtime, "cleanup_name:20500") <
                  Index(runtime, "destroy_container:20484") &&
              runtime.calls.back() == "free:20480",
          "name allocation failure releases name, container and storage");
    runtime = Runtime{};
    runtime.fail_copy = true;
    caught = false;
    try { (void)pal4::StuntSequenceTask::CreateNext(kTask, runtime); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && Index(runtime, "copy_name") <
                        Index(runtime, "cleanup_name:20500") &&
              runtime.calls.back() == "free:20480",
          "name copy failure releases EndStunt resources");
}

void DeadAndEndAction(bool inactive) {
    Runtime runtime;
    runtime.state_three = true;
    runtime.inactive = inactive;
    Check(pal4::StuntSequenceTask::CreateNext(kTask, runtime) ==
              kFirstObject &&
              runtime.allocations ==
                  std::vector<std::uint32_t>{52u, 48u} &&
              runtime.words.at(kFirstObject) == (inactive
                  ? pal4::QuickDeadSequenceTask::kVtable
                  : pal4::DeadSequenceTask::kVtable) &&
              runtime.words.at(kSecondObject) ==
                  pal4::CombatEndActionTask::kVtable &&
              runtime.words.at(kFirstObject + 36u) == kSecondObject &&
              runtime.words.at(kFirstObject + 44u) == kSource &&
              runtime.words.at(kFirstObject + 48u) == kTarget &&
              runtime.words.at(kSecondObject + 44u) == kSource &&
              runtime.names.at(kFirstObject + 20u) ==
                  (inactive ? "QuickDead" : "Dead") &&
              runtime.names.at(kSecondObject + 20u) == "EndAction",
          "state three composes Dead and EndAction for both combat modes");
    Check(Index(runtime, "allocate:52") <
              Index(runtime, "allocate:48") &&
              Index(runtime, "allocate:48") <
                  LastIndex(runtime, "write:20516"),
          "Stunt death path links EndAction after both factories");
    runtime = Runtime{};
    runtime.state_three = true;
    runtime.fail_allocation = 1u;
    Check(pal4::StuntSequenceTask::CreateNext(kTask, runtime) == 0u &&
              runtime.allocations == std::vector<std::uint32_t>{52u},
          "failed Dead factory skips EndAction and null write");
    runtime = Runtime{};
    runtime.state_three = true;
    runtime.fail_allocation = 2u;
    Check(pal4::StuntSequenceTask::CreateNext(kTask, runtime) ==
              kFirstObject &&
              runtime.words.at(kFirstObject + 36u) == 0u,
          "failed EndAction leaves Dead next task null");
}

void QuickStuntFollowUp() {
    Runtime runtime;
    runtime.combat_active = false;
    Check(pal4::QuickStuntSequenceTask::CreateNext(kTask, runtime) ==
              0u &&
              runtime.calls == std::vector<std::string>{"combat_active"},
          "inactive combat exits before QuickStunt task reads");
    runtime = Runtime{};
    runtime.state_three = true;
    Check(pal4::QuickStuntSequenceTask::CreateNext(kTask, runtime) ==
              kFirstObject &&
              runtime.words.at(kFirstObject + 36u) == kSecondObject &&
              !Called(runtime, "open_iterator"),
          "dead QuickStunt source creates linked Dead and EndAction");
    runtime = Runtime{};
    runtime.candidates = {0x7000u, 0x7100u};
    runtime.ready[0x7000u] = true;
    runtime.ready[0x7100u] = false;
    Check(pal4::QuickStuntSequenceTask::CreateNext(kTask, runtime) ==
              kFirstObject &&
              runtime.allocations ==
                  std::vector<std::uint32_t>{48u} &&
              runtime.cursor == 2u &&
              Index(runtime, "ready:28672") <
                  Index(runtime, "ready:28928") &&
              Index(runtime, "notify_unready") <
                  Index(runtime, "allocate:48") &&
              Index(runtime, "allocate:48") <
                  Index(runtime, "destroy_iterator"),
          "QuickStunt notifies only unready target members then creates EndAction");
    runtime = Runtime{};
    runtime.null_iterator = true;
    Check(pal4::QuickStuntSequenceTask::CreateNext(kTask, runtime) ==
              0u && !Called(runtime, "destroy_iterator"),
          "null QuickStunt iterator exits safely");
    runtime = Runtime{};
    runtime.candidates = {0x7100u};
    runtime.throw_on_current = true;
    bool caught = false;
    try {
        (void)pal4::QuickStuntSequenceTask::CreateNext(kTask, runtime);
    } catch (const std::runtime_error&) { caught = true; }
    Check(caught && Called(runtime, "destroy_iterator"),
          "QuickStunt iterator releases after exception");
}

}  // namespace

int main() {
    try {
        EndStuntLayout();
        EndStuntFailureCleanup();
        DeadAndEndAction(false);
        DeadAndEndAction(true);
        QuickStuntFollowUp();
        std::cout << "Combat Stunt transition tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
