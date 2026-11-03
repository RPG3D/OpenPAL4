#include "pal4/render_system_function_dispatcher.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IRenderSystemFunctionRuntime {
public:
    std::uint32_t GetRenderSystem() override {
        events.emplace_back("get");
        return 0x1000u;
    }
    std::uint32_t ReadSlot(const std::uint32_t system,
                            const std::uint32_t offset) override {
        events.emplace_back("slot");
        Check(system == 0x1000u, "wrapper reads active render system");
        last_offset = offset;
        return has_function ? 0x2000u + offset : 0u;
    }
    std::uint32_t Invoke(const std::uint32_t function,
                          const std::uint32_t argument) override {
        events.emplace_back("invoke");
        Check(function == 0x2000u + last_offset &&
                  argument == 0xaabbccddu,
              "wrapper forwards slot function and original argument");
        return function ^ argument;
    }
    bool has_function = true;
    std::uint32_t last_offset = 0u;
    std::vector<std::string> events;
};

using Dispatcher = pal4::RenderSystemFunctionDispatcher;
using Method = std::uint32_t (Dispatcher::*)(std::uint32_t);

void EveryWrapper() {
    constexpr std::array<std::pair<Method, std::uint32_t>, 12> methods{{
        {&Dispatcher::Call48, 0x30u}, {&Dispatcher::Call52, 0x34u},
        {&Dispatcher::Call56, 0x38u}, {&Dispatcher::Call60, 0x3cu},
        {&Dispatcher::Call64, 0x40u}, {&Dispatcher::Call68, 0x44u},
        {&Dispatcher::Call72, 0x48u}, {&Dispatcher::Call76, 0x4cu},
        {&Dispatcher::Call80, 0x50u}, {&Dispatcher::Call84, 0x54u},
        {&Dispatcher::Call88, 0x58u}, {&Dispatcher::Call92, 0x5cu},
    }};
    Runtime runtime;
    Dispatcher dispatcher(runtime);
    for (const auto& [method, offset] : methods) {
        runtime.events.clear();
        Check((dispatcher.*method)(0xaabbccddu) ==
                  ((0x2000u + offset) ^ 0xaabbccddu) &&
                  runtime.last_offset == offset &&
                  runtime.events ==
                      std::vector<std::string>{"get", "slot", "invoke"},
              "each wrapper reads its exact slot and forwards result");
        runtime.has_function = false;
        runtime.events.clear();
        Check((dispatcher.*method)(0xaabbccddu) == 2u &&
                  runtime.last_offset == offset &&
                  runtime.events ==
                      std::vector<std::string>{"get", "slot"},
              "each wrapper returns two for a null function pointer");
        runtime.has_function = true;
    }
}

void KeyboardDispatch() {
    Runtime runtime;
    Dispatcher dispatcher(runtime);
    for (const auto [code, offset] :
         {std::pair{22u, 0x30u}, std::pair{23u, 0x34u},
          std::pair{24u, 0x38u}}) {
        runtime.events.clear();
        Check(dispatcher.DispatchKeyboard(code, 0xaabbccddu) ==
                  ((0x2000u + offset) ^ 0xaabbccddu) &&
                  runtime.last_offset == offset,
              "keyboard code chooses observed render slot");
    }
    for (const auto code : {0u, 21u, 25u, 0xffffffffu}) {
        runtime.events.clear();
        Check(dispatcher.DispatchKeyboard(code, 0xaabbccddu) == 2u &&
                  runtime.events.empty(),
              "unmapped keyboard code returns two without runtime call");
    }
}

void MouseDispatch() {
    constexpr std::array<std::pair<std::uint32_t, std::uint32_t>, 9> mapping{{
        {5u, 0x3cu}, {6u, 0x40u}, {7u, 0x44u},
        {8u, 0x48u}, {9u, 0x4cu}, {10u, 0x50u},
        {11u, 0x58u}, {12u, 0x54u}, {13u, 0x5cu},
    }};
    Runtime runtime;
    Dispatcher dispatcher(runtime);
    for (const auto [code, offset] : mapping) {
        runtime.events.clear();
        Check(dispatcher.DispatchMouse(code, 0xaabbccddu) ==
                  ((0x2000u + offset) ^ 0xaabbccddu) &&
                  runtime.last_offset == offset &&
                  runtime.events ==
                      std::vector<std::string>{"get", "slot", "invoke"},
              "mouse jump table including 11/12 slot swap");
    }
    for (const auto code : {0u, 4u, 14u, 0xffffffffu}) {
        runtime.events.clear();
        Check(dispatcher.DispatchMouse(code, 0xaabbccddu) == 2u &&
                  runtime.events.empty(),
              "unmapped mouse code returns two without runtime call");
    }
}

}  // namespace

int main() {
    EveryWrapper();
    KeyboardDispatch();
    MouseDispatch();
    std::cout << "12 render wrappers and two dispatchers passed\n";
}
