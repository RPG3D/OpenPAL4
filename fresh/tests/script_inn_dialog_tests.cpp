#include "pal4/script_inn_dialog.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptInnDialogRuntime {
    std::vector<std::string> calls;
    pal4::ScriptInnDialogAttributeLookup lookup{0x2000u, 0x2000u};
    std::string fail_at;
    std::uint32_t event_value = 0xffffffffu;
    std::uint32_t frame_value = 0u;
    std::int32_t result_value = -17;

    void Record(std::string name) {
        calls.push_back(name);
        if (name == fail_at) throw std::runtime_error("injected failure");
    }
    std::uint32_t FrameManager() override {
        Record("manager");
        return 0x7000u;
    }
    void ProcessEvent(std::uint32_t dispatcher, std::uint32_t key_cell,
                      std::uint32_t kind, std::uint32_t value) override {
        Check(dispatcher == 0x7104u &&
                  key_cell == pal4::ScriptInnDialog::kEventKeyCell &&
                  kind == 1u, "inn event layout");
        event_value = value;
        Record("event");
    }
    std::uint32_t InitializeDocument() override {
        Record("document-init");
        return 0x8000u;
    }
    std::uint32_t DestroyDocument(std::uint32_t document) override {
        Check(document == 0x8000u, "document handle");
        Record("document-destroy");
        return 0x1234u;
    }
    std::uint32_t FindFrame(std::uint32_t manager,
                            std::uint32_t key_cell) override {
        Check(manager == 0x7000u &&
                  key_cell == pal4::ScriptInnDialog::kFrameKeyCell,
              "SettleSelectWindow frame lookup");
        Record("frame");
        return 0x9000u;
    }
    void BeginFrameName(std::string_view name) override {
        Check(name == "frame", "temporary name");
        Record("name-begin");
    }
    void EndFrameName() override { Record("name-end"); }
    pal4::ScriptInnDialogAttributeLookup FindAttribute(
        std::uint32_t document, std::string_view name) override {
        Check(document == 0x8000u && name == "frame", "attribute lookup");
        Record("lookup");
        return lookup;
    }
    std::uint32_t CreateFrameAttribute(std::string_view name,
                                       std::uint32_t frame) override {
        Check(name == "frame" && frame == 0x9000u,
              "new frame attribute arguments");
        Record("create");
        return 0xa000u;
    }
    void AddAttribute(std::uint32_t document,
                      std::uint32_t attribute) override {
        Check(document == 0x8000u && attribute == 0xa000u,
              "append frame attribute");
        Record("add");
    }
    void DestroyAttribute(std::uint32_t attribute) override {
        Check(attribute == 0xa000u, "temporary attribute handle");
        Record("attribute-destroy");
    }
    void SetExistingAttribute(std::uint32_t value_address,
                              std::uint32_t frame) override {
        Check(value_address == lookup.position + 0x1cu,
              "existing value field offset");
        frame_value = frame;
        Record("set");
    }
    std::uint32_t Game() override {
        Record("game");
        return 0xb000u;
    }
    void InitializeStateAndUi(std::uint32_t game, std::uint32_t state,
                              std::uint32_t document) override {
        Check(game == 0xb000u && state == 10u && document == 0x8000u,
              "inn dialog state transition");
        Record("state");
    }
    std::int32_t ReadSignedWord(std::uint32_t address) override {
        Check(address == 0x9070u, "inn result frame offset");
        Record("read");
        return result_value;
    }
};

void ShowBranches() {
    for (auto value : {0u, 1u, 255u, 0x12345678u}) {
        Runtime runtime;
        Check(pal4::ScriptInnDialog::Show(value, runtime) == 0x1234u,
              "show returns document destructor result");
        Check(runtime.event_value == (value & 0xffu), "event low byte");
        Check(runtime.calls == std::vector<std::string>{
                  "manager", "event", "document-init", "manager", "frame",
                  "name-begin", "lookup", "name-end", "create", "add",
                  "attribute-destroy", "game", "state", "document-destroy"},
              "new attribute call order");
    }
    Runtime existing;
    existing.lookup = {0x3000u, 0x4000u};
    (void)pal4::ScriptInnDialog::Show(0u, existing);
    Check(existing.frame_value == 0x9000u &&
              existing.calls == std::vector<std::string>{
                  "manager", "event", "document-init", "manager", "frame",
                  "name-begin", "lookup", "name-end", "set", "game",
                  "state", "document-destroy"},
          "existing attribute updates value field");
    Runtime sentinel;
    sentinel.lookup = {0xffffffe4u, 0x4000u};
    (void)pal4::ScriptInnDialog::Show(0u, sentinel);
    Check(sentinel.calls[8] == "create", "minus-28 cursor creates attribute");
}

void CleanupFailures() {
    for (const auto* fail : {"lookup", "add", "set", "state"}) {
        Runtime runtime;
        runtime.fail_at = fail;
        if (std::string_view(fail) == "set")
            runtime.lookup = {0x3000u, 0x4000u};
        bool caught = false;
        try {
            (void)pal4::ScriptInnDialog::Show(1u, runtime);
        } catch (const std::runtime_error&) {
            caught = true;
        }
        Check(caught && runtime.calls.back() == "document-destroy",
              "failure destroys XML document");
        const auto count = [&](std::string_view name) {
            std::size_t total = 0;
            for (const auto& call : runtime.calls)
                total += (call == name);
            return total;
        };
        Check(count("name-end") == 1u,
              "temporary frame name released exactly once");
        Check(count("attribute-destroy") ==
                  static_cast<std::size_t>(std::string_view(fail) == "add" ||
                                           std::string_view(fail) == "state"),
              "created attribute released exactly once");
    }
}

void ResultPath() {
    for (auto value : {-17, 0, 42}) {
        Runtime runtime;
        runtime.result_value = value;
        Check(pal4::ScriptInnDialog::Result(runtime) == value,
              "signed result forwarded");
        Check(runtime.calls == std::vector<std::string>{
                  "manager", "manager", "frame", "read"},
              "shared tail reacquires manager and reads frame");
    }
}

}  // namespace

int main() {
    try {
        ShowBranches();
        CleanupFailures();
        ResultPath();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
