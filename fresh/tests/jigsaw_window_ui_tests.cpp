#include "pal4/jigsaw_window_ui.h"

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

struct Runtime final : pal4::IJigsawWindowUiRuntime {
    static constexpr std::uint32_t frame = 0x1000u;
    static constexpr std::uint32_t message = 0x2000u;
    static constexpr std::uint32_t name = 0x8b1240u;
    static constexpr std::uint32_t manager = 0x3000u;
    static constexpr std::uint32_t document = 0x4000u;
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {0x8b123cu, name}, {message, 1u}, {message + 4u, 3u}};
    std::vector<std::string> calls;
    std::uint8_t active = 0u;
    std::uint32_t parent = 0u;
    std::uint32_t insertion = 0u;
    std::uint32_t removed = 0u;
    std::uint32_t event_type = 0u;
    std::uint32_t event_payload = 0xffffffffu;
    std::uint32_t found = frame;
    std::uint32_t applied_step = 0xffffffffu;
    bool throw_initialize = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        Check(address == frame + 0x18cu && value == 0u,
              "puzzle state flag reset");
        calls.emplace_back("clear-flag");
    }
    void SetParent(std::uint32_t target, std::uint8_t value,
                   std::uint32_t new_parent) override {
        Check(target == frame, "parent frame");
        active = value;
        parent = new_parent;
        calls.emplace_back("parent");
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return manager;
    }
    std::uint32_t InsertActive(std::uint32_t array,
                                std::uint32_t value) override {
        Check(array == manager + 0x134u, "active array");
        insertion = value;
        calls.emplace_back("insert");
        return 0xa5u;
    }
    void RemoveActive(std::uint32_t array,
                      std::uint32_t value) override {
        Check(array == manager + 0x134u, "active array removal");
        removed = value;
        calls.emplace_back("remove");
    }
    std::uint32_t CreateXmlDocument() override {
        calls.emplace_back("xml-create");
        return document;
    }
    std::uint32_t Game() override {
        calls.emplace_back("game");
        return 0x5000u;
    }
    void InitializeStateAndUI(std::uint32_t game,
                              std::uint32_t mode,
                              std::uint32_t xml) override {
        Check(game == 0x5000u && mode == 1u && xml == document,
              "game reinitialization arguments");
        calls.emplace_back("initialize");
        if (throw_initialize)
            throw std::runtime_error("initialization failed");
    }
    std::uint32_t DestroyXmlDocument(std::uint32_t xml) override {
        Check(xml == document, "document destruction");
        calls.emplace_back("xml-destroy");
        return 0xb6u;
    }
    void HandleFrameEvent(std::uint32_t event_name,
                          std::uint32_t type,
                          std::uint32_t payload) override {
        Check(event_name == name, "jigsaw event name");
        event_type = type;
        event_payload = payload;
        calls.emplace_back("event");
    }
    std::uint32_t FindFrame(std::uint32_t target,
                            std::uint32_t window_name) override {
        Check(target == manager && window_name == name,
              "jigsaw frame lookup");
        calls.emplace_back("find");
        return found;
    }
    void SetStepWithTimer(std::uint32_t target,
                          std::uint32_t step) override {
        Check(target == frame, "jigsaw step frame");
        applied_step = step;
        calls.emplace_back("step");
    }
};

void CheckVisibility() {
    Runtime active;
    Check(pal4::JigsawWindowUi::SetVisible(
              Runtime::frame, 2u, 0x6000u, active) == 0xa5u &&
              active.active == 2u && active.parent == 0x6000u &&
              active.insertion == Runtime::frame + 0x28u &&
              active.calls == (std::vector<std::string>{
                  "parent", "clear-flag", "manager", "insert"}),
          "show path inserts frame record and forwards result");
    Runtime hidden;
    Check(pal4::JigsawWindowUi::SetVisible(
              Runtime::frame, 0u, 0u, hidden) == 0xb6u &&
              hidden.removed == Runtime::frame + 0x28u &&
              hidden.calls == (std::vector<std::string>{
                  "parent", "clear-flag", "manager", "remove",
                  "xml-create", "game", "initialize", "xml-destroy"}),
          "hide path removes frame and reinitializes game state");
    Runtime throwing;
    throwing.throw_initialize = true;
    bool caught = false;
    try {
        (void)pal4::JigsawWindowUi::SetVisible(
            Runtime::frame, 0u, 0u, throwing);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && throwing.calls.back() == "xml-destroy",
          "exception releases temporary XML document");
}

void CheckMessages() {
    Runtime runtime;
    Check(pal4::JigsawWindowUi::HandleShowMessage(
              Runtime::message, runtime) == 1u &&
              runtime.event_type == 1u && runtime.event_payload == 0u &&
              runtime.applied_step == 3u &&
              runtime.calls == (std::vector<std::string>{
                  "read", "read", "event", "read", "read", "manager",
                  "find", "step"}),
          "matching show message dispatches and sets step");
    Runtime other_type;
    other_type.words[Runtime::message] = 2u;
    Check(pal4::JigsawWindowUi::HandleShowMessage(
              Runtime::message, other_type) == 1u &&
              other_type.applied_step == 0xffffffffu &&
              other_type.calls == (std::vector<std::string>{
                  "read", "read", "event", "read"}),
          "other event type only dispatches UI event");
    Runtime boundary;
    boundary.words[Runtime::message + 4u] = 12u;
    Check(pal4::JigsawWindowUi::HandleShowMessage(
              Runtime::message, boundary) == 1u &&
              boundary.applied_step == 0xffffffffu,
          "step twelve is outside upper bound");
    Runtime missing;
    missing.found = 0u;
    Check(pal4::JigsawWindowUi::HandleShowMessage(
              Runtime::message, missing) == 1u &&
              missing.applied_step == 0xffffffffu,
          "missing frame skips step setter");
    Runtime wrapped;
    wrapped.words[Runtime::message + 4u] = 0xfffffffau;
    Check(pal4::JigsawWindowUi::HandleShowMessage(
              Runtime::message, wrapped) == 1u &&
              wrapped.applied_step == 0xfffffffau,
          "signed x86 comparison accepts wrapped negative step");
}

}  // namespace

int main() {
    try {
        CheckVisibility();
        CheckMessages();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
