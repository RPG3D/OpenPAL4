#include "pal4/effect_system_text_manager.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IEffectSystemTextManagerEntryRuntime {
    static constexpr std::uint32_t kManager = 0x2000u;
    static constexpr std::uint32_t kHead = 0x3000u;
    static constexpr std::uint32_t kNode = 0x4000u;
    static constexpr std::uint32_t kClone = 0x7000u;
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {kManager + 4u, kHead}, {kNode + 28u, kClone}};
    std::vector<std::string> calls;
    bool existing = false;
    bool clone_succeeds = true;
    bool insert_succeeds = true;
    bool insert_throws = false;
    std::optional<std::uint32_t> attribute;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    std::uint32_t LowerBound(std::uint32_t manager,
                             std::string_view name) override {
        Check(manager == kManager && name == "Effect_A",
              "lookup receives manager and name");
        calls.emplace_back("lookup");
        return existing ? kNode : kHead;
    }
    int CompareName(std::string_view name,
                    std::uint32_t record) override {
        Check(name == "Effect_A" && record == kNode + 12u,
              "duplicate comparison uses node text record");
        calls.emplace_back("compare");
        return 0;
    }
    std::uint32_t CloneXmlNode(std::uint32_t source) override {
        Check(source == 0x6000u, "XML clone receives source node");
        calls.emplace_back("clone");
        return clone_succeeds ? kClone : 0u;
    }
    void DestroyXmlNode(std::uint32_t clone) override {
        Check(clone == kClone, "only rejected clone is destroyed");
        calls.emplace_back("destroy");
    }
    pal4::EffectSystemTextInsertResult Insert(
        std::uint32_t manager, std::string_view name,
        std::uint32_t clone) override {
        Check(manager == kManager && name == "Effect_A" &&
                  clone == kClone,
              "tree insertion receives cloned XML and name");
        calls.emplace_back("insert");
        if (insert_throws) throw std::runtime_error("insertion");
        return {insert_succeeds ? kNode : kHead, insert_succeeds};
    }
    std::optional<std::uint32_t> FindAttribute(
        std::uint32_t xml_node,
        std::string_view attribute_name) override {
        Check(xml_node == kClone && attribute_name == "Attr_Label",
              "label lookup uses inserted XML clone");
        calls.emplace_back("find_label");
        return attribute;
    }
    void AddAttribute(std::uint32_t xml_node,
                      std::string_view attribute_name,
                      std::string_view value) override {
        Check(xml_node == kClone && attribute_name == "Attr_Label" &&
                  value == "Effect_A",
              "missing label is added to inserted clone");
        calls.emplace_back("add_label");
    }
    void SetAttribute(std::uint32_t address,
                      std::string_view value) override {
        Check(address == 0x8000u && value == "Effect_A",
              "existing label is updated with entry name");
        calls.emplace_back("set_label");
    }
};

void TestEarlyDuplicateAndCloneFailure() {
    Runtime duplicate;
    duplicate.existing = true;
    Check(!pal4::EffectSystemTextManager::AddEntry(
              Runtime::kManager, "Effect_A", 0x6000u, duplicate) &&
              duplicate.calls == std::vector<std::string>{
                  "lookup", "compare"},
          "duplicate exits before XML copy");

    Runtime failed_clone;
    failed_clone.clone_succeeds = false;
    Check(!pal4::EffectSystemTextManager::AddEntry(
              Runtime::kManager, "Effect_A", 0x6000u, failed_clone) &&
              failed_clone.calls == std::vector<std::string>{
                  "lookup", "clone"},
          "failed clone does not reach tree insertion");
}

void TestInsertFailureAndException() {
    Runtime rejected;
    rejected.insert_succeeds = false;
    Check(!pal4::EffectSystemTextManager::AddEntry(
              Runtime::kManager, "Effect_A", 0x6000u, rejected) &&
              rejected.calls == std::vector<std::string>{
                  "lookup", "clone", "insert", "destroy"},
          "rejected insertion releases its unowned XML clone");

    Runtime throwing;
    throwing.insert_throws = true;
    bool caught = false;
    try {
        (void)pal4::EffectSystemTextManager::AddEntry(
            Runtime::kManager, "Effect_A", 0x6000u, throwing);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && throwing.calls == std::vector<std::string>{
              "lookup", "clone", "insert", "destroy"},
          "insertion exception releases clone and propagates");
}

void TestLabelBranches() {
    Runtime missing;
    Check(pal4::EffectSystemTextManager::AddEntry(
              Runtime::kManager, "Effect_A", 0x6000u, missing) &&
              missing.calls == std::vector<std::string>{
                  "lookup", "clone", "insert", "find_label", "add_label"},
          "new entry adds missing Attr_Label");

    Runtime existing;
    existing.attribute = 0x8000u;
    Check(pal4::EffectSystemTextManager::AddEntry(
              Runtime::kManager, "Effect_A", 0x6000u, existing) &&
              existing.calls == std::vector<std::string>{
                  "lookup", "clone", "insert", "find_label", "set_label"},
          "new entry updates existing Attr_Label");
}

}  // namespace

int main() {
    TestEarlyDuplicateAndCloneFailure();
    TestInsertFailureAndException();
    TestLabelBranches();
}
