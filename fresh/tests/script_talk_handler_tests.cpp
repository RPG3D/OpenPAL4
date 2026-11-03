#include "pal4/script_talk_handler.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::IScriptTalkHandlerRuntime {
public:
    pal4::ScenarioDialogWaitState state{};
    std::vector<std::string> calls;
    std::uint8_t voice_flag = 1u;
    std::uint8_t visible = 0u;
    bool allocation_fails = false;
    int executor_calls = 0;
    int close_calls = 0;
    int hide_calls = 0;
    std::string expected_voice_path = "gamedata\\PALSound\\VO001.mp3";

    std::uint8_t VoiceFlag() override {
        calls.emplace_back("voice flag");
        return voice_flag;
    }
    std::uint32_t ScriptExecutor() override {
        calls.emplace_back("executor");
        ++executor_calls;
        return 0x5000u;
    }
    std::uint32_t AudioManager() override {
        calls.emplace_back("audio manager");
        return 0x6000u;
    }
    std::uint32_t PlayMusic(std::uint32_t manager, std::string_view path,
                            std::uint32_t mode) override {
        Check(manager == 0x6000u && mode == 1u &&
                  path == expected_voice_path,
              "voice path, audio receiver and mode");
        calls.emplace_back("play voice");
        return 0x918u;
    }
    void StoreVoiceHandle(std::uint32_t address,
                          std::uint32_t handle) override {
        Check(address == 0x5024u && handle == 0x918u,
              "voice handle at script executor +0x24");
        calls.emplace_back("store voice");
    }
    pal4::ScenarioDialogWaitState* Allocate(std::uint32_t size) override {
        Check(size == 0x20u, "scenario wait allocation size");
        calls.emplace_back("allocate");
        return allocation_fails ? nullptr : &state;
    }
    void ConstructBase(pal4::ScenarioDialogWaitState& handler,
                       std::uint32_t wait_mode,
                       std::uint32_t queue_address) override {
        Check(&handler == &state && wait_mode == 1u &&
                  queue_address == 0x5054u,
              "blocking base constructor and executor queue");
        state.vtable_address = 0xdeadbeefu;
        calls.emplace_back("construct");
    }
    void Flush(pal4::ScenarioDialogWaitState& handler) override {
        Check(&handler == &state &&
                  handler.vtable_address == 0x849520u,
              "derived scenario wait vtable before flush");
        calls.emplace_back("flush");
    }
    std::uint32_t AddHandler(std::uint32_t queue_address,
                             pal4::ScenarioDialogWaitState* handler) override {
        Check(queue_address == 0x5054u &&
                  handler == (allocation_fails ? nullptr : &state),
              "queue receives allocation result, including null");
        calls.emplace_back("add");
        return 0x1234u;
    }
    std::uint32_t UiFrameManager() override {
        calls.emplace_back("ui manager");
        return 0x7000u;
    }
    std::uint32_t CreateScenarioDialog(std::uint32_t manager) override {
        Check(manager == 0x7000u, "scenario dialog manager");
        calls.emplace_back("dialog");
        return 0x8000u;
    }
    std::uint8_t IsVisible(std::uint32_t dialog) override {
        Check(dialog == 0x8000u, "scenario dialog visibility receiver");
        calls.emplace_back("visible");
        return visible;
    }
    std::uint8_t SetText(std::uint32_t dialog, std::string_view text,
                         std::uint32_t display_value) override {
        Check(dialog == 0x8000u && text == "dialog text" &&
                  display_value == 0x3dcccccdu,
              "hidden dialog text and original float bits");
        calls.emplace_back("set text");
        return 0x42u;
    }
    void HideDialog(std::uint32_t dialog, std::uint32_t first,
                    std::uint32_t second) override {
        Check(dialog == 0x8000u && first == 0u && second == 0u,
              "dialog virtual slot +4 operands");
        ++hide_calls;
        calls.emplace_back("hide");
    }
    void SetClosedFlag(pal4::ScenarioDialogWaitState& handler) override {
        Check(&handler == &state, "scenario wait close receiver");
        ++close_calls;
        calls.emplace_back("close");
    }
};

void VoiceAndHiddenDialog() {
    Runtime runtime;
    pal4::ScriptTalkHandler talk(runtime);
    Check(talk.Invoke("dialog text", "VO001") == 0x42u,
          "hidden dialog returns SetText result");
    Check(runtime.calls == std::vector<std::string>{
              "voice flag", "executor", "audio manager", "play voice",
              "store voice", "allocate", "executor", "construct", "flush",
              "executor", "add", "ui manager", "dialog", "visible",
              "set text"},
          "voice, wait-handler and dialog order");
    Check(runtime.executor_calls == 3,
          "voice and handler paths reacquire executor separately");
}

void VisibleDialogSkipsTextAndNonOneFlagSkipsVoice() {
    Runtime runtime;
    runtime.voice_flag = 2u;
    runtime.visible = 0x7fu;
    pal4::ScriptTalkHandler talk(runtime);
    Check(talk.Invoke("dialog text", "VO001") == 0x7fu,
          "visible dialog returns unmodified visibility byte");
    Check(runtime.calls == std::vector<std::string>{
              "voice flag", "allocate", "executor", "construct", "flush",
              "executor", "add", "ui manager", "dialog", "visible"} &&
              runtime.executor_calls == 2,
          "non-one flag skips audio and visible dialog skips text");
}

void EmptyVoiceAndAllocationFailure() {
    Runtime runtime;
    runtime.allocation_fails = true;
    pal4::ScriptTalkHandler talk(runtime);
    Check(talk.Invoke("dialog text", "") == 0x42u,
          "empty voice ID still displays dialog");
    Check(runtime.calls == std::vector<std::string>{
              "voice flag", "allocate", "executor", "add", "ui manager",
              "dialog", "visible", "set text"} &&
              runtime.executor_calls == 1,
          "failed allocation queues null without base construction");
}

void VoiceStillPlaysWhenHandlerAllocationFails() {
    Runtime runtime;
    runtime.allocation_fails = true;
    pal4::ScriptTalkHandler talk(runtime);
    Check(talk.Invoke("dialog text", "VO001") == 0x42u &&
              runtime.executor_calls == 2,
          "voice branch finishes before failed handler allocation");
    Check(runtime.calls[3] == "play voice" &&
              runtime.calls[4] == "store voice" &&
              runtime.calls[5] == "allocate",
          "voice handle stored before handler allocation");
}

void WaitCallbacks() {
    Runtime runtime;
    pal4::ScriptTalkHandler talk(runtime);
    runtime.visible = 1u;
    talk.Update(runtime.state);
    Check(runtime.close_calls == 0 &&
              runtime.calls == std::vector<std::string>{
                  "ui manager", "dialog", "visible"},
          "visible scenario dialog keeps script waiting");
    runtime.calls.clear();
    runtime.visible = 0u;
    talk.Update(runtime.state);
    Check(runtime.close_calls == 1 &&
              runtime.calls == std::vector<std::string>{
                  "ui manager", "dialog", "visible", "close"},
          "hidden scenario dialog closes wait handler");
    runtime.calls.clear();
    talk.Close(runtime.state);
    Check(runtime.hide_calls == 1 && runtime.close_calls == 2 &&
              runtime.calls == std::vector<std::string>{
                  "ui manager", "dialog", "hide", "close"},
          "explicit close invokes dialog slot +4 before closing handler");
}

void TalkWaitRegistration() {
    Runtime runtime;
    pal4::ScriptTalkHandler talk(runtime);
    Check(talk.Wait() == 0x1234u &&
              runtime.calls == std::vector<std::string>{
                  "allocate", "executor", "construct", "flush",
                  "executor", "add"} &&
              runtime.executor_calls == 2,
          "giTalkWait constructs, flushes and queues handler");

    Runtime failed;
    failed.allocation_fails = true;
    pal4::ScriptTalkHandler failed_talk(failed);
    Check(failed_talk.Wait() == 0x1234u &&
              failed.calls == std::vector<std::string>{
                  "allocate", "executor", "add"} &&
              failed.executor_calls == 1,
          "giTalkWait still queues null and returns queue result");
}

}  // namespace

int main() {
    try {
        VoiceAndHiddenDialog();
        VisibleDialogSkipsTextAndNonOneFlagSkipsVoice();
        EmptyVoiceAndAllocationFailure();
        VoiceStillPlaysWhenHandlerAllocationFails();
        WaitCallbacks();
        TalkWaitRegistration();
        pal4::ScriptTalkHandlerRuntimeStub stub;
        pal4::ScriptTalkHandler talk(stub);
        Check(talk.Invoke("dialog text", "") == 0u,
              "linkable native-boundary stub");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
