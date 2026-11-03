#include "pal4/physical_attack_task.h"

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
constexpr std::uint32_t kChild = 0x2000u;
constexpr std::uint32_t kSource = 0x3000u;
constexpr std::uint32_t kProtector = 0x4000u;
constexpr std::uint32_t kText = 0x6001u;
constexpr std::uint32_t kHeader = 0x8fa38cu;

struct Runtime final : pal4::IPhysicalAttackStartRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 44u, kSource}, {kTask + 48u, kProtector},
        {0x8f9680u, 0x7000u}, {kHeader + 4u, kText},
        {kHeader + 8u, 5u}, {kHeader + 12u, 31u}};
    std::map<std::uint32_t, std::uint8_t> bytes{
        {kTask + 53u, 5u}, {kTask + 54u, 9u},
        {kText - 1u, 2u}, {0x841964u - 1u, 0xffu}};
    std::vector<std::string> calls;
    std::uint32_t child = kChild;
    bool allocate_text = true;

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
    std::uint32_t CreateChildTask(
        const pal4::PhysicalAttackChildFactorySlot& factory) override {
        Check(factory.vtable == pal4::PhysicalAttackChildFactory::kVtable,
              "child factory vtable matches IDA");
        calls.push_back("create");
        return child;
    }
    void AddTaskAndInitialize(
        std::uint32_t container, std::uint32_t task,
        std::uint32_t source_text,
        const std::array<std::uint32_t, 3>& timing) override {
        Check(container == kTask + 4u && task == child &&
                  source_text == (words[0x8f9680u] == 0u
                                      ? 0x841964u : words[0x8f9680u]) &&
                  timing == std::array<std::uint32_t, 3>{1u, 0u, 0u},
              "child is attached with source text and base timing");
        calls.push_back("add");
    }
    void EraseText(std::uint32_t header, std::uint32_t start,
                   std::uint32_t count) override {
        Check(header == kHeader, "alias erase uses global header");
        calls.push_back("erase:" + std::to_string(start) + ":" +
                        std::to_string(count));
    }
    void SetTextErrorFlag(std::uint32_t header,
                          std::uint8_t flag) override {
        Check(header == child + 64u && flag == 1u,
              "share path marks destination header");
        calls.push_back("set_error_flag");
    }
    bool AllocateText(std::uint32_t header, std::uint32_t length,
                      std::uint8_t flag) override {
        Check(header == child + 64u &&
                  length == words[kHeader + 8u] && flag == 1u,
              "copy path allocates exact global text length");
        calls.push_back("allocate_text");
        if (allocate_text) words[child + 68u] = 0x8000u;
        return allocate_text;
    }
    void CopyText(std::uint32_t header, std::uint32_t source,
                  std::uint32_t length) override {
        Check(header == child + 64u &&
                  source == (words[kHeader + 4u] == 0u
                                 ? 0x841964u : words[kHeader + 4u]) &&
                  length == words[kHeader + 8u],
              "copy path reads source and exact length");
        calls.push_back("copy_text");
        bytes[words[child + 68u] + length] = 0u;
    }
};

std::size_t Index(const Runtime& runtime, const std::string& call) {
    const auto it = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(it != runtime.calls.end(), "expected runtime call");
    return static_cast<std::size_t>(it - runtime.calls.begin());
}

void FactoryAndSharedText() {
    pal4::PhysicalAttackChildFactorySlot slot;
    Check(pal4::PhysicalAttackChildFactory::Initialize(&slot) == &slot &&
          slot.vtable == pal4::PhysicalAttackChildFactory::kVtable,
          "factory writes vtable and returns slot");
    Runtime runtime;
    Check(pal4::PhysicalAttackTask::Start(kTask, runtime) == 5u &&
          runtime.words[kChild + 60u] == kSource &&
          runtime.words[kChild + 68u] == kText &&
          runtime.words[kChild + 72u] == 5u &&
          runtime.words[kChild + 76u] == 31u &&
          runtime.words[kChild + 276u] == kSource &&
          runtime.words[kChild + 280u] == kProtector &&
          runtime.bytes[kChild + 288u] == 5u &&
          runtime.bytes[kChild + 289u] == 9u &&
          runtime.bytes[kText - 1u] == 3u,
          "share path retains text and writes attack child fields");
    Check(Index(runtime, "create") < Index(runtime, "add") &&
          Index(runtime, "add") < Index(runtime, "set_error_flag") &&
          Index(runtime, "set_error_flag") <
              Index(runtime, "write:" + std::to_string(kChild + 68u)) &&
          runtime.calls.back() ==
              "write_byte:" + std::to_string(kChild + 289u),
          "factory, GOM add, text share and final flags follow IDA order");
}

void CopyAndAllocationFailure() {
    Runtime runtime;
    runtime.bytes[kText - 1u] = 0xfeu;
    Check(pal4::PhysicalAttackTask::Start(kTask, runtime) == 5u &&
          runtime.words[kChild + 68u] == 0x8000u &&
          runtime.words[kChild + 72u] == 5u &&
          runtime.bytes[0x8000u + 5u] == 0u &&
          Index(runtime, "allocate_text") < Index(runtime, "copy_text"),
          "saturated reference count allocates and copies text");
    runtime = Runtime{};
    runtime.words[kHeader + 8u] = 0u;
    Check(pal4::PhysicalAttackTask::Start(kTask, runtime) == 5u &&
          Index(runtime, "allocate_text") < Index(runtime, "copy_text"),
          "zero-length text follows allocation path");
    runtime = Runtime{};
    runtime.bytes[kText - 1u] = 0xfeu;
    runtime.allocate_text = false;
    Check(pal4::PhysicalAttackTask::Start(kTask, runtime) == 5u &&
          std::find(runtime.calls.begin(), runtime.calls.end(),
                    "copy_text") == runtime.calls.end() &&
          runtime.words[kChild + 280u] == kProtector,
          "text allocation failure still sets actor fields");
}

void AliasAndNullChild() {
    Runtime runtime;
    runtime.child = kHeader - 64u;
    Check(pal4::PhysicalAttackTask::Start(kTask, runtime) == 5u &&
          Index(runtime, "erase:5:4294967295") <
              Index(runtime, "erase:0:0") &&
          std::find(runtime.calls.begin(), runtime.calls.end(),
                    "allocate_text") == runtime.calls.end(),
          "destination alias executes two erasures without sharing");
    runtime = Runtime{};
    runtime.child = 0u;
    Check(pal4::PhysicalAttackTask::Start(kTask, runtime) == 0u &&
          runtime.calls.size() == 2u,
          "null child safely exits after global text and factory calls");
}

void EmptyGlobalPointers() {
    Runtime runtime;
    runtime.words[0x8f9680u] = 0u;
    runtime.words[kHeader + 4u] = 0u;
    runtime.bytes[0x841964u - 1u] = 0xfeu;
    Check(pal4::PhysicalAttackTask::Start(kTask, runtime) == 5u &&
          Index(runtime, "allocate_text") < Index(runtime, "copy_text"),
          "null global strings use empty fallback in both call paths");
}

}  // namespace

int main() {
    FactoryAndSharedText();
    CopyAndAllocationFailure();
    AliasAndNullChild();
    EmptyGlobalPointers();
    std::cout << "PhysicalAttack start tests passed\n";
}
