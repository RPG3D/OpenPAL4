#include "pal4/gob_machine.h"
#include "pal4/gob_machine_task_runtime.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

using Definition = pal4::GobMachineTaskDefinition;

struct Runtime final : pal4::IGobMachineTaskRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, float> floats;
    std::vector<std::string> calls;
    std::vector<std::string> read_keys;
    std::map<std::string, std::int32_t> int_values;
    std::map<std::string, float> float_values;
    std::map<std::string, std::string> string_values;
    Definition initialized;
    std::uint32_t document = 0x7000u;
    std::uint32_t tasks = 0x8000u;
    std::uint32_t child = 0x9000u;
    std::uint32_t task = 0xa000u;
    std::int32_t type = 0;
    std::int32_t count = 1;
    bool registered = false;
    bool initialized_task = false;
    bool appended = false;
    bool factory_called = false;
    bool saw_special_read = false;
    bool factory_before_special_read = false;

    void NoteSpecialRead(std::string_view key) {
        if (key == "PAL4_GOMTask_type" ||
            key == "PAL4_GOMTask_wait_mode" ||
            key == "PAL4_GOMTask_wait_time" ||
            key == "PAL4_GOMTask_expt_time" || saw_special_read) return;
        saw_special_read = true;
        factory_before_special_read = factory_called;
    }

    Runtime() {
        for (std::uint32_t axis = 0u; axis < 3u; ++axis) {
            words[object + 0xacu + axis * 4u] = 100u + axis;
            words[object + 0xa0u + axis * 4u] = 200u + axis;
        }
        words[object + 0x2a4u] = 0xb000u;
        words[object + 0x2a8u] = 0xb008u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write word");
    }
    void WriteFloat(std::uint32_t address, float value) override {
        floats[address] = value;
        calls.emplace_back("write float");
    }
    void ClearPrimaryTasks(std::uint32_t container) override {
        Check(container == object + 0x258u, "primary task container");
        calls.emplace_back("clear tasks");
    }
    void EraseTaskPointerRange(std::uint32_t record,
                              std::uint32_t begin,
                              std::uint32_t end) override {
        Check(record == object + 0x2a0u && begin == 0xb000u &&
              end == 0xb008u, "task pointer range");
        calls.emplace_back("copy pointers");
    }
    std::uint32_t GetMachineConfigNode(std::uint32_t address) override {
        Check(address == object, "machine config object");
        calls.emplace_back("get config");
        return document;
    }
    std::uint32_t FindXmlChild(std::uint32_t parent,
                                std::string_view name) override {
        Check(parent == document &&
              name == "PAL4_GameObject-machine-tasks",
              "task XML path");
        calls.emplace_back("find tasks");
        return tasks;
    }
    std::int32_t XmlChildCount(std::uint32_t parent) override {
        Check(parent == tasks, "task child count parent");
        calls.emplace_back("child count");
        return count;
    }
    std::uint32_t XmlChildAt(std::uint32_t parent,
                             std::int32_t index) override {
        Check(parent == tasks && index >= 0 && index < count,
              "task child index");
        calls.emplace_back("child at");
        return child;
    }
    std::int32_t ReadXmlInt(std::uint32_t node,
                            std::string_view key,
                            std::int32_t fallback) override {
        Check(node == child, "task integer node");
        read_keys.emplace_back(key);
        NoteSpecialRead(key);
        if (key == "PAL4_GOMTask_type") {
            Check(fallback == -1, "task type fallback");
            return type;
        }
        const auto found = int_values.find(std::string(key));
        if (found != int_values.end()) return found->second;
        return fallback;
    }
    float ReadXmlFloat(std::uint32_t node,
                       std::string_view key,
                       float fallback) override {
        Check(node == child, "task float node");
        read_keys.emplace_back(key);
        NoteSpecialRead(key);
        const auto found = float_values.find(std::string(key));
        if (found != float_values.end()) return found->second;
        return fallback;
    }
    std::string ReadXmlString(std::uint32_t node,
                              std::string_view key,
                              std::string_view fallback) override {
        Check(node == child && fallback.empty(),
              "task string node and fallback");
        read_keys.emplace_back(key);
        NoteSpecialRead(key);
        const auto found = string_values.find(std::string(key));
        if (found != string_values.end()) return found->second;
        return {};
    }
    std::uint32_t CreateTask(std::int32_t kind) override {
        Check(kind == type, "factory task type");
        factory_called = true;
        calls.emplace_back("create task");
        return task;
    }
    void RegisterTask(std::uint32_t container,
                      std::uint32_t address,
                      const Definition& definition) override {
        Check(container == object + 0x258u && address == task &&
              definition.type == type, "registered task");
        registered = true;
        calls.emplace_back("register task");
    }
    void InitializeTask(std::uint32_t address,
                        std::uint32_t owner,
                        std::uint32_t context,
                        const Definition& definition) override {
        Check(address == task && owner == object &&
              context == object + 0x268u,
              "task initialization addresses");
        initialized = definition;
        initialized_task = true;
        calls.emplace_back("initialize task");
    }
    void AppendTaskPointer(std::uint32_t record,
                           std::uint32_t address) override {
        Check(record == object + 0x2a0u && address == task,
              "task pointer append");
        appended = true;
        calls.emplace_back("append task");
    }
    float RandomWaitDuration(float minimum,
                             float maximum) override {
        Check(minimum == 0.0f && maximum == 1.0f,
              "time-wait default interval");
        calls.emplace_back("random wait");
        return 0.5f;
    }
};

struct Case {
    std::int32_t type;
    std::string_view keys;
};

constexpr Case cases[] = {
    {0, "move_speed move_target_X move_target_Y move_target_Z move_sound_name move_sound_time move_effect_name move_effect_time"},
    {1, "rotate_speed rotate_target_X rotate_target_Y rotate_target_Z rotate_sound_name rotate_sound_time rotate_effect_name rotate_effect_time"},
    {2, "script_wait script_func"},
    {3, "action_time action_name"},
    {6, ""},
    {7, "getitem_id getitem_num"},
    {8, "getitem_id getitem_num"},
    {9, "getitem_money"},
    {10, "movment_tPosX movment_tPosY movment_tPosZ movment_tRotX movment_tRotY movment_tRotZ movment_sound_name movment_sound_time movment_effect_name movment_effect_time"},
    {11, "camera_autoseek camera_filename"},
    {12, ""}, {13, ""}, {14, ""},
    {15, "active_object_ctrl active_object_name"},
    {16, "start_posX start_posY start_posZ start_rotX start_rotY start_rotZ"},
    {17, "touch_other_wait touch_other_name"},
    {18, "even_rotate_sound_name even_rotate_sound_time even_rotate_effect_name even_rotate_effect_time even_rotate_speedY"},
    {19, "time_wait_minval time_wait_maxval"},
    {20, "camera_quake_frequency camera_quake_amplitude camera_quake_time"},
    {21, ""},
    {22, "moveAndSelfRot_target_X moveAndSelfRot_target_Y moveAndSelfRot_target_Z moveAndSelfRot_speed_X moveAndSelfRot_speed_Y moveAndSelfRot_speed_Z moveAndSelfRot_sound_name moveAndSelfRot_sound_time moveAndSelfRot_effect_name moveAndSelfRot_effect_time moveAndSelfRot_direct moveAndSelfRot_directDist"},
};

std::vector<std::string> ExpectedKeys(const Case& item) {
    std::vector<std::string> result{
        "PAL4_GOMTask_type", "PAL4_GOMTask_wait_mode",
        "PAL4_GOMTask_wait_time", "PAL4_GOMTask_expt_time"};
    std::string remaining(item.keys);
    while (!remaining.empty()) {
        const auto separator = remaining.find(' ');
        const auto part = remaining.substr(0, separator);
        if (item.type == 22 && part != "moveAndSelfRot_sound_name")
            result.emplace_back("CSTR_GOMTask_" + part);
        else
            result.emplace_back("PAL4_GOMTask_" + part);
        if (separator == std::string::npos) break;
        remaining.erase(0, separator + 1);
    }
    return result;
}

void CheckTaskTypes() {
    for (const auto& item : cases) {
        Runtime runtime;
        runtime.type = item.type;
        Check(pal4::GobMachine::BuildAndStartTaskRun(
                  Runtime::object, runtime) == 1u &&
              runtime.read_keys == ExpectedKeys(item) &&
              runtime.registered && runtime.initialized_task &&
              runtime.appended &&
              runtime.initialized.type == item.type &&
              runtime.initialized.wait_mode == 0 &&
              runtime.initialized.wait_time == 0.0f &&
              runtime.initialized.expect_time == 0.0f,
              "task type schema and registration");
        Check(runtime.words.at(Runtime::object + 0x288u) == 100u &&
              runtime.words.at(Runtime::object + 0x294u) == 200u,
              "initial transform snapshot");
        if (item.type == 16) {
            Check(runtime.floats.size() == 6u &&
                  runtime.floats.at(Runtime::object + 0x288u) == 0.0f &&
                  runtime.floats.at(Runtime::object + 0x29cu) == 0.0f,
                  "start task replaces saved transform");
        }
        if (item.type == 19) {
            Check(runtime.initialized.parameters.size() == 3u &&
                  runtime.initialized.parameters.back().key ==
                      "generated_wait_duration" &&
                  std::get<float>(runtime.initialized.parameters.back().value)
                      == 0.5f,
                  "time-wait generated duration");
        }
        if (item.type == 3)
            Check(std::get<std::int32_t>(
                      runtime.initialized.parameters.front().value) == 1,
                  "action time defaults to one");
        if (item.type == 18)
            Check(std::get<float>(
                      runtime.initialized.parameters.back().value) == 10.0f,
                  "even-rotate speed defaults to ten");
        if (item.type == 20)
            for (const auto& parameter : runtime.initialized.parameters)
                Check(std::get<float>(parameter.value) == 1.0f,
                      "camera quake defaults to one");
    }
}

const pal4::GobMachineTaskParameter& Parameter(
    const Definition& definition, std::string_view key) {
    const auto found = std::find_if(
        definition.parameters.begin(), definition.parameters.end(),
        [key](const auto& value) { return value.key == key; });
    if (found == definition.parameters.end())
        throw std::runtime_error("missing task parameter");
    return *found;
}

void CheckAttributeValuesAndOrder() {
    Runtime movement;
    movement.type = 22;
    movement.int_values["PAL4_GOMTask_wait_mode"] = 2;
    movement.float_values["PAL4_GOMTask_wait_time"] = 0.25f;
    movement.float_values["PAL4_GOMTask_expt_time"] = 1.5f;
    movement.float_values["CSTR_GOMTask_moveAndSelfRot_target_X"] = 3.5f;
    movement.float_values["CSTR_GOMTask_moveAndSelfRot_directDist"] = 8.0f;
    movement.int_values["CSTR_GOMTask_moveAndSelfRot_direct"] = 1;
    movement.string_values["PAL4_GOMTask_moveAndSelfRot_sound_name"] =
        "bell.wav";
    Check(pal4::GobMachine::BuildAndStartTaskRun(
              Runtime::object, movement) == 1u &&
          movement.initialized.wait_mode == 2 &&
          movement.initialized.wait_time == 0.25f &&
          movement.initialized.expect_time == 1.5f &&
          std::get<float>(Parameter(movement.initialized,
              "CSTR_GOMTask_moveAndSelfRot_target_X").value) == 3.5f &&
          std::get<float>(Parameter(movement.initialized,
              "CSTR_GOMTask_moveAndSelfRot_directDist").value) == 8.0f &&
          std::get<std::int32_t>(Parameter(movement.initialized,
              "CSTR_GOMTask_moveAndSelfRot_direct").value) == 1 &&
          std::get<std::string>(Parameter(movement.initialized,
              "PAL4_GOMTask_moveAndSelfRot_sound_name").value) ==
              "bell.wav" && !movement.factory_before_special_read,
          "task attributes retain types and read before movement factory");
    const auto first = std::find(movement.calls.begin(),
        movement.calls.end(), "create task");
    Check(first != movement.calls.end() &&
          movement.calls.back() == "append task",
          "movement task initialization and append order");

    Runtime script;
    script.type = 2;
    script.string_values["PAL4_GOMTask_script_func"] = "OnStart";
    Check(pal4::GobMachine::BuildAndStartTaskRun(
              Runtime::object, script) == 1u &&
          std::get<std::string>(Parameter(script.initialized,
              "PAL4_GOMTask_script_func").value) == "OnStart" &&
          script.factory_before_special_read &&
          script.calls[script.calls.size() - 3u] == "register task" &&
          script.calls[script.calls.size() - 2u] == "initialize task" &&
          script.calls.back() == "append task",
          "script factory registers before initialization");

    Runtime start;
    start.type = 16;
    start.float_values["PAL4_GOMTask_start_posX"] = 7.0f;
    start.float_values["PAL4_GOMTask_start_rotZ"] = -2.0f;
    Check(pal4::GobMachine::BuildAndStartTaskRun(
              Runtime::object, start) == 1u &&
          start.floats.at(Runtime::object + 0x288u) == 7.0f &&
          start.floats.at(Runtime::object + 0x29cu) == -2.0f,
          "start task overrides both transform endpoints");
}

void CheckGates() {
    Runtime no_document;
    no_document.document = 0u;
    Check(pal4::GobMachine::BuildAndStartTaskRun(
              Runtime::object, no_document) == 0u &&
          no_document.calls.back() == "get config",
          "missing machine config");
    Runtime no_tasks;
    no_tasks.tasks = 0u;
    Check(pal4::GobMachine::BuildAndStartTaskRun(
              Runtime::object, no_tasks) == 0u &&
          no_tasks.calls.back() == "find tasks",
          "missing task list");
    Runtime empty;
    empty.count = 0;
    Check(pal4::GobMachine::BuildAndStartTaskRun(
              Runtime::object, empty) == 1u &&
          !empty.initialized_task, "empty task list succeeds");
    Runtime missing_child;
    missing_child.child = 0u;
    Check(pal4::GobMachine::BuildAndStartTaskRun(
              Runtime::object, missing_child) == 0u &&
          !missing_child.initialized_task,
          "missing child aborts task build");
    Runtime unsupported;
    unsupported.type = 4;
    Check(pal4::GobMachine::BuildAndStartTaskRun(
              Runtime::object, unsupported) == 1u &&
          !unsupported.registered && !unsupported.initialized_task &&
          !unsupported.appended &&
          unsupported.read_keys.size() == 4u,
          "unsupported task kind skips factory");
    Runtime failed_factory;
    failed_factory.task = 0u;
    failed_factory.type = 6;
    Check(pal4::GobMachine::BuildAndStartTaskRun(
              Runtime::object, failed_factory) == 1u &&
          !failed_factory.registered,
          "null factory skips container registration");
}

}  // namespace

int main() {
    try {
        CheckTaskTypes();
        CheckAttributeValuesAndOrder();
        CheckGates();
        std::cout << "gob_machine_task_builder_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
