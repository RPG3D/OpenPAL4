#include "pal4/cegui_ui_event_processor.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::ICeguiUiEventRuntime {
public:
    std::vector<std::string> events;
    std::map<std::uint32_t, std::uint32_t> key_map;
    std::map<std::uint32_t, std::uint32_t> words{
        {0x701cu, 0x8000u}, {0x7030u, 0x9000u}};
    std::uint32_t system_pointer = 0x7000u;
    std::uint32_t last_key = 0u;
    std::uint32_t last_button = 0xffffffffu;
    float wheel = 0.0f;
    std::array<float, 2> mouse_input{};
    std::array<float, 2> mouse_injected{};
    bool injection_result = true;

    std::uint32_t MapVirtualKey(std::uint32_t owner,
                                std::uint32_t virtual_key) override {
        Check(owner == 0x1000u, "virtual key uses UI owner");
        events.emplace_back("map-key");
        const auto it = key_map.find(virtual_key);
        return it == key_map.end() ? virtual_key : it->second;
    }
    std::uint32_t MapImeCharacter(std::uint32_t wparam) override {
        Check(wparam == 0x1234u, "IME conversion receives packed parameter");
        events.emplace_back("map-ime");
        return 0x4321u;
    }
    std::uint32_t GetSystemSingleton() override {
        events.emplace_back("system");
        return 0x7000u;
    }
    std::uint32_t GetSystemSingletonPtr() override {
        events.emplace_back("system-ptr");
        return system_pointer;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        events.emplace_back("read:" + std::to_string(address));
        return words.at(address);
    }
    bool InjectKeyDown(std::uint32_t system,
                       std::uint32_t key) override {
        Check(system == 0x7000u, "key down system");
        last_key = key;
        events.emplace_back("key-down");
        return injection_result;
    }
    bool InjectKeyUp(std::uint32_t system,
                     std::uint32_t key) override {
        Check(system == 0x7000u, "key up system");
        last_key = key;
        events.emplace_back("key-up");
        return injection_result;
    }
    bool InjectChar(std::uint32_t system,
                    std::uint32_t character) override {
        Check(system == 0x7000u, "character system");
        last_key = character;
        events.emplace_back("char");
        return injection_result;
    }
    void EnableMouseCapture(std::uint32_t owner) override {
        Check(owner == 0x1000u, "capture owner");
        events.emplace_back("capture-on");
    }
    void DisableMouseCapture(std::uint32_t owner) override {
        Check(owner == 0x1000u, "release owner");
        events.emplace_back("capture-off");
    }
    std::array<float, 2> TransformMouseCoordinates(
        std::uint32_t owner,
        const std::array<float, 2>& point) override {
        Check(owner == 0x1000u, "mouse transform owner");
        mouse_input = point;
        events.emplace_back("transform");
        return {point[0] + 10.0f, point[1] + 20.0f};
    }
    bool InjectMousePosition(std::uint32_t system,
                             float x, float y) override {
        Check(system == 0x7000u, "mouse position system");
        mouse_injected = {x, y};
        events.emplace_back("mouse-position");
        return injection_result;
    }
    bool InjectMouseButtonDown(std::uint32_t system,
                               std::uint32_t button) override {
        Check(system == 0x7000u, "button down system");
        last_button = button;
        events.emplace_back("button-down");
        return injection_result;
    }
    bool InjectMouseButtonUp(std::uint32_t system,
                             std::uint32_t button) override {
        Check(system == 0x7000u, "button up system");
        last_button = button;
        events.emplace_back("button-up");
        return injection_result;
    }
    bool InjectMouseWheel(std::uint32_t system,
                          float delta) override {
        Check(system == 0x7000u, "wheel system");
        wheel = delta;
        events.emplace_back("wheel");
        return injection_result;
    }
    void RendererReadDisplaySize(std::uint32_t renderer) override {
        Check(renderer == 0x8000u, "renderer display-size query");
        events.emplace_back("renderer-size");
    }
    void RendererFireDisplaySizeChanged(
        std::uint32_t renderer) override {
        Check(renderer == 0x8000u, "renderer size event");
        events.emplace_back("renderer-event");
    }
    void RequestRedraw(std::uint32_t window) override {
        Check(window == 0x9000u, "redraw root window");
        events.emplace_back("redraw");
    }
};

bool Process(std::uint32_t message, std::uint32_t wparam,
             std::uint32_t lparam, Runtime& runtime) {
    return pal4::CeguiUiEventProcessor::Process(
        0x1000u, 0x2000u, message, wparam, lparam, runtime);
}

void KeyboardBranchesAndResultForwarding() {
    Runtime runtime;
    runtime.key_map[0x1bu] = 1u;
    Check(!Process(0x100u, 0x1bu, 0u, runtime) &&
              runtime.events == std::vector<std::string>{"map-key"},
          "key-down code one is suppressed");
    runtime.events.clear();
    Check(Process(0x101u, 0x1bu, 0u, runtime) &&
              runtime.events == std::vector<std::string>{
                  "map-key", "system", "key-down", "system", "key-up"} &&
              runtime.last_key == 1u,
          "key-up code one injects down immediately before up");
    for (const auto [mapped, expected] :
         {std::pair{17u, 200u}, std::pair{30u, 203u},
          std::pair{31u, 208u}, std::pair{32u, 205u},
          std::pair{57u, 28u}, std::pair{42u, 42u}}) {
        runtime.events.clear();
        Check(Process(0x100u, mapped, 0u, runtime) &&
                  runtime.last_key == expected &&
                  runtime.events == std::vector<std::string>{
                      "map-key", "system", "key-down"},
              "key-down remap and injection");
        runtime.events.clear();
        Check(Process(0x101u, mapped, 0u, runtime) &&
                  runtime.last_key == expected &&
                  runtime.events == std::vector<std::string>{
                      "map-key", "system", "key-up"},
              "key-up remap and injection");
    }
    runtime.injection_result = false;
    Check(!Process(0x100u, 42u, 0u, runtime),
          "injectKeyDown return is forwarded");
}

void CharacterAndMouseBranches() {
    Runtime runtime;
    const auto host_mouse = [&](std::uint32_t message, std::uint32_t wparam,
                                std::uint32_t lparam) {
        return pal4::CeguiUiEventProcessor::IsMouseMessage(message) &&
               Process(message, wparam, lparam, runtime);
    };
    Check(Process(0x102u, 0x5678u, 0u, runtime) &&
              runtime.last_key == 0x5678u &&
              runtime.events == std::vector<std::string>{"system", "char"},
          "WM_CHAR forwards raw character");
    runtime.events.clear();
    Check(Process(0x286u, 0x1234u, 0u, runtime) &&
              runtime.last_key == 0x4321u &&
              runtime.events == std::vector<std::string>{
                  "map-ime", "system", "char"},
          "IME character conversion precedes singleton lookup");
    runtime.events.clear();
    Check(host_mouse(0x200u, 0u, 0xffff0005u) &&
              runtime.mouse_input == std::array<float, 2>{5.0f, 65535.0f} &&
              runtime.mouse_injected ==
                  std::array<float, 2>{15.0f, 65555.0f} &&
              runtime.events == std::vector<std::string>{
                  "capture-on", "transform", "system", "mouse-position"},
          "mouse move uses unsigned words and transforms after capture");
    for (const auto [message, button, down] :
         {std::tuple{0x201u, 0u, true},
          std::tuple{0x202u, 0u, false},
          std::tuple{0x204u, 1u, true},
          std::tuple{0x205u, 1u, false},
          std::tuple{0x207u, 2u, true},
          std::tuple{0x208u, 2u, false}}) {
        runtime.events.clear();
        Check(host_mouse(message, 0u, 0u) &&
                  runtime.last_button == button &&
                  runtime.events == std::vector<std::string>{
                      "system", down ? "button-down" : "button-up"},
              "left, right and middle button message mapping");
    }
    runtime.events.clear();
    Check(host_mouse(0x20au, 0xff880000u, 0u) &&
              std::fabs(runtime.wheel + 1.0f) < 1e-6f &&
              runtime.events == std::vector<std::string>{"system", "wheel"},
          "signed high word becomes a negative wheel unit");
    runtime.events.clear();
    Check(!Process(0xa0u, 0u, 0u, runtime) &&
              !Process(0x2a3u, 0u, 0u, runtime) &&
              runtime.events == std::vector<std::string>{
                  "capture-off", "capture-off"},
          "nonclient move and mouse leave disable capture");
}

void SizeAndActivationEvents() {
    Runtime runtime;
    runtime.system_pointer = 0u;
    Check(!Process(5u, 0u, 0u, runtime) &&
              runtime.events == std::vector<std::string>{"system-ptr"},
          "size event without CEGUI System exits early");
    runtime.system_pointer = 0x7000u;
    runtime.events.clear();
    Check(!Process(5u, 0u, 0u, runtime) &&
              runtime.events == std::vector<std::string>{
                  "system-ptr", "system", "read:28700",
                  "renderer-size", "renderer-event"},
          "size event queries renderer and fires display notification");
    runtime.events.clear();
    Check(!Process(6u, 0u, 0u, runtime) && runtime.events.empty(),
          "inactive window skips all CEGUI calls");
    Check(!Process(6u, 1u, 0u, runtime) &&
              runtime.events == std::vector<std::string>{
                  "system-ptr", "system", "read:28700",
                  "renderer-size", "renderer-event", "system-ptr",
                  "read:28720", "redraw"},
          "activation fires size event then requests root redraw");
    runtime.words[0x701cu] = 0u;
    runtime.events.clear();
    Check(!Process(6u, 1u, 0u, runtime) &&
              runtime.events == std::vector<std::string>{
                  "system-ptr", "system", "read:28700",
                  "system-ptr", "read:28720", "redraw"},
          "activation still redraws when renderer is absent");
    runtime.events.clear();
    Check(!Process(0x1234u, 0u, 0u, runtime) && runtime.events.empty(),
          "unrecognized Windows message is ignored");
    pal4::CeguiUiEventRuntimeStub stub;
    Check(!pal4::CeguiUiEventProcessor::Process(
              0u, 0u, 5u, 0u, 0u, stub),
          "linkable UI event stub handles absent CEGUI System");
}

}  // namespace

int main() {
    KeyboardBranchesAndResultForwarding();
    CharacterAndMouseBranches();
    SizeAndActivationEvents();
    std::cout << "3 CEGUI UI event test cases passed\n";
}
