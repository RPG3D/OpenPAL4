#include "pal4/script_dialogs.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::IScriptDialogsRuntime {
public:
    pal4::ScriptDialogWaitState wait{};
    std::vector<std::string> calls;
    bool allocation_fails = false;
    bool visible = true;
    bool xml_fails = false;
    std::uint32_t expected_vtable = 0x849648u;
    std::uint32_t expected_mode = 4u;
    std::string text;

    std::uint32_t UiFrameManager() override {
        calls.emplace_back("ui manager");
        return 0x4000u;
    }
    std::uint32_t CreateCommonDialog(std::uint32_t manager) override {
        Check(manager == 0x4000u, "common dialog manager");
        calls.emplace_back("common dialog");
        return 0x7000u;
    }
    std::uint32_t CreateSelectDialog(std::uint32_t manager) override {
        Check(manager == 0x4000u, "select dialog manager");
        calls.emplace_back("select dialog");
        return 0x8000u;
    }
    void ShowCommon(std::string_view message,
                    std::int32_t first,
                    std::int32_t second,
                    std::uint32_t mode,
                    std::uint32_t flags) override {
        Check(first == 3 && second == -2 && mode == expected_mode &&
                  flags == 0u,
              "common dialog operands");
        text = message;
        calls.emplace_back("show common");
    }
    void SetSelectQuestion(std::string_view question,
                           std::uint32_t option) override {
        Check(option == 0u, "select question option");
        text = question;
        calls.emplace_back("set question");
    }
    pal4::ScriptDialogWaitState* AllocateWait(
        std::uint32_t bytes) override {
        Check(bytes == 0x20u, "dialog wait allocation size");
        calls.emplace_back("allocate");
        return allocation_fails ? nullptr : &wait;
    }
    std::uint32_t ScriptExecutor() override {
        calls.emplace_back("executor");
        return 0x6000u;
    }
    void ConstructWait(pal4::ScriptDialogWaitState& state,
                       std::uint8_t mode,
                       std::uint32_t queue) override {
        Check(&state == &wait && mode == 1u && queue == 0x6054u,
              "dialog wait constructor");
        calls.emplace_back("construct");
    }
    void FlushWait(pal4::ScriptDialogWaitState& state) override {
        Check(&state == &wait &&
                  state.vtable_address == expected_vtable,
              "dialog wait vtable before flush");
        calls.emplace_back("flush");
    }
    std::uint32_t AddHandler(
        std::uint32_t queue, pal4::ScriptDialogWaitState* state) override {
        Check(queue == 0x6054u &&
                  state == (allocation_fails ? nullptr : &wait),
              "dialog wait queue");
        calls.emplace_back("add handler");
        return 0x1234u;
    }
    bool IsVisible(std::uint32_t dialog) override {
        Check(dialog == 0x7000u || dialog == 0x8000u,
              "dialog visibility receiver");
        calls.emplace_back("visible");
        return visible;
    }
    void CloseWait(pal4::ScriptDialogWaitState& state) override {
        Check(&state == &wait, "dialog wait close receiver");
        calls.emplace_back("close");
    }
    std::uint32_t CreateDocument() override {
        calls.emplace_back("create document");
        return 0x9000u;
    }
    void UpsertPointerAttribute(std::uint32_t document,
                                std::string_view key,
                                std::uint32_t value) override {
        Check(document == 0x9000u && key == "frame" &&
                  value == 0x8000u, "select Frame attribute");
        calls.emplace_back("Frame attribute");
        if (xml_fails) throw std::runtime_error("XML failure");
    }
    void EnterState(std::uint32_t state,
                    std::uint32_t document) override {
        Check(state == 10u && document == 0x9000u,
              "select state transition");
        calls.emplace_back("enter state");
    }
    std::uint32_t DestroyDocument(std::uint32_t document) override {
        Check(document == 0x9000u, "select document cleanup");
        calls.emplace_back("destroy document");
        return 0xabcdU;
    }
};

void CommonCases() {
    Runtime runtime;
    pal4::ScriptDialogs dialogs(runtime);
    Check(dialogs.ShowCommon("Hello", 3, -2) == 0x1234u &&
              runtime.text == "Hello" &&
              runtime.calls == std::vector<std::string>{
                  "ui manager", "common dialog", "show common",
                  "allocate", "executor", "construct", "flush",
                  "executor", "add handler"},
          "common dialog shows before queuing wait");
    runtime.calls.clear();
    dialogs.UpdateCommon(runtime.wait);
    Check(runtime.calls == std::vector<std::string>{
              "ui manager", "common dialog", "visible"},
          "visible common dialog keeps waiting");
    runtime.visible = false;
    runtime.calls.clear();
    dialogs.UpdateCommon(runtime.wait);
    Check(runtime.calls == std::vector<std::string>{
              "ui manager", "common dialog", "visible", "close"},
          "hidden common dialog closes wait");

    Runtime failed;
    failed.allocation_fails = true;
    pal4::ScriptDialogs failed_dialogs(failed);
    Check(failed_dialogs.ShowCommon({}, 3, -2) == 0x1234u &&
              failed.text.empty() &&
              failed.calls == std::vector<std::string>{
                  "ui manager", "common dialog", "show common",
                  "allocate", "executor", "add handler"},
          "common dialog queues null on allocation failure");
}

void CommonSelectModeCases() {
    Runtime runtime;
    runtime.expected_mode = 3u;
    pal4::ScriptDialogs dialogs(runtime);
    Check(dialogs.ShowCommonInSelectMode("Pick", 3, -2) == 0x1234u &&
              runtime.text == "Pick" &&
              runtime.calls == std::vector<std::string>{
                  "ui manager", "common dialog", "show common",
                  "allocate", "executor", "construct", "flush",
                  "executor", "add handler"},
          "select-mode common dialog uses mode three and shared wait");

    Runtime failed;
    failed.expected_mode = 3u;
    failed.allocation_fails = true;
    pal4::ScriptDialogs failed_dialogs(failed);
    Check(failed_dialogs.ShowCommonInSelectMode({}, 3, -2) == 0x1234u &&
              failed.text.empty() &&
              failed.calls == std::vector<std::string>{
                  "ui manager", "common dialog", "show common",
                  "allocate", "executor", "add handler"},
          "select-mode dialog queues null after failed allocation");
}

void SelectCases() {
    Runtime runtime;
    runtime.expected_vtable = 0x849654u;
    pal4::ScriptDialogs dialogs(runtime);
    Check(dialogs.ShowSelect("Choose") == 0xabcdu &&
              runtime.text == "Choose" &&
              runtime.calls == std::vector<std::string>{
                  "ui manager", "select dialog", "set question",
                  "allocate", "executor", "construct", "flush",
                  "executor", "add handler", "create document",
                  "ui manager", "select dialog", "Frame attribute",
                  "enter state", "destroy document"},
          "select dialog queues wait before Frame state transition");
    runtime.calls.clear();
    dialogs.UpdateSelect(runtime.wait);
    Check(runtime.calls == std::vector<std::string>{
              "ui manager", "select dialog", "visible"},
          "visible select dialog keeps waiting");
    runtime.visible = false;
    runtime.calls.clear();
    dialogs.UpdateSelect(runtime.wait);
    Check(runtime.calls == std::vector<std::string>{
              "ui manager", "select dialog", "visible", "close"},
          "hidden select dialog closes wait");

    Runtime failed;
    failed.allocation_fails = true;
    pal4::ScriptDialogs failed_dialogs(failed);
    Check(failed_dialogs.ShowSelect({}) == 0xabcdu &&
              failed.calls == std::vector<std::string>{
                  "ui manager", "select dialog", "set question",
                  "allocate", "executor", "add handler",
                  "create document", "ui manager", "select dialog",
                  "Frame attribute", "enter state", "destroy document"},
          "select dialog still enters state after null handler");

    Runtime throws;
    throws.expected_vtable = 0x849654u;
    throws.xml_fails = true;
    pal4::ScriptDialogs throws_dialogs(throws);
    bool caught = false;
    try {
        static_cast<void>(throws_dialogs.ShowSelect("Choose"));
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && throws.calls.back() == "destroy document",
          "select XML failure destroys temporary document");
}

}  // namespace

int main() {
    try {
        CommonCases();
        CommonSelectModeCases();
        SelectCases();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
