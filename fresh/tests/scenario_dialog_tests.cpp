#include "pal4/scenario_dialog.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScenarioDialogRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> events;
    std::vector<std::string> names;
    std::vector<std::uint32_t> disabled;
    std::string fail_at;
    unsigned find_count = 0u;

    Runtime() { words[0x1024u] = 0x3000u; }

    void MaybeFail(const std::string& step) {
        if (fail_at == step) throw std::runtime_error(step);
    }
    void LoadLayout(std::uint32_t object, const char* layout) override {
        Check(object == 0x1000u &&
                  std::string(layout) == "ScenarioDialog", "layout name");
        events.emplace_back("layout");
        MaybeFail("layout");
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        events.emplace_back("word");
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        events.emplace_back("byte");
    }
    std::uint8_t IndeterminateTreeByte() override {
        events.emplace_back("tree-byte");
        return 0xa7u;
    }
    void InitializeTree(std::uint32_t tree) override {
        Check(tree == 0x1040u, "tree address");
        events.emplace_back("tree");
        MaybeFail("tree");
    }
    std::string ReadLayoutPrefix(std::uint32_t object) override {
        Check(object == 0x1000u, "prefix owner");
        events.emplace_back("prefix");
        MaybeFail("prefix");
        return "Scene/";
    }
    std::uint32_t FindWindow(
        std::uint32_t object, const std::string& name) override {
        Check(object == 0x1000u, "window owner");
        names.push_back(name);
        events.emplace_back("find" + std::to_string(++find_count));
        MaybeFail("find" + std::to_string(find_count));
        return 0x2000u + find_count * 0x10u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        events.emplace_back("read");
        return words.at(address);
    }
    void SetEnabled(std::uint32_t window, bool enabled) override {
        Check(!enabled, "window must be disabled");
        disabled.push_back(window);
        events.emplace_back("disable");
        MaybeFail("disable");
    }
    void SetAlwaysOnTop(std::uint32_t window, bool enabled) override {
        Check(window == 0x3000u && enabled, "root topmost");
        events.emplace_back("topmost");
        MaybeFail("topmost");
    }
    void RegisterEditboxEvent(std::uint32_t object) override {
        Check(object == 0x1000u, "editbox owner");
        events.emplace_back("editbox");
        MaybeFail("editbox");
    }
    void SetTextAndImagePositions(
        std::uint32_t object, std::uint32_t option) override {
        Check(object == 0x1000u && option == 0u, "initial positions");
        events.emplace_back("positions");
        MaybeFail("positions");
    }
    void DestructTree(std::uint32_t tree) override {
        Check(tree == 0x1040u, "tree cleanup address");
        events.emplace_back("destroy-tree");
    }
    void DestructBase(std::uint32_t object) override {
        Check(object == 0x1000u, "base cleanup address");
        events.emplace_back("destroy-base");
    }
};

void CheckConstruct() {
    Runtime runtime;
    Check(pal4::ScenarioDialog::Construct(0x1000u, runtime) ==
              0x1000u, "constructor return");
    Check(runtime.words.at(0x1000u) == pal4::ScenarioDialog::kVtable &&
              runtime.words.at(0x1028u) ==
                  pal4::ScenarioDialog::kTreeVtable &&
              runtime.bytes.at(0x1040u) == 0xa7u &&
              runtime.bytes.at(0x1041u) == 0xa7u &&
              runtime.bytes.at(0x1048u) == 0u &&
              runtime.bytes.at(0x103cu) == 0u,
          "vtable and byte fields");
    Check(runtime.names == std::vector<std::string>{
              "Scene/EditDialog", "Scene/ImgPortrait",
              "Scene/ImgClick", "Scene/ScenarioDialog"} &&
              runtime.words.at(0x1038u) == 0x2010u &&
              runtime.words.at(0x102cu) == 0x2020u &&
              runtime.words.at(0x1030u) == 0x2030u &&
              runtime.words.at(0x1034u) == 0x2040u,
          "four suffixes and member offsets");
    Check(runtime.disabled ==
              (std::vector<std::uint32_t>{0x2010u, 0x2040u,
                                          0x2020u}),
          "edit, scenario and portrait disable order");
    const auto first_find = std::find(runtime.events.begin(),
                                     runtime.events.end(), "find1");
    const auto last_find = std::find(runtime.events.begin(),
                                    runtime.events.end(), "find4");
    const auto topmost = std::find(runtime.events.begin(),
                                  runtime.events.end(), "topmost");
    Check(first_find != runtime.events.end() &&
              last_find < topmost &&
              runtime.events[runtime.events.size() - 2u] == "editbox" &&
              runtime.events.back() == "positions",
          "constructor call order");
}

void CheckFailure(const std::string& step,
                  const std::vector<std::string>& cleanup) {
    Runtime runtime;
    runtime.fail_at = step;
    bool threw = false;
    try {
        (void)pal4::ScenarioDialog::Construct(0x1000u, runtime);
    } catch (const std::runtime_error& error) {
        threw = std::string(error.what()) == step;
    }
    Check(threw, "constructor forwards failure");
    Check(runtime.events.size() >= cleanup.size(), "cleanup event count");
    Check(std::equal(cleanup.begin(), cleanup.end(),
                     runtime.events.end() - cleanup.size()),
          "stage-specific cleanup order");
}

}  // namespace

int main() {
    try {
        CheckConstruct();
        CheckFailure("layout", {});
        CheckFailure("tree", {"destroy-base"});
        for (const auto& step : {"prefix", "find1", "find2",
                                 "find3", "find4", "disable",
                                 "topmost", "editbox", "positions"})
            CheckFailure(step, {"destroy-tree", "destroy-base"});
        std::cout << "scenario_dialog_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
