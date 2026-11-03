#include "pal4/dialog_object.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {
void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
struct Node final {
    std::string name;
    std::vector<std::uint32_t> children;
    std::map<std::string, std::string> strings;
    std::map<std::string, int> ints;
    std::map<std::string, float> floats;
};
struct Runtime final : pal4::IDialogObjectConfigRuntime {
    std::map<std::uint32_t, Node> nodes;
    std::vector<std::string> calls;
    bool text_ok = true;
    std::string_view NodeName(std::uint32_t node) override {
        return nodes.at(node).name;
    }
    std::uint32_t FindChild(std::uint32_t parent,
                            std::string_view name) override {
        for (const auto child : nodes.at(parent).children)
            if (nodes.at(child).name == name) return child;
        return 0u;
    }
    std::uint32_t NextChild(std::uint32_t parent, std::uint32_t current,
                            std::string_view name) override {
        bool past_current = false;
        for (const auto child : nodes.at(parent).children) {
            if (past_current && nodes.at(child).name == name) return child;
            if (child == current) past_current = true;
        }
        return 0u;
    }
    std::uint32_t ChildCount(std::uint32_t parent) override {
        return static_cast<std::uint32_t>(nodes.at(parent).children.size());
    }
    std::uint32_t ChildAt(std::uint32_t parent,
                          std::uint32_t index) override {
        return nodes.at(parent).children.at(index);
    }
    std::optional<std::string_view> StringAttribute(
        std::uint32_t node, std::string_view name) override {
        const auto& values = nodes.at(node).strings;
        const auto found = values.find(std::string(name));
        if (found == values.end()) return std::nullopt;
        return found->second;
    }
    std::optional<int> IntAttribute(std::uint32_t node,
                                     std::string_view name) override {
        const auto& values = nodes.at(node).ints;
        const auto found = values.find(std::string(name));
        if (found == values.end()) return std::nullopt;
        return found->second;
    }
    std::optional<float> FloatAttribute(std::uint32_t node,
                                         std::string_view name) override {
        const auto& values = nodes.at(node).floats;
        const auto found = values.find(std::string(name));
        if (found == values.end()) return std::nullopt;
        return found->second;
    }
    void AppendObjectId(pal4::DialogObjectState&,
                        std::string_view id) override {
        calls.emplace_back("object:" + std::string(id));
    }
    void AppendTaskId(pal4::DialogObjectState&, int id) override {
        calls.emplace_back("task:" + std::to_string(id));
    }
    std::uint32_t CopyXmlNode(std::uint32_t node) override {
        calls.emplace_back("copy:" + std::to_string(node));
        return node + 0x1000u;
    }
    void AppendTaskParameter(pal4::DialogObjectState&,
                             std::uint32_t node) override {
        calls.emplace_back("param:" + std::to_string(node));
    }
    bool LoadText(std::uint32_t manager, std::uint32_t list) override {
        Check(manager == 0x2000u, "text manager");
        calls.emplace_back("text:" + std::to_string(list));
        return text_ok;
    }
    void InsertTarget(pal4::DialogObjectState&, std::string_view name,
                      const std::array<float, 3u>& position) override {
        Check((name == "origin" &&
               position == std::array<float, 3u>{1.0f, 0.0f, 3.0f}) ||
              (name.empty() &&
               position == std::array<float, 3u>{0.0f, 0.0f, 0.0f}),
              "target name and xyz defaults");
        calls.emplace_back("target:" + std::string(name));
    }
};
}  // namespace

int main() {
    pal4::DialogObjectState state{};
    state.words[4u] = 0x2000u;
    Runtime runtime;
    Check(!pal4::DialogObject::LoadConfig(state, 0u, runtime) &&
          runtime.calls.empty(), "null XML root rejected");
    runtime.nodes[1u].name = "Other";
    Check(!pal4::DialogObject::LoadConfig(state, 1u, runtime) &&
          runtime.calls.empty(), "wrong root rejected without side effects");

    runtime.nodes[1u] = {"eFfEcT", {2u, 5u, 8u, 10u, 11u}, {}, {}, {}};
    runtime.nodes[2u] = {"ObjectList", {3u, 4u}};
    runtime.nodes[3u].name = "Object";
    runtime.nodes[3u].strings["ObjectId"] = "hero";
    runtime.nodes[4u].name = "Object";
    runtime.nodes[5u] = {"TaskList", {6u, 7u}};
    runtime.nodes[6u].name = "TASK";
    runtime.nodes[6u].ints["TaskId"] = 42;
    runtime.nodes[7u].name = "TASK";
    runtime.nodes[8u] = {"TaskParamList", {9u}};
    runtime.nodes[9u].name = "Param";
    runtime.nodes[10u].name = "OxnList";
    runtime.nodes[11u] = {"TargetList", {12u, 13u}};
    runtime.nodes[12u].name = "Target";
    runtime.nodes[12u].strings["TargetName"] = "origin";
    runtime.nodes[12u].floats["TargetPosX"] = 1.0f;
    runtime.nodes[12u].floats["TargetPosZ"] = 3.0f;
    runtime.nodes[13u].name = "Target";
    runtime.text_ok = false;
    Check(!pal4::DialogObject::LoadConfig(state, 1u, runtime) &&
          runtime.calls == std::vector<std::string>{
              "object:hero", "object:", "task:42", "task:0", "copy:9",
              "param:4105", "text:10"},
          "text load failure follows lists and stops before targets");
    runtime.calls.clear();
    runtime.text_ok = true;
    state.words[20u] = 0x00ffffffu;
    Check(pal4::DialogObject::LoadConfig(state, 1u, runtime) &&
          runtime.calls == std::vector<std::string>{
              "object:hero", "object:", "task:42", "task:0", "copy:9",
              "param:4105", "text:10", "target:origin", "target:"} &&
          state.words[20u] == 0x01ffffffu &&
          state.words[21u] == std::bit_cast<std::uint32_t>(1.0f),
          "successful load preserves native order and defaults");

    Runtime minimal;
    minimal.nodes[1u].name = "Effect";
    minimal.nodes[1u].ints["DistanceClip"] = 0;
    minimal.nodes[1u].floats["CameraStart"] = -2.0f;
    state.words[20u] = 0xffffffffu;
    Check(pal4::DialogObject::LoadConfig(state, 1u, minimal) &&
          minimal.calls == std::vector<std::string>{"text:0"} &&
          state.words[20u] == 0x00ffffffu && state.words[21u] == 0u,
          "optional lists, disabled clip and negative camera clamp");
}
