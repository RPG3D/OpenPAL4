#include "pal4/script_quest_dialog.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptQuestDialogRuntime {
    std::vector<std::string> calls;
    pal4::ScriptFrameAttributeLookup lookup{0x2000u, 0x2000u};
    std::string expected_text = "quest";
    std::string fail_at;
    std::int32_t result = -4;

    void Record(std::string call) {
        calls.push_back(call);
        if (call == fail_at) throw std::runtime_error("injected failure");
    }
    std::uint32_t FrameManager() override {
        Record("manager");
        return 0x7000u;
    }
    std::uint32_t CreateQuestDialog(std::uint32_t manager) override {
        Check(manager == 0x7000u, "quest dialog manager");
        Record("dialog");
        return 0x9000u;
    }
    void ShowText(std::uint32_t dialog, const char* text) override {
        Check(dialog == 0x9000u && text == expected_text,
              "quest dialog text and empty fallback");
        Record("show-text");
    }
    std::uint32_t InitializeDocument() override {
        Record("document-init");
        return 0x8000u;
    }
    std::uint32_t DestroyDocument(std::uint32_t document) override {
        Check(document == 0x8000u, "quest XML document handle");
        Record("document-destroy");
        return 0x5678u;
    }
    void BeginFrameName(std::string_view name) override {
        Check(name == "frame", "frame name");
        Record("name-begin");
    }
    void EndFrameName() override { Record("name-end"); }
    pal4::ScriptFrameAttributeLookup FindAttribute(
        std::uint32_t document, std::string_view name) override {
        Check(document == 0x8000u && name == "frame",
              "quest frame attribute lookup");
        Record("lookup");
        return lookup;
    }
    std::uint32_t CreateFrameAttribute(std::string_view name,
                                       std::uint32_t frame) override {
        Check(name == "frame" && frame == 0x9000u,
              "quest frame attribute creation");
        Record("create");
        return 0xa000u;
    }
    void AddAttribute(std::uint32_t document,
                      std::uint32_t attribute) override {
        Check(document == 0x8000u && attribute == 0xa000u,
              "quest frame attribute addition");
        Record("add");
    }
    void DestroyAttribute(std::uint32_t attribute) override {
        Check(attribute == 0xa000u, "quest attribute cleanup handle");
        Record("attribute-destroy");
    }
    void SetExistingAttribute(std::uint32_t value_address,
                              std::uint32_t frame) override {
        Check(value_address == lookup.position + 0x1cu && frame == 0x9000u,
              "quest existing frame attribute value");
        Record("set");
    }
    std::uint32_t Game() override {
        Record("game");
        return 0xb000u;
    }
    void InitializeStateAndUi(std::uint32_t game, std::uint32_t state,
                              std::uint32_t document) override {
        Check(game == 0xb000u && state == 10u && document == 0x8000u,
              "quest state and XML payload");
        Record("state");
    }
    std::int32_t ReadSignedWord(std::uint32_t address) override {
        Check(address == 0x904cu, "quest result offset");
        Record("read");
        return result;
    }
};

void ShowBranches() {
    Runtime created;
    Check(pal4::ScriptQuestDialog::Show("quest", created) == 0x5678u &&
              created.calls == std::vector<std::string>{
                  "manager", "dialog", "show-text", "document-init",
                  "manager", "dialog", "name-begin", "lookup", "name-end",
                  "create", "add", "attribute-destroy", "game", "state",
                  "document-destroy"},
          "quest show text precedes document and second dialog lookup");
    Runtime empty;
    empty.expected_text = "";
    (void)pal4::ScriptQuestDialog::Show(nullptr, empty);
    Check(empty.calls[2] == "show-text", "null script text is empty");
    Runtime existing;
    existing.lookup = {0x3000u, 0x4000u};
    (void)pal4::ScriptQuestDialog::Show("quest", existing);
    Check(existing.calls == std::vector<std::string>{
              "manager", "dialog", "show-text", "document-init",
              "manager", "dialog", "name-begin", "lookup", "name-end",
              "set", "game", "state", "document-destroy"},
          "existing quest frame attribute updates in place");
    Runtime sentinel;
    sentinel.lookup = {0xffffffe4u, 0x4000u};
    (void)pal4::ScriptQuestDialog::Show("quest", sentinel);
    Check(sentinel.calls[9] == "create", "minus-28 cursor creates attribute");
}

void FailureCleanup() {
    for (const auto* fail : {"lookup", "add", "state"}) {
        Runtime runtime;
        runtime.fail_at = fail;
        bool caught = false;
        try {
            (void)pal4::ScriptQuestDialog::Show("quest", runtime);
        } catch (const std::runtime_error&) {
            caught = true;
        }
        Check(caught && runtime.calls.back() == "document-destroy",
              "quest failure destroys XML document");
        const auto count = [&](std::string_view name) {
            std::size_t total = 0;
            for (const auto& call : runtime.calls)
                total += (call == name);
            return total;
        };
        Check(count("name-end") == 1u, "frame name destroyed once");
        Check(count("attribute-destroy") ==
                  static_cast<std::size_t>(std::string_view(fail) != "lookup"),
              "created attribute destroyed once");
    }
}

void ResultPath() {
    for (auto value : {-4, 0, 2}) {
        Runtime runtime;
        runtime.result = value;
        Check(pal4::ScriptQuestDialog::Result(runtime) == value &&
                  runtime.calls == std::vector<std::string>{
                      "manager", "dialog", "read"},
              "quest result reads signed dialog word");
    }
}

}  // namespace

int main() {
    try {
        ShowBranches();
        FailureCleanup();
        ResultPath();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
