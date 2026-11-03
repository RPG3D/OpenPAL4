#include "pal4/script_random_talk.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kExecutor = 0x5000u;
constexpr std::uint32_t kArray = kExecutor + 0x10u;
constexpr std::uint32_t kSource = 0x7000u;

class Records final : public pal4::ISelectDialogOptionArrayRuntime {
public:
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> copies;
    std::vector<std::uint32_t> frees;
    std::vector<std::uint32_t> allocations;

    Records() {
        words[kSource + 4u] = 0x7101u;
        words[kSource + 8u] = 3u;
        words[kSource + 12u] = 4u;
        bytes[0x7100u] = 1u;
    }

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    void Free(std::uint32_t address) override { frees.push_back(address); }
    std::uint32_t Allocate(std::uint32_t size) override {
        allocations.push_back(size);
        return 0x9000u;
    }
    void CopyRecord(std::uint32_t destination,
                    std::uint32_t source) override {
        copies.emplace_back(destination, source);
        for (std::uint32_t offset = 0u; offset < 16u; offset += 4u)
            words[destination + offset] = words[source + offset];
    }
    std::uint32_t EmptyStringPointer() override {
        throw std::runtime_error("unexpected empty string lookup");
    }
    bool AllocateBuffer(std::uint32_t, std::uint32_t, bool) override {
        throw std::runtime_error("unexpected record replacement allocation");
    }
    void CopyBytes(std::uint32_t, std::uint32_t, std::uint32_t) override {
        throw std::runtime_error("unexpected record replacement copy");
    }
    void DetachAndReset(std::uint32_t) override {
        throw std::runtime_error("unexpected record detachment");
    }
    void SetSize(std::uint32_t, std::uint32_t) override {
        throw std::runtime_error("unexpected record size change");
    }
    void ThrowRangeError() override {
        throw std::out_of_range("random talk record range");
    }
};

class Service final : public pal4::IScriptRandomTalkService {
public:
    pal4::ScenarioDialogWaitState state{};
    std::vector<std::string> calls;
    bool allocation_fails = false;
    std::uint8_t visible = 0u;
    int executor_calls = 0;
    std::uint32_t selected_record = 0x1030u;

    std::uint32_t ScriptExecutor() override {
        calls.emplace_back("executor");
        ++executor_calls;
        return kExecutor;
    }
    pal4::ScenarioDialogWaitState* AllocateHandler(
        std::uint32_t size) override {
        Check(size == 0x20u, "scenario wait handler allocation size");
        calls.emplace_back("allocate handler");
        return allocation_fails ? nullptr : &state;
    }
    void ConstructBase(pal4::ScenarioDialogWaitState& handler,
                       std::uint32_t mode,
                       std::uint32_t queue_address) override {
        Check(&handler == &state && mode == 1u &&
                  queue_address == 0x5054u,
              "blocking scenario wait constructor");
        state.vtable_address = 0xdeadbeefu;
        calls.emplace_back("construct");
    }
    void Flush(pal4::ScenarioDialogWaitState& handler) override {
        Check(&handler == &state &&
                  handler.vtable_address == 0x849520u,
              "scenario wait vtable before flush");
        calls.emplace_back("flush");
    }
    void AddHandler(std::uint32_t queue_address,
                    pal4::ScenarioDialogWaitState* handler) override {
        Check(queue_address == 0x5054u &&
                  handler == (allocation_fails ? nullptr : &state),
              "queue receives handler or null allocation result");
        calls.emplace_back("add handler");
    }
    std::int32_t Random() override {
        calls.emplace_back("random");
        return 7;
    }
    std::uint32_t UiFrameManager() override {
        calls.emplace_back("ui manager");
        return 0x6000u;
    }
    std::uint32_t CreateScenarioDialog(std::uint32_t manager) override {
        Check(manager == 0x6000u, "dialog manager");
        calls.emplace_back("dialog");
        return 0x6100u;
    }
    std::uint8_t IsVisible(std::uint32_t dialog) override {
        Check(dialog == 0x6100u, "visibility receiver");
        calls.emplace_back("visible");
        return visible;
    }
    std::uint8_t SetTextRecord(std::uint32_t dialog,
                               std::uint32_t record,
                               std::uint32_t display_value) override {
        Check(dialog == 0x6100u && record == selected_record &&
                  display_value == 0x3dcccccdu,
              "selected record and display value");
        calls.emplace_back("set text");
        return 0x39u;
    }
};

void AppendCases() {
    Service service;
    Records empty;
    pal4::ScriptRandomTalk talk(empty, service);
    Check(talk.Push(kSource) == 0u &&
              empty.allocations == std::vector<std::uint32_t>{16u} &&
              empty.words[kArray + 4u] == 0x9000u &&
              empty.words[kArray + 8u] == 0x9010u &&
              empty.words[kArray + 12u] == 0x9010u &&
              empty.copies == std::vector<std::pair<std::uint32_t,
                                                  std::uint32_t>>{
                  {0x9000u, kSource}} &&
              empty.frees == std::vector<std::uint32_t>{0u},
          "empty array allocates one record and returns old null begin");

    Records full;
    full.words[kArray + 4u] = 0x1000u;
    full.words[kArray + 8u] = 0x1020u;
    full.words[kArray + 12u] = 0x1020u;
    full.words[0x1004u] = 0x2001u;
    full.words[0x1014u] = 0x3001u;
    full.bytes[0x2000u] = 0u;
    full.bytes[0x3000u] = 2u;
    pal4::ScriptRandomTalk full_talk(full, service);
    Check(full_talk.Push(kSource) == 0x1000u &&
              full.allocations == std::vector<std::uint32_t>{64u} &&
              full.words[kArray + 4u] == 0x9000u &&
              full.words[kArray + 8u] == 0x9030u &&
              full.words[kArray + 12u] == 0x9040u &&
              full.copies == std::vector<std::pair<std::uint32_t,
                                                 std::uint32_t>>{
                  {0x9000u, 0x1000u}, {0x9010u, 0x1010u},
                  {0x9020u, kSource}} &&
              full.bytes[0x3000u] == 1u &&
              full.frees == std::vector<std::uint32_t>{0x2000u, 0x1000u},
          "full array doubles capacity, copies records and cleans old storage");

    Records spare;
    spare.words[kArray + 4u] = 0x1000u;
    spare.words[kArray + 8u] = 0x1020u;
    spare.words[kArray + 12u] = 0x1040u;
    pal4::ScriptRandomTalk spare_talk(spare, service);
    Check(spare_talk.Push(kSource) == 0x1030u &&
              spare.allocations.empty() &&
              spare.words[kArray + 8u] == 0x1030u &&
              spare.copies == std::vector<std::pair<std::uint32_t,
                                                  std::uint32_t>>{
                  {0x1020u, kSource}},
          "available capacity appends in place and returns new end");
    Check(service.executor_calls == 3,
          "each push reacquires script executor once");
}

void ShowCases() {
    Records records;
    records.words[kArray + 4u] = 0x1000u;
    records.words[kArray + 8u] = 0x1040u;
    Service service;
    pal4::ScriptRandomTalk talk(records, service);
    Check(talk.Show() == 0x39u &&
              service.calls == std::vector<std::string>{
                  "allocate handler", "executor", "construct", "flush",
                  "executor", "add handler", "executor", "random",
                  "ui manager", "dialog", "visible", "executor", "set text"} &&
              service.executor_calls == 4,
          "hidden dialog selects CRT random remainder and reacquires executor");

    Service visible;
    visible.visible = 0x7fu;
    pal4::ScriptRandomTalk visible_talk(records, visible);
    Check(visible_talk.Show() == 0x7fu &&
              visible.calls.back() == "visible" &&
              visible.executor_calls == 3,
          "visible dialog returns visibility byte without text update");

    Service failed;
    failed.allocation_fails = true;
    pal4::ScriptRandomTalk failed_talk(records, failed);
    Check(failed_talk.Show() == 0x39u &&
              failed.executor_calls == 3 &&
              failed.calls[1] == "executor" &&
              failed.calls[2] == "add handler",
          "failed allocation still queues null and displays selection");

    Records empty;
    Service empty_service;
    pal4::ScriptRandomTalk empty_talk(empty, empty_service);
    try {
        static_cast<void>(empty_talk.Show());
        throw std::runtime_error("empty candidate vector was accepted");
    } catch (const std::domain_error&) {
        Check(empty_service.calls.back() == "executor",
              "empty vector fails after handler registration");
    }
}

void ReleaseCases() {
    Records records;
    records.words[kArray + 4u] = 0x1000u;
    records.words[kArray + 8u] = 0x1020u;
    records.words[kArray + 12u] = 0x1040u;
    records.words[0x1004u] = 0x2001u;
    records.words[0x1014u] = 0x3001u;
    records.bytes[0x2000u] = 0u;
    records.bytes[0x3000u] = 2u;
    Service service;
    pal4::ScriptRandomTalk talk(records, service);
    Check(talk.Release() == 0x1020u &&
              records.words[kArray + 4u] == 0x1000u &&
              records.words[kArray + 8u] == 0x1000u &&
              records.words[kArray + 12u] == 0x1040u &&
              records.words[0x1004u] == 0u &&
              records.words[0x1014u] == 0u &&
              records.bytes[0x3000u] == 1u &&
              records.frees == std::vector<std::uint32_t>{0x2000u},
          "release cleans candidate strings and retains allocated capacity");
    Check(talk.Release() == 0x1000u &&
              records.frees == std::vector<std::uint32_t>{0x2000u},
          "repeated release sees empty range and returns current end");
}

}  // namespace

int main() {
    try {
        AppendCases();
        ShowCases();
        ReleaseCases();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
