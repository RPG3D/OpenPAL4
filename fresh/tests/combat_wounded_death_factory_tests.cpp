#include "pal4/combat_wounded_death_factory.h"
#include "pal4/combat_wounded_sequence_task.h"
#include "pal4/combat_dead_sequence_task.h"

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
constexpr std::uint32_t kSource = 0x3000u;
constexpr std::uint32_t kTarget = 0x4000u;

struct Runtime final : pal4::IWoundedDeathFactoryRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 44u, kSource}, {kTask + 48u, kTarget}};
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t allocation = kObject;
    std::uint32_t gated_actor = 0u;
    std::string copied_name;
    bool ready = true;
    bool change_on_allocate = false;
    bool fail_container = false;
    bool fail_name = false;
    bool fail_vtable = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    bool ActorCheckStateThree(std::uint32_t actor) override {
        gated_actor = actor;
        calls.emplace_back("gate");
        return ready;
    }
    bool CombatInactive() override {
        throw std::runtime_error("unexpected combat-state query");
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 52u, "death object size");
        calls.emplace_back("allocate");
        if (change_on_allocate) {
            words[kTask + 44u] = 0x3100u;
            words[kTask + 48u] = 0x4100u;
        }
        return allocation;
    }
    void Free(std::uint32_t object) override {
        Check(object == kObject, "free death object");
        calls.emplace_back("free");
    }
    void InitializeContainer(std::uint32_t address) override {
        Check(address == kObject + 4u, "container address");
        calls.emplace_back("init_container");
        if (fail_container) throw std::runtime_error("container failure");
    }
    void DestroyContainer(std::uint32_t address) override {
        Check(address == kObject + 4u, "destroy container address");
        calls.emplace_back("destroy_container");
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("word:" + std::to_string(address));
        if (address == kObject && fail_vtable)
            throw std::runtime_error("vtable failure");
        words[address] = value;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.push_back("byte:" + std::to_string(address));
        bytes[address] = value;
    }
    void SetNameErrorFlag(std::uint32_t address,
                          std::uint8_t flag) override {
        Check(address == kObject + 20u && flag == 1u,
              "name flag arguments");
        calls.emplace_back("name_flag");
    }
    bool AllocateName(std::uint32_t, std::uint32_t,
                      std::uint8_t) override {
        throw std::runtime_error("unexpected name allocation");
    }
    void ReallocateName(std::uint32_t address,
                        std::uint32_t size) override {
        Check(address == kObject + 20u &&
                  (size == 4u || size == 9u),
              "name reallocation arguments");
        calls.emplace_back("reallocate_name");
        if (fail_name) throw std::runtime_error("name failure");
    }
    void CopyName(std::uint32_t address,
                  std::string_view name) override {
        Check(address == kObject + 20u, "name copy address");
        copied_name = name;
        calls.emplace_back("copy_name");
    }
    void SetNameSize(std::uint32_t address,
                     std::uint32_t size) override {
        Check(address == kObject + 20u, "name size address");
        words[address + 8u] = size;
        calls.emplace_back("set_name_size");
    }
    void CleanupName(std::uint32_t address) override {
        Check(address == kObject + 20u, "name cleanup address");
        calls.emplace_back("cleanup_name");
    }
};

std::uint32_t Create(bool quick, Runtime& runtime) {
    return quick
        ? pal4::QuickWoundedSequenceTask::CreateDeathTask(kTask, runtime)
        : pal4::WoundedSequenceTask::CreateDeathTask(kTask, runtime);
}

bool Called(const Runtime& runtime, const std::string& call) {
    return std::find(runtime.calls.begin(), runtime.calls.end(), call) !=
           runtime.calls.end();
}

void GatesAndLayout(bool quick) {
    Runtime r;
    r.ready = false;
    Check(Create(quick, r) == 0u &&
              r.gated_actor == (quick ? kTarget : kSource) &&
              !Called(r, "allocate"), "state gate");

    r = Runtime{};
    r.allocation = 0u;
    Check(Create(quick, r) == 0u &&
              !Called(r, "init_container"), "allocation failure");

    r = Runtime{};
    r.change_on_allocate = true;
    Check(Create(quick, r) == kObject &&
              r.gated_actor == (quick ? kTarget : kSource) &&
              r.words.at(kObject) == (quick
                  ? pal4::QuickDeadSequenceTask::kVtable
                  : pal4::DeadSequenceTask::kVtable) &&
              r.copied_name == (quick ? "QuickDead" : "Dead"),
          "name and vtable");
    Check(r.bytes.at(kObject + 20u) == 0u &&
              r.words.at(kObject + 24u) == 0u &&
              r.words.at(kObject + 28u) == (quick ? 9u : 4u) &&
              r.words.at(kObject + 32u) == 0u &&
              r.words.at(kObject + 36u) == 0u &&
              r.bytes.at(kObject + 40u) == 1u &&
              r.words.at(kObject + 44u) ==
                  (quick ? 0x4100u : 0x3100u) &&
              r.words.at(kObject + 48u) ==
                  (quick ? 0x3100u : 0x4100u),
          "name header and actor layout");
    Check(r.calls[0] == (quick ? "read:4144" : "read:4140") &&
              r.calls[1] == "gate" &&
              r.calls[2] == "allocate" &&
              r.calls[3] == (quick ? "read:4140" : "read:4144") &&
              r.calls[4] == (quick ? "read:4144" : "read:4140") &&
              r.calls[5] == "init_container" &&
              r.calls.back() == "word:8192",
          "snapshot and construction order");
}

void FailureCleanup(bool quick) {
    for (int stage = 0; stage < 3; ++stage) {
        Runtime r;
        r.fail_container = stage == 0;
        r.fail_name = stage == 1;
        r.fail_vtable = stage == 2;
        bool caught = false;
        try { (void)Create(quick, r); }
        catch (const std::runtime_error&) { caught = true; }
        Check(caught && r.calls.back() == "free",
              "failed construction frees storage");
        Check(Called(r, "destroy_container") == (stage != 0) &&
                  Called(r, "cleanup_name") == (stage != 0),
              "staged cleanup");
        if (stage != 0) {
            const auto name = std::find(r.calls.begin(), r.calls.end(),
                                        "cleanup_name");
            const auto container = std::find(r.calls.begin(), r.calls.end(),
                                             "destroy_container");
            Check(name < container, "reverse cleanup order");
        }
    }
}

}  // namespace

int main() {
    try {
        for (bool quick : {false, true}) {
            GatesAndLayout(quick);
            FailureCleanup(quick);
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
