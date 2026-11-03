#include "pal4/effect_system_text_manager.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IEffectSystemTextManagerXmlRuntime {
    static constexpr std::uint32_t kManager = 0x2000u;
    static constexpr std::uint32_t kRoot = 0x6000u;
    std::string root_name = "OxnList";
    std::vector<std::string> labels{"First", "Second", "Third"};
    std::vector<std::string> calls;
    std::int32_t child_count = 3;
    std::int32_t failed_index = -1;

    void ClearElements(std::uint32_t manager) override {
        Check(manager == kManager, "clear receives manager");
        calls.emplace_back("clear");
    }
    std::string_view NodeName(std::uint32_t node) override {
        Check(node == kRoot, "root-name lookup receives XML root");
        calls.emplace_back("root_name");
        return root_name;
    }
    std::int32_t ChildCount(std::uint32_t node) override {
        Check(node == kRoot, "child count receives XML root");
        calls.emplace_back("child_count");
        return child_count;
    }
    std::uint32_t ChildAt(std::uint32_t node,
                           std::uint32_t index) override {
        Check(node == kRoot && index < labels.size(),
              "child access is ordered and in range");
        calls.push_back("child:" + std::to_string(index));
        return 0x6100u + index;
    }
    std::string ReadAttribute(std::uint32_t node,
                              std::string_view name) override {
        const auto index = node - 0x6100u;
        Check(index < labels.size() && name == "Attr_Label",
              "label reader uses child and verified attribute name");
        calls.push_back("label:" + std::to_string(index));
        return labels[index];
    }
    bool AddEntry(std::uint32_t manager, std::string_view name,
                  std::uint32_t node) override {
        const auto index = node - 0x6100u;
        Check(manager == kManager && index < labels.size() &&
                  name == labels[index],
              "entry receives child and its label");
        calls.push_back("add:" + std::to_string(index));
        return static_cast<std::int32_t>(index) != failed_index;
    }
};

void TestValidationOrder() {
    Runtime missing;
    Check(!pal4::EffectSystemTextManager::LoadFromXml(
              Runtime::kManager, 0u, missing) &&
              missing.calls == std::vector<std::string>{"clear"},
          "null XML still clears old entries first");

    Runtime wrong_root;
    wrong_root.root_name = "Other";
    Check(!pal4::EffectSystemTextManager::LoadFromXml(
              Runtime::kManager, Runtime::kRoot, wrong_root) &&
              wrong_root.calls == std::vector<std::string>{
                  "clear", "root_name"},
          "wrong root name stops before child traversal");
}

void TestEmptyAndOrderedChildren() {
    Runtime empty;
    empty.child_count = 0;
    Check(pal4::EffectSystemTextManager::LoadFromXml(
              Runtime::kManager, Runtime::kRoot, empty) &&
              empty.calls == std::vector<std::string>{
                  "clear", "root_name", "child_count"},
          "valid empty OxnList succeeds");

    Runtime mixed_case;
    mixed_case.root_name = "oXnLiSt";
    mixed_case.labels[1] = "";
    Check(pal4::EffectSystemTextManager::LoadFromXml(
              Runtime::kManager, Runtime::kRoot, mixed_case) &&
              mixed_case.calls == std::vector<std::string>{
                  "clear", "root_name", "child_count",
                  "child:0", "label:0", "add:0",
                  "child:1", "label:1", "add:1",
                  "child:2", "label:2", "add:2"},
          "case-insensitive root loads every child in order, including empty label");
}

void TestStopOnRejectedEntry() {
    Runtime runtime;
    runtime.failed_index = 1;
    Check(!pal4::EffectSystemTextManager::LoadFromXml(
              Runtime::kManager, Runtime::kRoot, runtime) &&
              runtime.calls == std::vector<std::string>{
                  "clear", "root_name", "child_count",
                  "child:0", "label:0", "add:0",
                  "child:1", "label:1", "add:1"},
          "rejected entry stops before later children");
}

}  // namespace

int main() {
    TestValidationOrder();
    TestEmptyAndOrderedChildren();
    TestStopOnRejectedEntry();
}
