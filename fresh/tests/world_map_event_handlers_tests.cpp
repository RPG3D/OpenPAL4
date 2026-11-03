#include "pal4/world_map_event_handlers.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace {

void Check(const bool value, const char* message) {
    if (!value) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IWorldMapEventRuntime {
    static constexpr std::uint32_t kFrame = 0x5000u;
    std::map<std::uint32_t, std::uint32_t> words{
        {kFrame + 0x18u, 0x8000u}, {kFrame + 0x24u, 0x9000u},
        {0x8411b8u, 0xe001u}, {0x84103cu, 0xe002u}};
    std::vector<std::string> calls;
    std::vector<std::tuple<std::uint32_t, std::uint32_t, std::uint32_t>>
        subscriptions;
    std::vector<std::uint32_t> mouse_windows;
    std::vector<std::uint32_t> released;
    std::uint32_t button_number = 0u;
    std::uint32_t allocations = 0u;
    std::uint32_t fail_allocation = 0xffffffffu;
    std::uint32_t fail_connection = 0xffffffffu;
    std::uint32_t fail_format = 0u;
    bool role_name = false;

    std::uint32_t BeginName() override {
        calls.emplace_back("begin");
        return 0x8100u;
    }
    void FormatButtonName(const std::uint32_t buffer,
                          const std::uint32_t prefix,
                          const std::uint32_t number) override {
        Check(buffer == 0x8100u && prefix == 0x8000u &&
                  number >= 1u && number <= 21u,
              "button format arguments");
        calls.emplace_back("format button");
        if (number == fail_format) throw std::runtime_error("format");
        button_number = number;
        role_name = false;
    }
    void FormatRoleName(const std::uint32_t buffer,
                        const std::uint32_t prefix) override {
        Check(buffer == 0x8100u && prefix == 0x8000u,
              "role format arguments");
        calls.emplace_back("format role");
        role_name = true;
    }
    std::uint32_t FindWindow(const std::uint32_t frame,
                             const std::uint32_t buffer) override {
        Check(frame == kFrame && buffer == 0x8100u,
              "window lookup arguments");
        calls.emplace_back("find");
        return role_name ? 0x7000u : 0x6000u + button_number - 1u;
    }
    std::uint32_t EndName(const std::uint32_t buffer) override {
        Check(buffer == 0x8100u, "temporary name cleanup");
        calls.emplace_back("end");
        return 0x1234u;
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        calls.emplace_back("write");
        words[address] = value;
    }
    std::uint32_t Allocate(const std::uint32_t bytes) override {
        Check(bytes == 24u, "event callback allocation size");
        calls.emplace_back("allocate");
        const auto index = allocations++;
        return index == fail_allocation ? 0u : 0xa000u + index * 0x20u;
    }
    std::uint32_t Subscribe(const std::uint32_t window,
                            const std::uint32_t event,
                            const std::uint32_t callback) override {
        calls.emplace_back("subscribe");
        subscriptions.emplace_back(window, event, callback);
        const auto index = static_cast<std::uint32_t>(
            subscriptions.size() - 1u);
        return index == fail_connection ? 0u : 0xc000u + index;
    }
    void ReleaseConnection(const std::uint32_t connection) override {
        calls.emplace_back("release");
        released.push_back(connection);
    }
    void SetEnabled(const std::uint32_t window,
                    const bool enabled) override {
        Check(window == 0x7000u && !enabled,
              "role image disabled");
        calls.emplace_back("disable role");
    }
    void SetupMouseHandlers(const std::uint32_t mouse,
                            const std::uint32_t window) override {
        Check(mouse == kFrame + 0x28u, "mouse helper address");
        calls.emplace_back("mouse");
        mouse_windows.push_back(window);
    }
};

struct MouseRuntime final : pal4::IWorldMapMouseDispatchRuntime {
    std::vector<std::uint32_t> reads;
    std::uint32_t ReadWord(const std::uint32_t address) override {
        reads.push_back(address);
        if (address == Runtime::kFrame + 0x28u) return 0x843c80u;
        Check(address == 0x843c80u, "mouse vtable address");
        return 0x484e00u;
    }
    std::uint32_t CallVirtual(const std::uint32_t object,
                              const std::uint32_t function) override {
        Check(object == Runtime::kFrame + 0x28u &&
                  function == 0x484e00u,
              "mouse slot-zero tail call");
        return 0x9876u;
    }
};

void TestAllBindings() {
    Runtime runtime;
    Check(pal4::WorldMapEventHandlers::Initialize(Runtime::kFrame,
                                                   runtime) == 0x1234u,
          "event initializer returns name cleanup result");
    Check(runtime.allocations == 43u &&
              runtime.subscriptions.size() == 43u &&
              runtime.released.size() == 43u &&
              runtime.calls.front() == "begin" &&
              runtime.calls.back() == "end",
          "42 click callbacks, one key callback and cleanup");
    for (std::uint32_t i = 0u; i < 21u; ++i) {
        const auto window = 0x6000u + i;
        Check(runtime.words.at(Runtime::kFrame + 0x40u + i * 4u) == window &&
                  runtime.words.at(window + 0x118u) == i,
              "button window field and zero-based index");
        for (std::uint32_t duplicate = 0u; duplicate < 2u; ++duplicate) {
            const auto callback_index = i * 2u + duplicate;
            const auto callback = 0xa000u + callback_index * 0x20u;
            Check(runtime.subscriptions[callback_index] ==
                      std::tuple{window, 0xe001u, callback} &&
                      runtime.words.at(callback) == 0x843cc0u &&
                      runtime.words.at(callback + 8u) == 0x484a20u &&
                      runtime.words.at(callback + 12u) == 0u &&
                      runtime.words.at(callback + 16u) == Runtime::kFrame &&
                      runtime.released[callback_index] ==
                          0xc000u + callback_index,
                  "duplicate click subscription and callback layout");
        }
    }
    Check(runtime.words.at(Runtime::kFrame + 0x238u) == 0x7000u &&
              runtime.subscriptions.back() ==
                  std::tuple{0x9000u, 0xe002u, 0xa540u} &&
              runtime.words.at(0xa540u + 8u) == 0x484bd0u,
          "role window and root key callback");
    constexpr std::array<std::uint32_t, 21> mouse_order{
        14u, 1u, 15u, 2u, 5u, 6u, 7u, 8u, 10u, 19u, 16u,
        11u, 18u, 0u, 20u, 12u, 3u, 4u, 17u, 13u, 9u};
    Check(runtime.mouse_windows.size() == mouse_order.size(),
          "all mouse bindings present");
    for (std::size_t i = 0u; i < mouse_order.size(); ++i) {
        Check(runtime.mouse_windows[i] == 0x6000u + mouse_order[i],
              "mouse registration order");
    }
}

void TestNullHandles() {
    Runtime runtime;
    runtime.fail_allocation = 1u;
    runtime.fail_connection = 2u;
    (void)pal4::WorldMapEventHandlers::Initialize(Runtime::kFrame,
                                                   runtime);
    Check(std::get<2>(runtime.subscriptions[1]) == 0u &&
              runtime.released.size() == 42u &&
              runtime.released[2] == 0xc003u,
          "null callback still subscribes; null connection skips release");
}

void TestFormatFailure() {
    Runtime runtime;
    runtime.fail_format = 3u;
    bool caught = false;
    try {
        (void)pal4::WorldMapEventHandlers::Initialize(Runtime::kFrame,
                                                       runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.calls.back() == "end" &&
              runtime.subscriptions.size() == 4u,
          "format exception releases temporary name");
}

void TestMouseForwarding() {
    MouseRuntime runtime;
    Check(pal4::WorldMapEventHandlers::ForwardMouseSlot0(
              Runtime::kFrame, runtime) == 0x9876u &&
              runtime.reads == std::vector<std::uint32_t>{
                  Runtime::kFrame + 0x28u, 0x843c80u},
          "key callback forwards mouse member vtable slot zero");
}

}  // namespace

int main() {
    TestAllBindings();
    TestNullHandles();
    TestFormatFailure();
    TestMouseForwarding();
    return EXIT_SUCCESS;
}
