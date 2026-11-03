#include "pal4/combat_dead_sequence_task.h"
#include "pal4/combat_fend_off_sequence_task.h"

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
constexpr std::uint32_t kObject = 0x2000u;
constexpr std::uint32_t kSource = 0x7777u;
constexpr std::uint32_t kTarget = 0x5555u;
constexpr std::uint32_t kChangedTarget = 0x6666u;

struct Runtime final : pal4::IFendOffDeathFactoryRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 44u, kSource}, {kTask + 48u, kTarget}};
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    bool ready = true;
    bool fail_container = false;
    bool fail_reallocate = false;
    bool fail_copy = false;
    bool change_target_on_allocate = false;
    std::uint32_t allocation = kObject;
    std::uint32_t requested_size = 0u;
    std::string copied_name;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    bool ActorCheckStateThree(std::uint32_t actor) override {
        Check(actor == kTarget, "state probe sees original target");
        calls.emplace_back("check_state");
        return ready;
    }
    bool CombatInactive() override {
        throw std::runtime_error("death callback has no combat-state query");
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        calls.emplace_back("allocate");
        requested_size = size;
        if (change_target_on_allocate) words[kTask + 48u] = kChangedTarget;
        return allocation;
    }
    void Free(std::uint32_t object) override {
        Check(object == kObject, "free allocated task");
        calls.emplace_back("free");
    }
    void InitializeContainer(std::uint32_t container) override {
        Check(container == kObject + 4u, "container address");
        calls.emplace_back("init_container");
        if (fail_container) throw std::runtime_error("container init");
    }
    void DestroyContainer(std::uint32_t container) override {
        Check(container == kObject + 4u, "destroy container address");
        calls.emplace_back("destroy_container");
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("word:" + std::to_string(address));
        words[address] = value;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.push_back("byte:" + std::to_string(address));
        bytes[address] = value;
    }
    void SetNameErrorFlag(std::uint32_t header,
                          std::uint8_t flag) override {
        Check(header == kObject + 20u && flag == 1u,
              "death task name error flag");
        calls.emplace_back("flag:1");
    }
    bool AllocateName(std::uint32_t, std::uint32_t,
                      std::uint8_t) override {
        throw std::runtime_error("death task uses reallocation");
    }
    void ReallocateName(std::uint32_t header,
                        std::uint32_t size) override {
        Check(header == kObject + 20u &&
                  (size == 4u || size == 9u),
              "death task reallocation arguments");
        calls.emplace_back("reallocate_name");
        if (fail_reallocate) throw std::runtime_error("reallocation");
    }
    void CopyName(std::uint32_t header,
                  std::string_view name) override {
        Check(header == kObject + 20u, "death task copy header");
        calls.emplace_back("copy_name");
        if (fail_copy) throw std::runtime_error("copy");
        copied_name = name;
    }
    void SetNameSize(std::uint32_t header,
                     std::uint32_t size) override {
        Check(header == kObject + 20u &&
                  (size == 4u || size == 9u),
              "death task size setter");
        calls.emplace_back("set_name_size");
        words[header + 8u] = size;
    }
    void CleanupName(std::uint32_t header) override {
        Check(header == kObject + 20u, "death task name cleanup");
        calls.emplace_back("cleanup_name");
    }
};

std::uint32_t Create(bool quick, Runtime& runtime) {
    return quick
        ? pal4::QuickFendOffSequenceTask::CreateDeathTask(kTask, runtime)
        : pal4::FendOffSequenceTask::CreateDeathTask(kTask, runtime);
}

std::size_t Index(const Runtime& runtime, const std::string& call) {
    const auto it = std::find(runtime.calls.begin(),
                              runtime.calls.end(), call);
    Check(it != runtime.calls.end(), "expected call");
    return static_cast<std::size_t>(it - runtime.calls.begin());
}

void GatesAndLayout(bool quick) {
    Runtime runtime;
    runtime.ready = false;
    Check(Create(quick, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "read:4144", "check_state"},
          "target state gate exits before allocation");

    runtime = Runtime{};
    runtime.allocation = 0u;
    Check(Create(quick, runtime) == 0u &&
              runtime.requested_size == 52u &&
              runtime.calls == std::vector<std::string>{
                  "read:4144", "check_state", "allocate"},
          "allocation failure exits before snapshot and container init");

    runtime = Runtime{};
    runtime.change_target_on_allocate = true;
    Check(Create(quick, runtime) == kObject &&
              runtime.copied_name == (quick ? "QuickDead" : "Dead") &&
              runtime.words.at(kObject) == (quick
                  ? pal4::QuickDeadSequenceTask::kVtable
                  : pal4::DeadSequenceTask::kVtable) &&
              runtime.bytes.at(kObject + 20u) == 0u &&
              runtime.words.at(kObject + 24u) == 0u &&
              runtime.words.at(kObject + 28u) ==
                  (quick ? 9u : 4u) &&
              runtime.words.at(kObject + 32u) == 0u &&
              runtime.words.at(kObject + 36u) == 0u &&
              runtime.bytes.at(kObject + 40u) == 1u &&
              runtime.words.at(kObject + 44u) == kChangedTarget &&
              runtime.words.at(kObject + 48u) == kSource,
          "death task name, vtable, deterministic byte and exchanged actors");
    Check(Index(runtime, "read:4144") <
              Index(runtime, "check_state") &&
              Index(runtime, "allocate") <
                  Index(runtime, "read:4140") &&
              Index(runtime, "read:4140") <
                  Index(runtime, "init_container") &&
              Index(runtime, "flag:1") <
                  Index(runtime, "reallocate_name") &&
              Index(runtime, "copy_name") <
                  Index(runtime, "set_name_size") &&
              Index(runtime, "word:8240") <
                  Index(runtime, "word:8192"),
          "state, snapshot, name and final vtable order");
}

void FailureCleanup(bool quick) {
    Runtime runtime;
    runtime.fail_container = true;
    bool caught = false;
    try { (void)Create(quick, runtime); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && runtime.calls.back() == "free" &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "cleanup_name") == runtime.calls.end() &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "destroy_container") == runtime.calls.end(),
          "container failure frees raw storage");

    for (bool fail_copy : {false, true}) {
        runtime = Runtime{};
        runtime.fail_reallocate = !fail_copy;
        runtime.fail_copy = fail_copy;
        caught = false;
        try { (void)Create(quick, runtime); }
        catch (const std::runtime_error&) { caught = true; }
        Check(caught && Index(runtime, "cleanup_name") <
                            Index(runtime, "destroy_container") &&
                  runtime.calls.back() == "free",
              "name failure cleans name, container and storage");
    }
}

}  // namespace

int main() {
    for (bool quick : {false, true}) {
        GatesAndLayout(quick);
        FailureCleanup(quick);
    }
    std::cout << "FendOff death factory tests passed\n";
}
