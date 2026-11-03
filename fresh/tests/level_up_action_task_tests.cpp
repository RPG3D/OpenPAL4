#include "pal4/level_up_action_task.h"

#include <algorithm>
#include <array>
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
constexpr std::uint32_t kChild = 0x2000u;
constexpr std::uint32_t kActor = 0x3000u;
constexpr std::uint32_t kHeader = 0x8fa2b8u;
constexpr std::uint32_t kData = 0x7188u;
constexpr std::uint32_t kOldData = 0x8100u;

struct Runtime final : pal4::ILevelUpActionRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 44u, kActor}, {0x8f95d0u, 0xa000u},
        {kHeader + 4u, kData}, {kHeader + 8u, 5u},
        {kHeader + 12u, 8u}, {kChild + 68u, kOldData},
        {kChild + 72u, 3u}, {kChild + 76u, 4u}};
    std::map<std::uint32_t, std::uint8_t> bytes{
        {kData - 1u, 1u}, {kOldData - 1u, 3u},
        {0x841963u, 1u}};
    std::vector<std::string> calls;
    std::uint32_t allocation = kChild;
    bool allocate_text = true;
    std::uint8_t detach_result = 0x5au;
    std::uint32_t alias_first_length = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        if (address == kHeader + 8u && allocation + 64u == kHeader &&
            alias_first_length != 0u) {
            const auto value = alias_first_length;
            alias_first_length = 0u;
            return value;
        }
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
    void SetActorState(std::uint32_t actor, std::uint32_t state,
                       std::uint32_t enabled) override {
        Check(actor == kActor && state == 7u && enabled == 1u,
              "actor state 7 enabled");
        calls.push_back("set_state");
    }
    std::uint32_t CreateChildTask(std::uint32_t factory) override {
        Check(factory == 0x845a84u, "shared WinAction third factory");
        calls.push_back("create");
        return allocation;
    }
    void AddTaskAndInitialize(
        std::uint32_t container, std::uint32_t child,
        std::uint32_t text,
        const std::array<std::uint32_t, 3>& timing) override {
        Check(container == kTask + 4u && child == allocation &&
                  text == (words[0x8f95d0u] == 0u ? 0x841964u : 0xa000u) &&
                  timing == std::array<std::uint32_t, 3>{1u, 0u, 0u},
              "level-up child GOM arguments");
        calls.push_back("add");
    }
    void FreeText(std::uint32_t address) override {
        Check(address == kOldData - 1u, "old text allocation released");
        calls.push_back("free_text");
    }
    bool AllocateText(std::uint32_t header, std::uint32_t length,
                      std::uint8_t flag) override {
        Check(header == allocation + 64u && flag <= 1u,
              "level-up text allocation args");
        calls.push_back("allocate_text:" + std::to_string(length) +
                        ":" + std::to_string(flag));
        if (allocate_text) words[header + 4u] = 0xd000u;
        return allocate_text;
    }
    void CopyText(std::uint32_t header,
                  std::uint32_t source,
                  std::uint32_t length) override {
        Check(header == allocation + 64u &&
                  (source == kData || source == 0x841964u),
              "level-up text copy args");
        calls.push_back("copy:" + std::to_string(length));
    }
    void DetachText(std::uint32_t header) override {
        Check(header == allocation + 64u, "alias first detach");
        calls.push_back("detach");
    }
    void MoveTextBytes(std::uint32_t, std::uint32_t,
                       std::uint32_t length) override {
        Check(length == 0u, "alias movement byte count");
        calls.push_back("move");
    }
    void SetTextSize(std::uint32_t header,
                     std::uint32_t length) override {
        Check(header == allocation + 64u && length == 5u,
              "alias resized length");
        calls.push_back("set_size");
    }
    std::uint8_t DetachTextAndReturn(std::uint32_t header) override {
        Check(header == allocation + 64u, "alias final detach");
        calls.push_back("detach_return");
        return detach_result;
    }
};

std::size_t Index(const Runtime& runtime, const std::string& call) {
    const auto it = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(it != runtime.calls.end(), "expected call");
    return static_cast<std::size_t>(it - runtime.calls.begin());
}

bool Called(const Runtime& runtime, const std::string& call) {
    return std::find(runtime.calls.begin(), runtime.calls.end(), call) !=
           runtime.calls.end();
}

void SharedTextAndOldOwnership() {
    Runtime r;
    Check(pal4::LevelUpActionTask::Start(kTask, r) == 0x88u &&
              r.words[kChild + 60u] == kActor &&
              r.words[kChild + 68u] == kData &&
              r.words[kChild + 72u] == 5u &&
              r.words[kChild + 76u] == 8u &&
              r.bytes[kData - 1u] == 2u &&
              r.bytes[kOldData - 1u] == 2u &&
              Index(r, "set_state") < Index(r, "create") &&
              Index(r, "create") < Index(r, "add") &&
              Index(r, "write_byte:33023") <
                  Index(r, "write:8260"),
          "state, scheduling, old reference release and shared layout");

    for (std::uint8_t old_count : {0u, 0xffu}) {
        r = Runtime{};
        r.bytes[kOldData - 1u] = old_count;
        Check(pal4::LevelUpActionTask::Start(kTask, r) == 0x88u &&
                  Called(r, "free_text"),
              "zero and immortal old reference values free allocation");
    }

    r = Runtime{};
    r.words[kChild + 68u] = 0u;
    Check(pal4::LevelUpActionTask::Start(kTask, r) == 0x88u &&
              !Called(r, "free_text"),
          "empty destination has no old allocation");
}

void CopyAndFallback() {
    Runtime r;
    r.bytes[kData - 1u] = 0xfeu;
    Check(pal4::LevelUpActionTask::Start(kTask, r) == 5u &&
              Called(r, "allocate_text:5:1") && Called(r, "copy:5") &&
              r.bytes[0xd005u] == 0u && !Called(r, "free_text"),
          "saturated source copies without shared release path");

    r = Runtime{};
    r.words[kHeader + 8u] = 0u;
    Check(pal4::LevelUpActionTask::Start(kTask, r) == 0u &&
              Called(r, "allocate_text:0:1") && Called(r, "copy:0"),
          "zero length allocates and copies zero bytes");

    r = Runtime{};
    r.bytes[kData - 1u] = 0xfeu;
    r.allocate_text = false;
    Check(pal4::LevelUpActionTask::Start(kTask, r) == 0u &&
              !Called(r, "copy:5"),
          "allocation failure stops before copy");

    r = Runtime{};
    r.words[0x8f95d0u] = 0u;
    r.words[kHeader + 4u] = 0u;
    Check(pal4::LevelUpActionTask::Start(kTask, r) == 0x64u &&
              r.words[kChild + 68u] == 0x841964u &&
              r.bytes[0x841963u] == 2u,
          "both task text and global data use empty fallback");
}

void AliasAndNullChild() {
    Runtime r;
    r.allocation = kHeader - 64u;
    Check(pal4::LevelUpActionTask::Start(kTask, r) == r.detach_result &&
              Index(r, "detach") < Index(r, "detach_return") &&
              !Called(r, "move"),
          "alias text detaches twice for equal length");

    r = Runtime{};
    r.allocation = kHeader - 64u;
    r.words[kHeader + 8u] = 4u;
    r.alias_first_length = 5u;
    bool caught = false;
    try { (void)pal4::LevelUpActionTask::Start(kTask, r); }
    catch (const std::out_of_range&) { caught = true; }
    Check(caught, "alias length overflow throws before detach");

    r = Runtime{};
    r.allocation = 0u;
    Check(pal4::LevelUpActionTask::Start(kTask, r) == 0u &&
              Called(r, "set_state") && Called(r, "create") &&
              !Called(r, "add"),
          "null child returns safely after state update");
}

}  // namespace

int main() {
    try {
        Check(pal4::LevelUpActionTask::kVtable == 0x845cd4u,
              "LevelUp task vtable");
        SharedTextAndOldOwnership();
        CopyAndFallback();
        AliasAndNullChild();
        std::cout << "LevelUp action tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
