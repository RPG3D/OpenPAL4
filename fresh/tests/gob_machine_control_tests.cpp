#include "pal4/gob_machine.h"
#include "pal4/gob_machine_control.h"

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

struct Runtime final : pal4::IGobMachineControlRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> invoked;
    std::uint32_t parameters = 0x8000u;
    std::uint32_t machine = 0x9000u;
    std::uint32_t clear_result = 0x77u;
    bool extend_range = false;
    std::uint32_t tasks_node = 0xa000u;
    std::vector<std::uint32_t> children;
    std::map<std::pair<std::uint32_t, std::string>, std::int32_t> integers;
    std::map<std::pair<std::uint32_t, std::string>, std::string> strings;
    std::string action_name;
    std::uint32_t trigger_result = 0x99u;

    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write " + std::to_string(address - object));
    }
    void StartTaskRun(std::uint32_t address) override {
        Check(address == object, "start object");
        calls.emplace_back("start");
    }
    void MarkEventProcessed(std::uint32_t address) override {
        Check(address == object + 0x258u, "processed collection");
        calls.emplace_back("processed");
    }
    void SetPosition(std::uint32_t address, std::uint32_t vector,
                     std::uint32_t mode) override {
        Check(address == object && vector == object + 0x288u && mode == 0u,
              "restored position arguments");
        calls.emplace_back("position");
    }
    void SetRotation(std::uint32_t address, std::uint32_t vector,
                     std::uint32_t mode) override {
        Check(address == object && vector == object + 0x294u && mode == 0u,
              "restored rotation arguments");
        calls.emplace_back("rotation");
    }
    std::uint32_t ClearTaskCollection(std::uint32_t address) override {
        Check(address == object + 0x258u || address == object + 0x268u,
              "clear collection address");
        calls.emplace_back(address == object + 0x258u ?
                           "clear primary" : "clear context");
        return clear_result;
    }
    void InvokeTaskSlot(std::uint32_t task, std::uint32_t slot) override {
        invoked.push_back(task);
        calls.push_back("slot " + std::to_string(slot));
        if (extend_range && invoked.size() == 1u) {
            words[object + 0x2a8u] += 4u;
            words[0x5008u] = 0x7000u;
        }
    }
    std::uint32_t FindXmlChild(std::uint32_t parent,
                               std::string_view name) override {
        if (parent == 0x6000u && name == "PAL4-GameObject-Parameters") {
            calls.emplace_back("parameters");
            return parameters;
        }
        if (parent == parameters && name == "PAL4_GameObject-machine") {
            calls.emplace_back("machine");
            return machine;
        }
        Check(parent == machine && name == "PAL4_GameObject-machine-tasks",
              "task list lookup arguments");
        calls.emplace_back("tasks");
        return tasks_node;
    }
    void ApplyBaseParameters(std::uint32_t address) override {
        Check(address == object, "base parameter object");
        calls.emplace_back("base parameters");
    }
    std::int32_t ReadXmlInt(std::uint32_t node, std::string_view name,
                            std::int32_t fallback) override {
        calls.push_back(std::string(name));
        const auto found = integers.find({node, std::string(name)});
        return found == integers.end() ? fallback : found->second;
    }
    std::int32_t XmlChildCount(std::uint32_t node) override {
        Check(node == tasks_node, "child count node");
        return static_cast<std::int32_t>(children.size());
    }
    std::uint32_t XmlChildAt(std::uint32_t node,
                             std::int32_t index) override {
        Check(node == tasks_node && index >= 0 &&
              static_cast<std::size_t>(index) < children.size(),
              "indexed child arguments");
        return children[static_cast<std::size_t>(index)];
    }
    std::string ReadXmlString(std::uint32_t node, std::string_view name,
                              std::string_view fallback) override {
        Check(name == "PAL4_GOMTask_action_name", "action name key");
        calls.emplace_back("read action name");
        const auto found = strings.find({node, std::string(name)});
        return found == strings.end() ? std::string(fallback) : found->second;
    }
    void AssignActionName(std::uint32_t record,
                          std::string_view value) override {
        Check(record == object + 0x2bcu, "action string record");
        action_name = value;
        calls.emplace_back("assign action name");
    }
    void SetVisible(std::uint32_t address, std::uint32_t visible) override {
        Check(address == object, "visible object");
        words[object + 0x184u] = visible;
        calls.emplace_back("set visible");
    }
    std::uint32_t TriggerTaskRun(
        std::uint32_t address,
        const std::array<float, 3>& position) override {
        Check(address == object && position == std::array<float, 3>{},
              "repeat trigger arguments");
        calls.emplace_back("trigger");
        return trigger_result;
    }
};

void CheckStartAndReset() {
    Runtime runtime;
    for (std::uint32_t index = 0; index < 3u; ++index) {
        runtime.words[Runtime::object + 0xacu + 4u * index] = 10u + index;
        runtime.words[Runtime::object + 0xa0u + 4u * index] = 20u + index;
    }
    Check(pal4::GobMachine::StartAndSaveTransform(
              Runtime::object, runtime) == 20u, "start return rotation");
    for (std::uint32_t index = 0; index < 3u; ++index) {
        Check(runtime.words[Runtime::object + 0x288u + 4u * index] ==
                  10u + index, "saved position");
        Check(runtime.words[Runtime::object + 0x294u + 4u * index] ==
                  20u + index, "saved rotation");
    }
    Check(runtime.words[Runtime::object + 0x278u] == 0u &&
          runtime.words[Runtime::object + 0x27cu] == 1u &&
          runtime.calls[0] == "start" && runtime.calls[1] == "processed",
          "start event and flags order");
    runtime.calls.clear();
    runtime.words[Runtime::object + 0x278u] = 1u;
    runtime.words[Runtime::object + 0x198u] = 8u;
    runtime.words[Runtime::object + 0x284u] = 1u;
    Check(pal4::GobMachine::ResetTriggeredRuntime(
              Runtime::object, runtime) == 0x77u &&
          runtime.calls == (std::vector<std::string>{
              "position", "rotation", "clear primary", "clear context",
              "write 632", "write 636"}) &&
          runtime.words[Runtime::object + 0x278u] == 0u &&
          runtime.words[Runtime::object + 0x27cu] == 0u &&
          runtime.words[Runtime::object + 0x198u] == 8u &&
          runtime.words[Runtime::object + 0x284u] == 1u,
          "reset restores transform and clears both collections");
    runtime.calls.clear();
    Check(pal4::GobMachine::ResetTriggeredRuntime(
              Runtime::object, runtime) == 0x77u &&
          runtime.calls == (std::vector<std::string>{
              "clear primary", "clear context", "write 632", "write 636"}),
          "idle reset skips transform restore");
}

void CheckReadyAndSlots() {
    Runtime runtime;
    runtime.bytes[Runtime::object + 0x264u] = 1u;
    runtime.bytes[Runtime::object + 0x274u] = 0u;
    Check(!pal4::GobMachine::AreTaskContainersReady(
              Runtime::object, runtime), "context not ready");
    runtime.bytes[Runtime::object + 0x274u] = 2u;
    Check(pal4::GobMachine::AreTaskContainersReady(
              Runtime::object, runtime), "both containers ready");
    runtime.bytes[Runtime::object + 0x264u] = 0xabu;
    Check(pal4::GobMachine::InvokeTaskSlot28Range(
              Runtime::object, runtime) == 0xabu && runtime.invoked.empty(),
          "ready byte bypasses slot 28 range");
    Check(pal4::GobMachine::InvokeTaskSlot32Range(
              Runtime::object, runtime) == 0xabu && runtime.invoked.empty(),
          "ready byte bypasses slot 32 range");

    runtime.bytes[Runtime::object + 0x264u] = 0u;
    runtime.words[Runtime::object + 0x2a4u] = 0x5000u;
    runtime.words[Runtime::object + 0x2a8u] = 0x5008u;
    runtime.words[0x5000u] = 0x6000u;
    runtime.words[0x5004u] = 0x6100u;
    Check(pal4::GobMachine::InvokeTaskSlot28Range(
              Runtime::object, runtime) == 8u &&
          runtime.invoked == (std::vector<std::uint32_t>{0x6000u, 0x6100u}) &&
          runtime.calls == (std::vector<std::string>{"slot 28", "slot 28"}),
          "slot 28 iteration and end pointer low byte");
    runtime.invoked.clear();
    runtime.calls.clear();
    Check(pal4::GobMachine::InvokeTaskSlot32Range(
              Runtime::object, runtime) == 8u &&
          runtime.calls == (std::vector<std::string>{"slot 32", "slot 32"}),
          "slot 32 iteration");
    runtime.words[Runtime::object + 0x2a8u] = 0x5000u;
    runtime.invoked.clear();
    Check(pal4::GobMachine::InvokeTaskSlot28Range(
              Runtime::object, runtime) == 0u && runtime.invoked.empty(),
          "empty range returns end low byte");
    runtime.words[Runtime::object + 0x2a8u] = 0x5008u;
    runtime.extend_range = true;
    runtime.invoked.clear();
    Check(pal4::GobMachine::InvokeTaskSlot28Range(
              Runtime::object, runtime) == 12u &&
          runtime.invoked ==
              (std::vector<std::uint32_t>{0x6000u, 0x6100u, 0x7000u}),
          "range end is reread after each task");
}

void CheckMachineNode() {
    Runtime runtime;
    Check(pal4::GobMachine::GetMachineConfigNode(
              Runtime::object, runtime) == 0u && runtime.calls.empty(),
          "missing document");
    runtime.words[Runtime::object + 0x1c0u] = 0x6000u;
    runtime.parameters = 0u;
    Check(pal4::GobMachine::GetMachineConfigNode(
              Runtime::object, runtime) == 0u &&
          runtime.calls == (std::vector<std::string>{"parameters"}),
          "missing parameters node");
    runtime.parameters = 0x8000u;
    runtime.calls.clear();
    Check(pal4::GobMachine::GetMachineConfigNode(
              Runtime::object, runtime) == 0x9000u &&
          runtime.calls ==
              (std::vector<std::string>{"parameters", "machine"}),
          "machine child lookup");
    runtime.machine = 0u;
    Check(pal4::GobMachine::GetMachineConfigNode(
              Runtime::object, runtime) == 0u,
          "missing machine node");
}

void CheckMainConfig() {
    Runtime missing;
    Check(pal4::GobMachine::LoadMainConfigAndTaskFlags(
              Runtime::object, missing) == 0u &&
          missing.calls == (std::vector<std::string>{"base parameters"}) &&
          missing.words.count(Runtime::object + 0x280u) == 0u,
          "missing machine node applies base only");

    Runtime defaults;
    defaults.words[Runtime::object + 0x1c0u] = 0x6000u;
    defaults.words[Runtime::object + 0x2b8u] = 5u;
    Check(pal4::GobMachine::LoadMainConfigAndTaskFlags(
              Runtime::object, defaults) == 0xa000u &&
          defaults.words[Runtime::object + 0x280u] == 0u &&
          defaults.words[Runtime::object + 0x284u] == 0u &&
          defaults.words[Runtime::object + 0x2b0u] == 0u &&
          defaults.words[Runtime::object + 0x2b8u] == 5u &&
          defaults.calls[0] == "base parameters" &&
          defaults.calls[3] == "PAL4_GameObject-machine-repeat" &&
          defaults.calls[5] == "PAL4_GameObject-machine-auto-touch",
          "empty task list defaults and key order");

    Runtime configured;
    configured.words[Runtime::object + 0x1c0u] = 0x6000u;
    configured.integers[{0x9000u,
        "PAL4_GameObject-machine-repeat"}] = 7;
    configured.integers[{0x9000u,
        "PAL4_GameObject-machine-auto-touch"}] = 9;
    configured.children = {0xb000u, 0xb100u, 0xb200u, 0xb300u};
    configured.integers[{0xb000u, "PAL4_GOMTask_type"}] = 6;
    configured.integers[{0xb100u, "PAL4_GOMTask_type"}] = 3;
    configured.integers[{0xb200u, "PAL4_GOMTask_type"}] = 8;
    configured.strings[{0xb100u, "PAL4_GOMTask_action_name"}] = "OpenGate";
    Check(pal4::GobMachine::LoadMainConfigAndTaskFlags(
              Runtime::object, configured) == 4u &&
          configured.words[Runtime::object + 0x280u] == 7u &&
          configured.words[Runtime::object + 0x284u] == 9u &&
          configured.words[Runtime::object + 0x2b0u] == 1u &&
          configured.words[Runtime::object + 0x2b8u] == 1u &&
          configured.action_name == "OpenGate",
          "configured machine flags and action name");
    configured.children[2] = 0u;
    Check(pal4::GobMachine::LoadMainConfigAndTaskFlags(
              Runtime::object, configured) == 0u,
          "missing indexed task exits scan");
    configured.children[2] = 0xb200u;
    configured.tasks_node = 0u;
    Check(pal4::GobMachine::LoadMainConfigAndTaskFlags(
              Runtime::object, configured) == 0u,
          "missing task list exits after base flags");
}

void CheckVisibleRepeat() {
    for (const auto visible : {0u, 1u}) {
        for (const auto repeat : {0u, 1u}) {
            Runtime runtime;
            runtime.words[Runtime::object + 0x284u] = repeat;
            runtime.words[Runtime::object + 0x278u] = 1u;
            runtime.words[Runtime::object + 0x27cu] = 1u;
            const bool triggers = visible != 0u && repeat != 0u;
            Check(pal4::GobMachine::ApplyVisibleFlagAndRepeatTrigger(
                      Runtime::object, visible, runtime) ==
                      (triggers ? 0x99u : 0u) &&
                  runtime.words[Runtime::object + 0x184u] == visible &&
                  runtime.words[Runtime::object + 0x278u] == 0u &&
                  runtime.words[Runtime::object + 0x27cu] == 0u &&
                  runtime.calls[0] == "set visible" &&
                  runtime.calls[1] == "clear primary" &&
                  runtime.calls[2] == "clear context" &&
                  (runtime.calls.back() == "trigger") == triggers,
                  "visible repeat trigger gate and cleanup order");
        }
    }
}

}  // namespace

int main() {
    try {
        CheckStartAndReset();
        CheckReadyAndSlots();
        CheckMachineNode();
        CheckMainConfig();
        CheckVisibleRepeat();
        std::cout << "gob_machine_control_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
