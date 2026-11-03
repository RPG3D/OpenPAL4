#include "pal4/ui_frame_manager_singleton.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Events final : pal4::IUiFrameManagerEventRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t key_calls = 0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read " + std::to_string(address));
        return words[address];
    }
    std::uint8_t ProcessEvent(
        std::uint32_t tree, std::uint32_t key,
        const pal4::UiFrameEventPayload& payload) override {
        Check(tree == 0x5104u && key == 0x8a80a8u &&
                  payload.type == 0xabu &&
                  payload.first == 0x11112222u &&
                  payload.second == 0x33334444u,
              "event tree, key and copied payload");
        calls.emplace_back("event");
        return 0x5au;
    }
    std::uint8_t ProcessUiEvent(
        std::uint32_t system, std::uint32_t hwnd,
        std::uint32_t message, std::uint32_t wparam,
        std::uint32_t lparam) override {
        Check(hwnd == 0u && wparam == 0x12345678u && lparam == 0u,
              "keyboard message arguments");
        if (key_calls == 0u) {
            Check(system == 0x7000u && message == 0x100u,
                  "first event is keydown");
            words[0x5198u] = 0x8000u;
        } else {
            Check(system == 0x8000u && message == 0x101u,
                  "second event is keyup with reread system");
        }
        ++key_calls;
        calls.emplace_back("keyboard");
        return key_calls == 1u ? 0xffu : 0x34u;
    }
};

struct Coordinates final : pal4::IUiFrameManagerCoordinatesRuntime {
    std::uint32_t system = 0x7000u;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x5198u, "CEGUI system field");
        calls.emplace_back("read");
        return system;
    }
    void TransformMouse(std::uint32_t receiver,
                        std::uint32_t output,
                        std::uint32_t input) override {
        Check(receiver == 0x7000u && output == 0x9000u &&
                  input == 0xa000u, "mouse transform arguments");
        calls.emplace_back("mouse");
    }
    void TransformCegui(std::uint32_t receiver,
                        std::uint32_t output,
                        std::uint32_t input) override {
        Check(receiver == 0x8000u && output == 0x9000u &&
                  input == 0xa000u, "CEGUI transform arguments");
        calls.emplace_back("cegui");
    }
};

struct Resources final : pal4::IUiFrameManagerResourceRuntime {
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x519cu, "resource collection field");
        calls.emplace_back("read");
        return 0x7000u;
    }
    std::uint32_t AddElement(std::uint32_t collection,
                             const char* prefix,
                             const char* name) override {
        Check(collection == 0x7000u &&
                  std::string(prefix) == "gamedata\\ui\\" &&
                  std::string(name) == "test.xml",
              "resource prefix and filename");
        calls.emplace_back("add");
        return 0x1234u;
    }
};

void CheckEvents() {
    Events runtime;
    runtime.words[0x9000u] = 0x11112222u;
    runtime.words[0x9004u] = 0x33334444u;
    Check(pal4::UiFrameManagerSingleton::ProcessUiEvents(
              0x5000u, 0x1abu, 0x9000u, runtime) == 0x5au &&
              runtime.calls == std::vector<std::string>{
                  "read 36864", "read 36868", "event"},
          "event wrapper copies two words and masks type");

    runtime.calls.clear();
    runtime.words[0x5198u] = 0x7000u;
    Check(pal4::UiFrameManagerSingleton::SimulateKeyPressAndRelease(
              0x5000u, 0x12345678u, runtime) == 0x34u &&
              runtime.calls == std::vector<std::string>{
                  "read 20888", "keyboard", "read 20888", "keyboard"},
          "keydown return discarded and system reread for keyup");
}

void CheckCoordinatesAndResource() {
    Coordinates coordinates;
    Check(pal4::UiFrameManagerSingleton::ConvertUiFrameCoordinates(
              0x5000u, 0x9000u, 0xa000u, coordinates) == 0x9000u &&
              coordinates.calls ==
                  std::vector<std::string>{"read", "mouse"},
          "mouse conversion forwards addresses and returns output");
    coordinates.system = 0x8000u;
    coordinates.calls.clear();
    Check(pal4::UiFrameManagerSingleton::TransformCeguiCoordinates(
              0x5000u, 0x9000u, 0xa000u, coordinates) == 0x9000u &&
              coordinates.calls ==
                  std::vector<std::string>{"read", "cegui"},
          "CEGUI conversion uses its own transform");

    Resources resources;
    Check(pal4::UiFrameManagerSingleton::AddUiResource(
              0x5000u, "test.xml", resources) == 0x1234u &&
              resources.calls ==
                  std::vector<std::string>{"read", "add"},
          "UI resource prefix and result forwarded");
}

}  // namespace

int main() {
    try {
        CheckEvents();
        CheckCoordinatesAndResource();
        std::cout << "ui_frame_manager_forwarders_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
