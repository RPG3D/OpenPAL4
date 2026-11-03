#include "pal4/game_object_npc_behavior_config.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

constexpr std::uint32_t kOwner = 0x10000u;
constexpr std::uint32_t kConfig = 0xa000u;

struct Runtime final : pal4::GameObjectNpcBehaviorRuntimeStub {
    struct Value {
        std::uint32_t node;
        std::string name;
        std::int32_t integer;
        float real;
    };
    std::vector<Value> attributes;
    std::vector<std::string> calls;
    std::vector<std::string> writes;
    std::int32_t count{};
    std::uint32_t next_child{};
    std::uint32_t selected_count{};

    void AddInt(std::uint32_t node, const char* name, std::int32_t value) {
        attributes.push_back({node, name, value, 0.0f});
    }
    void AddFloat(std::uint32_t node, const char* name, float value) {
        attributes.push_back({node, name, 0, value});
    }
    std::uint32_t FindAttribute(std::uint32_t node,
                                const char* name) override {
        calls.push_back(std::string("find:") + name);
        for (std::uint32_t i = 0u; i < attributes.size(); ++i)
            if (attributes[i].node == node && attributes[i].name == name)
                return i + 1u;
        return 0u;
    }
    std::int32_t AttributeToInt(std::uint32_t attribute) override {
        calls.emplace_back("to-int");
        return attributes.at(attribute - 1u).integer;
    }
    float AttributeToFloat(std::uint32_t attribute) override {
        calls.emplace_back("to-float");
        return attributes.at(attribute - 1u).real;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        Check(address == kConfig + 0x38u, "child count offset");
        calls.emplace_back("read-count");
        return static_cast<std::uint32_t>(count);
    }
    std::uint32_t GetChildByIndex(std::uint32_t node,
                                  std::uint32_t& index) override {
        Check(node == kConfig && index == next_child,
              "child index order");
        calls.push_back("child:" + std::to_string(index));
        return 0xb000u + 0x100u * next_child++;
    }
    std::uint32_t AddEvent(std::uint32_t processor,
                           const char* name) override {
        Check(processor == kOwner + 0x658u &&
              std::string(name) == "NPC-Fuzzy-Behavior",
              "fuzzy event name and processor address");
        calls.emplace_back("add-event");
        return 0xc000u;
    }
    std::int32_t ResolveClassId(std::int32_t type) override {
        Check(type == static_cast<std::int32_t>(selected_count),
              "resolver receives only object type");
        calls.emplace_back("resolve");
        return pal4::GameObjectNpcBehaviorConfig::ClassIdFromObjectType(type);
    }
    std::uint32_t AddClassProportion(std::uint32_t event,
                                     std::int32_t class_id,
                                     std::int32_t proportion) override {
        Check(event == 0xc000u &&
              class_id == static_cast<std::int32_t>(selected_count) &&
              proportion == 10 + static_cast<std::int32_t>(selected_count),
              "class record receives ID and saved proportion");
        calls.emplace_back("add-record");
        return 0xd000u + 0x100u * selected_count++;
    }
    void WriteInt(std::uint32_t node, const char* name,
                  std::int32_t value) override {
        Check(node == 0xd000u + 0x100u * (selected_count - 1u) &&
              std::string(name) == "beh-vtm-waitMode",
              "wait mode target and key");
        writes.push_back("mode:" + std::to_string(value));
        calls.emplace_back("write-int");
    }
    void WriteFloat(std::uint32_t node, const char* name,
                    float value) override {
        Check(node == 0xd000u + 0x100u * (selected_count - 1u),
              "float target");
        writes.push_back(std::string(name) + ":" + std::to_string(value));
        calls.emplace_back("write-float");
    }
    void SetGroupMode(std::uint32_t event,
                      std::int32_t mode) override {
        Check(event == 0xc000u && mode == 2,
              "final event group mode");
        calls.emplace_back("set-group-mode");
    }
};

void AddValidChild(Runtime& runtime, std::uint32_t child,
                   std::int32_t type) {
    runtime.AddInt(child, "NPCINFO_Behavior_ObjectType", type);
    runtime.AddInt(child, "NPCINFO_Behavior_ObjectProportion", 10 + type);
    runtime.AddInt(child, "NPCINFO_Behavior_ObjectWaitMode", 1 + type);
}

}  // namespace

int main() {
    for (std::int32_t id = 0; id <= 4; ++id)
        Check(pal4::GameObjectNpcBehaviorConfig::ClassIdFromObjectType(id) == id,
              "five object types map to their constant class IDs");
    Check(pal4::GameObjectNpcBehaviorConfig::ClassIdFromObjectType(-1) == -1 &&
          pal4::GameObjectNpcBehaviorConfig::ClassIdFromObjectType(5) == -1,
          "out-of-range object types map to minus one");

    Runtime runtime;
    pal4::GameObjectNpcBehaviorConfig::Process(kOwner, 0u, runtime);
    Check(runtime.calls.empty(), "null config is inert");

    pal4::GameObjectNpcBehaviorConfig::Process(kOwner, kConfig, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "find:NPCINFO_Behavior_GroupMode"},
          "missing group mode defaults to disabled mode three");

    runtime.calls.clear();
    runtime.AddInt(kConfig, "NPCINFO_Behavior_GroupMode", 3);
    pal4::GameObjectNpcBehaviorConfig::Process(kOwner, kConfig, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "find:NPCINFO_Behavior_GroupMode", "to-int"},
          "explicit mode three also skips the event");

    runtime.attributes.clear();
    runtime.calls.clear();
    runtime.AddInt(kConfig, "NPCINFO_Behavior_GroupMode", 2);
    pal4::GameObjectNpcBehaviorConfig::Process(kOwner, kConfig, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "find:NPCINFO_Behavior_GroupMode", "to-int", "add-event",
              "read-count", "set-group-mode"},
          "zero-child group finalizes after event creation");

    runtime.calls.clear();
    runtime.count = 2;
    AddValidChild(runtime, 0xb000u, 0);
    AddValidChild(runtime, 0xb100u, 1);
    runtime.AddFloat(0xb000u, "NPCINFO_Behavior_ObjectWaitTime", 2.5f);
    runtime.AddFloat(0xb000u, "NPCINFO_Behavior_ObjectExpectTime", 3.5f);
    pal4::GameObjectNpcBehaviorConfig::Process(kOwner, kConfig, runtime);
    Check(runtime.next_child == 2u && runtime.selected_count == 2u &&
          runtime.calls.back() == "set-group-mode" &&
          runtime.writes == std::vector<std::string>{
              "mode:1", "beh-vtm-waitTime:2.500000",
              "beh-vtm-expectTime:3.500000", "mode:2",
              "beh-vtm-waitTime:-1.000000",
              "beh-vtm-expectTime:-1.000000"},
          "two children preserve integer, float and default values");
    const auto first_resolve = std::find(runtime.calls.begin(),
                                         runtime.calls.end(), "resolve");
    Check(first_resolve != runtime.calls.end() &&
          first_resolve + 1 != runtime.calls.end() &&
          *(first_resolve + 1) == "add-record",
          "class resolver precedes record insertion");

    Runtime invalid;
    invalid.count = 1;
    invalid.AddInt(kConfig, "NPCINFO_Behavior_GroupMode", 2);
    invalid.AddInt(0xb000u, "NPCINFO_Behavior_ObjectType", 0);
    invalid.AddInt(0xb000u, "NPCINFO_Behavior_ObjectProportion", 10);
    pal4::GameObjectNpcBehaviorConfig::Process(kOwner, kConfig, invalid);
    Check(invalid.selected_count == 0u &&
          std::find(invalid.calls.begin(), invalid.calls.end(),
                    "set-group-mode") == invalid.calls.end() &&
          invalid.calls.back() ==
              "find:NPCINFO_Behavior_ObjectExpectTime",
          "invalid wait mode exits after reading all five fields");
}
