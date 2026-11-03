#include "pal4/monster_fuzzy_behavior_loader.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IMonsterFuzzyBehaviorRuntime {
    std::vector<std::string> calls;
    std::unordered_map<std::string, std::uint32_t> attributes;
    std::unordered_map<std::uint32_t, std::int32_t> integers;
    std::unordered_map<std::uint32_t, float> floats;
    std::int32_t child_count{};

    static std::string Key(std::uint32_t node, const char* name) {
        return std::to_string(node) + ":" + name;
    }
    void SetInt(std::uint32_t node, const char* name, std::uint32_t id,
                std::int32_t value) {
        attributes[Key(node, name)] = id;
        integers[id] = value;
    }
    void SetFloat(std::uint32_t node, const char* name, std::uint32_t id,
                  float value) {
        attributes[Key(node, name)] = id;
        floats[id] = value;
    }
    std::uint32_t Attribute(std::uint32_t node,
                            const char* name) override {
        calls.push_back("attribute:" + Key(node, name));
        const auto found = attributes.find(Key(node, name));
        return found == attributes.end() ? 0u : found->second;
    }
    std::int32_t ParseInt(std::uint32_t attribute,
                          std::int32_t fallback) override {
        calls.push_back("parse-int:" + std::to_string(attribute) + ":" +
                        std::to_string(fallback));
        return integers.at(attribute);
    }
    float ParseFloat(std::uint32_t attribute, float fallback) override {
        calls.push_back("parse-float:" + std::to_string(attribute) + ":" +
                        std::to_string(fallback));
        return floats.at(attribute);
    }
    std::int32_t ChildCount(std::uint32_t node) override {
        Check(node == 0x7000u, "config child count address");
        calls.push_back("child-count");
        return child_count;
    }
    std::uint32_t ChildByIndex(std::uint32_t node,
                               std::uint32_t index) override {
        Check(node == 0x7000u, "config child lookup address");
        calls.push_back("child:" + std::to_string(index));
        return 0x7100u + index;
    }
    std::uint32_t CreateEvent(std::uint32_t manager,
                              const char* name) override {
        Check(manager == 0x5000u &&
                  std::string(name) == "MST-Fuzzy-Behavior",
              "fuzzy event name and manager");
        calls.push_back("create-event");
        return 0x8000u;
    }
    std::uint32_t AddClassProportion(std::uint32_t event,
                                     std::int32_t class_id,
                                     std::int32_t proportion) override {
        Check(event == 0x8000u, "atomic event address");
        calls.push_back("atomic:" + std::to_string(class_id) + ":" +
                        std::to_string(proportion));
        return 0x9000u;
    }
    void SetIntAttribute(std::uint32_t record, const char* name,
                         std::int32_t value) override {
        Check(record == 0x9000u, "wait-mode record address");
        calls.push_back(std::string(name) + ":" + std::to_string(value));
    }
    void SetFloatAttribute(std::uint32_t record, const char* name,
                           float value) override {
        Check(record == 0x9000u, "wait-time record address");
        calls.push_back(std::string(name) + ":" + std::to_string(value));
    }
    void SetGroupMode(std::uint32_t event, std::int32_t mode) override {
        Check(event == 0x8000u, "group mode event address");
        calls.push_back("group-mode:" + std::to_string(mode));
    }
};

using Loader = pal4::MonsterFuzzyBehaviorLoader;

void TestNullAndDefaultGroupMode() {
    Runtime runtime;
    Loader::Load(0x5000u, 0u, runtime);
    Check(runtime.calls.empty(), "null config returns without reads");
    Loader::Load(0x5000u, 0x7000u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "attribute:28672:MSTINFO_Behavior_GroupMode"},
          "missing GroupMode defaults to three and skips event");
    runtime.SetInt(0x7000u, "MSTINFO_Behavior_GroupMode", 1u, 3);
    runtime.calls.clear();
    Loader::Load(0x5000u, 0x7000u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "attribute:28672:MSTINFO_Behavior_GroupMode",
              "parse-int:1:3"},
          "explicit GroupMode three also skips event");
}

void TestEmptyNonDefaultGroup() {
    Runtime runtime;
    runtime.SetInt(0x7000u, "MSTINFO_Behavior_GroupMode", 1u, 2);
    Loader::Load(0x5000u, 0x7000u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "attribute:28672:MSTINFO_Behavior_GroupMode",
              "parse-int:1:3", "create-event", "child-count",
              "group-mode:2"},
          "empty non-default group creates event and writes mode");
}

void TestValidChildAndDefaults() {
    Runtime runtime;
    runtime.SetInt(0x7000u, "MSTINFO_Behavior_GroupMode", 1u, 1);
    runtime.child_count = 1;
    runtime.SetInt(0x7100u, "MSTINFO_Behavior_ObjectType", 2u, 2);
    runtime.SetInt(0x7100u, "MSTINFO_Behavior_ObjectProportion", 3u, 40);
    runtime.SetInt(0x7100u, "MSTINFO_Behavior_ObjectWaitMode", 4u, 5);
    Loader::Load(0x5000u, 0x7000u, runtime);
    const auto& calls = runtime.calls;
    Check(calls.size() == 19u &&
              calls[3] == "child-count" &&
              calls[4] == "child:0" &&
              calls[6] == "parse-int:2:-1" &&
              calls[8] == "parse-int:3:-1" &&
              calls[10] == "parse-int:4:0" &&
              calls[11] ==
                  "attribute:28928:MSTINFO_Behavior_ObjectWaitTime" &&
              calls[12] ==
                  "attribute:28928:MSTINFO_Behavior_ObjectExpectTime" &&
              calls[13] == "atomic:2:40" &&
              calls[14] == "beh-vtm-waitMode:5" &&
              calls[15] == "beh-vtm-waitTime:-1.000000" &&
              calls[16] == "beh-vtm-expectTime:-1.000000" &&
              calls[17] == "child-count" &&
              calls[18] == "group-mode:1",
          "valid child reads five attributes before writing atomic record");
}

void TestInvalidChildStopsWithoutGroupWrite() {
    for (int missing : {0, 1, 2}) {
        Runtime runtime;
        runtime.SetInt(0x7000u, "MSTINFO_Behavior_GroupMode", 1u, 2);
        runtime.child_count = 2;
        if (missing != 0)
            runtime.SetInt(0x7100u, "MSTINFO_Behavior_ObjectType", 2u, 1);
        if (missing != 1)
            runtime.SetInt(0x7100u, "MSTINFO_Behavior_ObjectProportion",
                           3u, 20);
        if (missing != 2)
            runtime.SetInt(0x7100u, "MSTINFO_Behavior_ObjectWaitMode", 4u, 1);
        Loader::Load(0x5000u, 0x7000u, runtime);
        Check(runtime.calls.back() ==
                  "attribute:28928:MSTINFO_Behavior_ObjectExpectTime" &&
                  std::find(runtime.calls.begin(), runtime.calls.end(),
                            "child:1") == runtime.calls.end() &&
                  std::find(runtime.calls.begin(), runtime.calls.end(),
                            "group-mode:2") == runtime.calls.end(),
              "invalid first child exits after all five reads");
    }
}

void TestOutOfRangeTypeMapsToMinusOne() {
    Check(Loader::ResolveObjectType(-1) == -1 &&
              Loader::ResolveObjectType(0) == 0 &&
              Loader::ResolveObjectType(1) == 1 &&
              Loader::ResolveObjectType(2) == 2 &&
              Loader::ResolveObjectType(3) == 3 &&
              Loader::ResolveObjectType(4) == -1,
          "ObjectType switch returns classID zero through three only");
    Runtime runtime;
    runtime.SetInt(0x7000u, "MSTINFO_Behavior_GroupMode", 1u, 0);
    runtime.child_count = 1;
    runtime.SetInt(0x7100u, "MSTINFO_Behavior_ObjectType", 2u, 4);
    runtime.SetInt(0x7100u, "MSTINFO_Behavior_ObjectProportion", 3u, 1);
    runtime.SetInt(0x7100u, "MSTINFO_Behavior_ObjectWaitMode", 4u, 1);
    runtime.SetFloat(0x7100u, "MSTINFO_Behavior_ObjectWaitTime", 5u, 2.5f);
    runtime.SetFloat(0x7100u, "MSTINFO_Behavior_ObjectExpectTime", 6u, 4.5f);
    Loader::Load(0x5000u, 0x7000u, runtime);
    Check(std::find(runtime.calls.begin(), runtime.calls.end(),
                    "atomic:-1:1") != runtime.calls.end() &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                    "parse-float:5:-1.000000") != runtime.calls.end() &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                    "parse-float:6:-1.000000") != runtime.calls.end() &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                    "beh-vtm-waitTime:2.500000") != runtime.calls.end() &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                    "beh-vtm-expectTime:4.500000") != runtime.calls.end(),
          "positive out-of-range type still builds record with classID -1");
}

}  // namespace

int main() {
    TestNullAndDefaultGroupMode();
    TestEmptyNonDefaultGroup();
    TestValidChildAndDefaults();
    TestInvalidChildStopsWithoutGroupWrite();
    TestOutOfRangeTypeMapsToMinusOne();
    pal4::MonsterFuzzyBehaviorRuntimeStub stub;
    Loader::Load(0x5000u, 0x7000u, stub);
    std::cout << "6 monster fuzzy behavior cases passed\n";
}
