#include "pal4/check_protect_task.h"

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

constexpr std::uint32_t kOwner = 0x1000u;
constexpr std::uint32_t kActor = 0x2000u;
constexpr std::uint32_t kPrimary = 0x3000u;
constexpr std::uint32_t kSecondary = 0x4000u;
constexpr std::uint32_t kTask = 0x6000u;

struct Runtime final : pal4::ICheckProtectTaskRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kOwner + 44u, kSecondary}, {kOwner + 48u, kPrimary},
        {kActor + 804u, 1u}, {kPrimary + 804u, 2u}};
    std::map<std::uint32_t, std::uint8_t> bytes{{kOwner + 56u, 9u}};
    std::map<std::uint32_t, bool> ready{
        {kActor, true}, {kPrimary, true}};
    std::map<std::uint32_t, std::vector<std::uint32_t>> candidates;
    std::map<std::uint32_t, std::size_t> position;
    std::vector<std::string> calls;
    std::uint32_t allocation = kTask;
    bool throw_on_assign = false;
    bool throw_on_container_init = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.push_back("read_byte:" + std::to_string(address));
        return bytes.at(address);
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
    bool IsActorReady(std::uint32_t actor) override {
        calls.push_back("ready:" + std::to_string(actor));
        return ready[actor];
    }
    std::uint32_t OpenIterator(std::uint32_t collection,
                               std::uint32_t virtual_offset) override {
        Check((collection == kPrimary && virtual_offset == 464u) ||
                  (collection == kSecondary && virtual_offset == 472u),
              "iterator uses expected collection virtual slot");
        calls.push_back("open:" + std::to_string(collection));
        return collection + 0x100u;
    }
    bool IteratorAtEnd(std::uint32_t iterator) override {
        calls.push_back("end:" + std::to_string(iterator));
        return position[iterator] == candidates[iterator].size();
    }
    std::uint32_t IteratorCurrent(std::uint32_t iterator) override {
        calls.push_back("current:" + std::to_string(iterator));
        return candidates[iterator].at(position[iterator]);
    }
    void IteratorNext(std::uint32_t iterator) override {
        calls.push_back("next:" + std::to_string(iterator));
        ++position[iterator];
    }
    void DeleteIterator(std::uint32_t iterator,
                        std::uint32_t flags) override {
        Check(flags == 1u, "iterator uses deleting destructor flag");
        calls.push_back("delete:" + std::to_string(iterator));
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == pal4::CheckProtectTask::kSize,
              "CheckProtect task allocation is 60 bytes");
        calls.push_back("allocate");
        return allocation;
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
    void InitializeContainer(std::uint32_t container) override {
        Check(container == kTask + 4u, "container begins at task+4");
        calls.push_back("init_container");
        if (throw_on_container_init) throw std::runtime_error("init");
    }
    void DestroyContainer(std::uint32_t container) override {
        Check(container == kTask + 4u, "container cleanup begins at task+4");
        calls.push_back("destroy_container");
    }
    void AssignName(std::uint32_t name,
                    std::string_view value) override {
        Check(name == kTask + 20u && value == "CheckProtect",
              "task receives CheckProtect name");
        calls.push_back("assign_name");
        if (throw_on_assign) throw std::runtime_error("name");
    }
    void CleanupName(std::uint32_t name) override {
        Check(name == kTask + 20u, "name cleanup begins at task+20");
        calls.push_back("cleanup_name");
    }
};

std::size_t Index(const Runtime& runtime, const std::string& call) {
    const auto it = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(it != runtime.calls.end(), "expected call occurred");
    return static_cast<std::size_t>(it - runtime.calls.begin());
}

void DirectConstruction() {
    Runtime runtime;
    Check(pal4::CheckProtectTask::Create(kOwner, kActor, runtime) == kTask,
          "ready actor and ready protector construct task");
    Check(runtime.words[kTask] == pal4::CheckProtectTask::kVtable &&
          runtime.words[kTask + 24u] == 0u &&
          runtime.words[kTask + 28u] == 0u &&
          runtime.words[kTask + 32u] == 0u &&
          runtime.words[kTask + 36u] == 0u &&
          runtime.words[kTask + 44u] == kActor &&
          runtime.words[kTask + 48u] == kPrimary &&
          runtime.words[kTask + 52u] == 0u &&
          runtime.bytes[kTask + 20u] ==
              static_cast<std::uint8_t>(kActor) &&
          runtime.bytes[kTask + 40u] == 1u &&
          runtime.bytes[kTask + 56u] == 9u,
          "task fields match 60-byte IDA layout");
    Check(Index(runtime, "allocate") < Index(runtime, "init_container") &&
          Index(runtime, "init_container") < Index(runtime, "assign_name") &&
          Index(runtime, "assign_name") < Index(runtime, "write:24576"),
          "constructor orders container, name and final vtable");
    Check(std::find(runtime.calls.begin(), runtime.calls.end(),
                    "open:12288") == runtime.calls.end(),
          "ready protector avoids first iterator");
}

void FirstCollectionFallback() {
    Runtime runtime;
    runtime.ready[kPrimary] = false;
    runtime.ready[5u] = false;
    runtime.ready[6u] = true;
    runtime.candidates[kPrimary + 0x100u] = {5u, kActor, 6u};
    runtime.words[6u + 804u] = 2u;
    Check(pal4::CheckProtectTask::Create(kOwner, kActor, runtime) == kTask &&
          runtime.words[kTask + 48u] == 6u &&
          runtime.position[kPrimary + 0x100u] == 2u,
          "first collection chooses first ready actor other than input");
    Check(Index(runtime, "open:12288") <
              Index(runtime, "delete:12544") &&
          Index(runtime, "delete:12544") < Index(runtime, "allocate"),
          "first iterator is deleted before task allocation");
}

void SecondCollectionFallback() {
    Runtime runtime;
    runtime.words[kPrimary + 804u] = 1u;
    runtime.ready[7u] = true;
    runtime.candidates[kSecondary + 0x100u] = {kActor, 7u};
    Check(pal4::CheckProtectTask::Create(kOwner, kActor, runtime) == kTask &&
          runtime.words[kTask + 48u] == 7u &&
          runtime.position[kSecondary + 0x100u] == 1u,
          "same-state protector triggers second collection");
    Check(Index(runtime, "open:16384") <
              Index(runtime, "delete:16640") &&
          Index(runtime, "delete:16640") < Index(runtime, "allocate"),
          "second iterator is deleted before allocation");
}

void RejectionAndAllocationFailure() {
    Runtime runtime;
    runtime.ready[kActor] = false;
    Check(pal4::CheckProtectTask::Create(kOwner, kActor, runtime) == 0u &&
          runtime.calls.size() == 1u,
          "unready input returns before reading owner");
    runtime = Runtime{};
    runtime.ready[kPrimary] = false;
    Check(pal4::CheckProtectTask::Create(kOwner, kActor, runtime) == 0u &&
          runtime.calls.back() == "delete:12544",
          "empty first collection releases iterator and returns zero");
    runtime = Runtime{};
    runtime.words[kPrimary + 804u] = 1u;
    Check(pal4::CheckProtectTask::Create(kOwner, kActor, runtime) == 0u &&
          runtime.calls.back() == "delete:16640",
          "empty second collection releases iterator and returns zero");
    runtime = Runtime{};
    runtime.allocation = 0u;
    Check(pal4::CheckProtectTask::Create(kOwner, kActor, runtime) == 0u &&
          runtime.calls.back() == "allocate",
          "allocation failure performs no partial initialization");
}

void ExceptionCleanup() {
    Runtime runtime;
    runtime.throw_on_assign = true;
    bool caught = false;
    try {
        (void)pal4::CheckProtectTask::Create(kOwner, kActor, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught &&
          Index(runtime, "assign_name") < Index(runtime, "cleanup_name") &&
          Index(runtime, "cleanup_name") <
              Index(runtime, "destroy_container") &&
          runtime.calls.back() == "free:24576",
          "name exception unwinds name, container and allocation");

    runtime = Runtime{};
    runtime.throw_on_container_init = true;
    caught = false;
    try {
        (void)pal4::CheckProtectTask::Create(kOwner, kActor, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.calls.back() == "free:24576" &&
          std::find(runtime.calls.begin(), runtime.calls.end(),
                    "destroy_container") == runtime.calls.end(),
          "container initialization failure frees raw task only");
}

}  // namespace

int main() {
    DirectConstruction();
    FirstCollectionFallback();
    SecondCollectionFallback();
    RejectionAndAllocationFailure();
    ExceptionCleanup();
    std::cout << "CheckProtect task tests passed\n";
}
