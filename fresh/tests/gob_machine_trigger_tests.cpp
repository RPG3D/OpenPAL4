#include "pal4/gob_machine_trigger.h"

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

struct Runtime final : pal4::IGobMachineTriggerRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t conditions = 1u;
    std::uint32_t start_result = 1u;
    std::string crc_input;
    std::uint32_t dispatched_crc = 0u;

    Runtime() {
        words[object + 0x27cu] = 0u;
        words[object + 0x198u] = 2u;
        words[object + 0x1b0u] = 0u;
        words[object + 4u] = 0x4000u;
        words[object + 0x14u] = 0x5000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write " + std::to_string(address - object));
    }
    std::uint32_t MovementAdapterRoot() override {
        calls.emplace_back("adapter root");
        return 0x7000u;
    }
    void SetClumpRenderCallback(std::uint32_t adapter,
                                std::uint32_t clump) override {
        Check(adapter == 0x7000u && clump == 0x5000u,
              "movement adapter clump");
        calls.emplace_back("set render callback");
    }
    std::uint32_t ForwardMovementSlot84(
        std::uint32_t address) override {
        Check(address == object, "movement forward object");
        calls.emplace_back("forward movement");
        return 0x55u;
    }
    void ClearTaskCollection(std::uint32_t address) override {
        Check(address == object + 0x258u ||
              address == object + 0x268u,
              "trigger collection address");
        calls.emplace_back(address == object + 0x258u ?
                           "clear primary" : "clear context");
    }
    std::uint32_t CheckAndConsumeInventoryConditions(
        std::uint32_t address) override {
        Check(address == object, "condition object");
        calls.emplace_back("check conditions");
        return conditions;
    }
    std::uint32_t StartTaskRun(std::uint32_t address) override {
        Check(address == object, "start task object");
        calls.emplace_back("start task run");
        return start_result;
    }
    void MarkEventProcessed(std::uint32_t event) override {
        Check(event == object + 0x258u, "processed event address");
        calls.emplace_back("mark event processed");
    }
    std::string ArenaName() override {
        calls.emplace_back("arena name");
        return "Arena";
    }
    std::string SceneName() override {
        calls.emplace_back("scene name");
        return "Scene";
    }
    std::string ObjectName(std::uint32_t address) override {
        Check(address == 0x4000u, "object name address");
        calls.emplace_back("object name");
        return "Lever";
    }
    std::uint32_t Crc32(const std::string& value) override {
        crc_input = value;
        calls.emplace_back("crc32");
        return 0x1234u;
    }
    void DispatchGameEvent(std::uint32_t crc) override {
        dispatched_crc = crc;
        calls.emplace_back("dispatch game event");
    }
    std::uint32_t ReleaseEventNameBuffer() override {
        calls.emplace_back("release event name");
        return 0x77u;
    }
};

void CheckMovementForwarder() {
    Runtime runtime;
    Check(pal4::GobMachineTrigger::InitializeMovementAdapterAndCall(
              Runtime::object, runtime) == 0x55u &&
          runtime.calls == (std::vector<std::string>{
              "adapter root", "set render callback",
              "forward movement"}),
          "movement adapter callback and forward order");
}

void CheckTriggerGates() {
    Runtime running;
    running.words[Runtime::object + 0x27cu] = 9u;
    Check(pal4::GobMachineTrigger::TryStartTaskRun(
              Runtime::object, running) == 9u &&
          running.calls.empty(), "already running gate");
    Runtime no_uses;
    no_uses.words[Runtime::object + 0x198u] = 0u;
    Check(pal4::GobMachineTrigger::TryStartTaskRun(
              Runtime::object, no_uses) == 0u &&
          no_uses.calls.empty(), "common use-count gate");
    Runtime denied;
    denied.conditions = 0u;
    Check(pal4::GobMachineTrigger::TryStartTaskRun(
              Runtime::object, denied) == 0u &&
          denied.calls == (std::vector<std::string>{
              "clear primary", "clear context", "check conditions"}) &&
          denied.words.at(Runtime::object + 0x198u) == 2u,
          "failed conditions preserve count");
    Runtime failed_start;
    failed_start.start_result = 0u;
    Check(pal4::GobMachineTrigger::TryStartTaskRun(
              Runtime::object, failed_start) == 0u &&
          failed_start.words.at(Runtime::object + 0x198u) == 1u &&
          failed_start.calls.back() == "start task run",
          "failed task build follows use decrement");
}

void CheckSuccessfulTrigger() {
    Runtime started;
    Check(pal4::GobMachineTrigger::TryStartTaskRun(
              Runtime::object, started) == 1u &&
          started.words.at(Runtime::object + 0x198u) == 1u &&
          started.words.at(Runtime::object + 0x278u) == 0u &&
          started.words.at(Runtime::object + 0x27cu) == 1u &&
          started.words.at(Runtime::object + 0x2b4u) == 1u &&
          started.calls == (std::vector<std::string>{
              "clear primary", "clear context", "check conditions",
              "write 408", "start task run", "mark event processed",
              "write 632", "write 636", "write 692"}),
          "successful trigger order and state");
    Runtime unlimited;
    unlimited.words[Runtime::object + 0x198u] = 0xffffffffu;
    Check(pal4::GobMachineTrigger::TryStartTaskRun(
              Runtime::object, unlimited) == 1u &&
          unlimited.words.at(Runtime::object + 0x198u) == 0xffffffffu,
          "negative count remains unlimited");
    Runtime event;
    event.words[Runtime::object + 0x1b0u] = 4u;
    Check(pal4::GobMachineTrigger::TryStartTaskRun(
              Runtime::object, event) == 0x77u &&
          event.crc_input == "Arena-Scene-Lever" &&
          event.dispatched_crc == 0x1234u &&
          event.calls[event.calls.size() - 3u] == "crc32" &&
          event.calls[event.calls.size() - 2u] ==
              "dispatch game event" &&
          event.calls.back() == "release event name",
          "type-4 event name, CRC and dispatch");
}

}  // namespace

int main() {
    try {
        CheckMovementForwarder();
        CheckTriggerGates();
        CheckSuccessfulTrigger();
        std::cout << "gob_machine_trigger_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
