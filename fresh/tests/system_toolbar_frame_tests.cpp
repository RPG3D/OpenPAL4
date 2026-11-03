#include "pal4/system_toolbar_frame.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ISystemToolbarFrameRuntime {
    std::map<std::uint32_t, std::uint32_t> words{{0x5024u, 0x6000u}};
    std::vector<std::string> calls;
    std::vector<std::string> found_names;
    std::vector<std::uint32_t> mouse_windows;
    std::uint32_t allocation_count = 0u;
    std::uint32_t connection_count = 0u;
    std::uint32_t release_count = 0u;
    std::uint32_t null_allocation_index = 99u;
    std::uint32_t null_connection_index = 99u;
    bool fail_layout = false;
    bool fail_helper = false;
    bool fail_subscribe = false;

    void LoadLayout(const std::uint32_t frame,
                    const std::uint32_t name) override {
        Check(frame == 0x5000u && name == 0x7000u,
              "layout constructor arguments");
        calls.emplace_back("layout");
        if (fail_layout) throw std::runtime_error("layout");
    }
    void ConstructMouseHelper(const std::uint32_t helper) override {
        Check(helper == 0x5028u, "mouse helper offset");
        calls.emplace_back("helper");
        if (fail_helper) throw std::runtime_error("helper");
    }
    void DestroyMouseHelper(const std::uint32_t helper) override {
        Check(helper == 0x5028u, "mouse helper cleanup target");
        calls.emplace_back("destroy-helper");
    }
    void DestroyBase(const std::uint32_t frame) override {
        Check(frame == 0x5000u, "base cleanup target");
        calls.emplace_back("destroy-base");
    }
    std::uint32_t FindGlobalWindow(const std::string_view name) override {
        calls.emplace_back("find-global");
        found_names.emplace_back(name);
        return 0x8000u + static_cast<std::uint32_t>(found_names.size());
    }
    std::uint32_t FindChildWindow(const std::uint32_t frame,
                                  const std::string_view name) override {
        Check(frame == 0x5000u, "child lookup frame");
        calls.emplace_back("find-child");
        found_names.emplace_back(name);
        return 0x8000u + static_cast<std::uint32_t>(found_names.size());
    }
    std::uint32_t Allocate(const std::uint32_t bytes) override {
        Check(bytes == 24u, "click callback allocation size");
        calls.emplace_back("allocate");
        ++allocation_count;
        if (allocation_count == null_allocation_index) return 0u;
        return 0x9000u + allocation_count * 0x20u;
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.emplace_back("read");
        if (address == pal4::SystemToolbarFrame::kClickedEvent) {
            return 0xa000u;
        }
        return words.at(address);
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        calls.emplace_back("write");
        words[address] = value;
    }
    std::uint32_t Subscribe(const std::uint32_t window,
                            const std::uint32_t event,
                            const std::uint32_t callback) override {
        Check(window == 0x8000u + ++connection_count && event == 0xa000u,
              "click subscription window and event");
        Check(callback == (connection_count == null_allocation_index ? 0u :
               0x9000u + connection_count * 0x20u),
              "click subscription callback");
        calls.emplace_back("subscribe");
        if (fail_subscribe) throw std::runtime_error("subscribe");
        return connection_count == null_connection_index ? 0u :
            0xb000u + connection_count;
    }
    void ReleaseConnection(const std::uint32_t connection) override {
        Check(connection >= 0xb001u && connection <= 0xb007u,
              "connection release target");
        calls.emplace_back("release");
        ++release_count;
    }
    void SetZOrderingEnabled(const std::uint32_t window,
                             const bool enabled) override {
        Check(window == 0x6000u && !enabled, "root z ordering disabled");
        calls.emplace_back("z-order");
    }
    void SetupMouseHandlers(const std::uint32_t helper,
                            const std::uint32_t window) override {
        Check(helper == 0x5028u, "mouse helper receiver");
        calls.emplace_back("mouse");
        mouse_windows.push_back(window);
    }
};

void TestConstructor() {
    Runtime runtime;
    Check(pal4::SystemToolbarFrame::Construct(
              0x5000u, 0x7000u, runtime) == 0x5000u,
          "constructor returns frame");
    Check(runtime.calls[0] == "layout" && runtime.calls[1] == "helper" &&
              runtime.words.at(0x5000u) == 0x843c44u &&
              runtime.words.at(0x5028u) == 0x843c1cu,
          "base/helper initialization precedes controls");
    Check(runtime.found_names == std::vector<std::string>{
              "sysToolBar/Property", "sysToolBar/Magic",
              "sysToolBar/Role", "sysToolBar/Equipment",
              "sysToolBar/Task", "sysToolBar/System",
              "sysToolBar/Forging"},
          "all seven toolbar controls in IDA order");
    Check(std::count(runtime.calls.begin(), runtime.calls.end(),
                     "find-global") == 2 &&
              std::count(runtime.calls.begin(), runtime.calls.end(),
                         "find-child") == 5,
          "property and magic use global lookup; others use dialog lookup");
    Check(runtime.allocation_count == 7u && runtime.connection_count == 7u &&
              runtime.release_count == 7u,
          "seven callbacks subscribed and connections released");
    for (std::uint32_t index = 0u; index < 7u; ++index) {
        Check(runtime.words.at(0x5040u + 4u * index) == 0x8001u + index,
              "window stored at toolbar field");
        const auto callback = 0x9020u + index * 0x20u;
        Check(runtime.words.at(callback + 8u) == 0x483090u &&
                  runtime.words.at(callback + 12u) == 0u &&
                  runtime.words.at(callback + 16u) == 0x5000u &&
                  runtime.words.at(callback) == 0x843c14u,
              "callback payload and vtable");
    }
    Check(runtime.mouse_windows == std::vector<std::uint32_t>{
              0x8003u, 0x8001u, 0x8004u, 0x8002u,
              0x8007u, 0x8005u, 0x8006u},
          "mouse hookup order");
    Check(runtime.calls[runtime.calls.size() - 15u] == "z-order" &&
              runtime.calls.back() == "mouse",
          "z ordering updated before mouse hookups");
}

void TestNullHandles() {
    Runtime runtime;
    runtime.null_allocation_index = 3u;
    runtime.null_connection_index = 5u;
    Check(pal4::SystemToolbarFrame::Construct(
              0x5000u, 0x7000u, runtime) == 0x5000u &&
              runtime.allocation_count == 7u &&
              runtime.connection_count == 7u &&
              runtime.release_count == 6u &&
              runtime.words.find(0x9060u) == runtime.words.end(),
          "null callback and connection preserve remaining setup");
}

void TestUnwind() {
    Runtime runtime;
    runtime.fail_layout = true;
    bool caught = false;
    try {
        (void)pal4::SystemToolbarFrame::Construct(0x5000u, 0x7000u, runtime);
    } catch (const std::runtime_error&) { caught = true; }
    Check(caught && runtime.calls == std::vector<std::string>{"layout"},
          "layout failure leaves cleanup to layout loader");

    runtime = Runtime{};
    runtime.fail_helper = true;
    caught = false;
    try {
        (void)pal4::SystemToolbarFrame::Construct(0x5000u, 0x7000u, runtime);
    } catch (const std::runtime_error&) { caught = true; }
    Check(caught && runtime.calls == std::vector<std::string>{
              "layout", "helper", "destroy-base"},
          "helper failure destroys only completed base");

    runtime = Runtime{};
    runtime.fail_subscribe = true;
    caught = false;
    try {
        (void)pal4::SystemToolbarFrame::Construct(0x5000u, 0x7000u, runtime);
    } catch (const std::runtime_error&) { caught = true; }
    Check(caught && runtime.calls[runtime.calls.size() - 2u] ==
              "destroy-helper" && runtime.calls.back() == "destroy-base",
          "subscription failure unwinds helper and base");
}

}  // namespace

int main() {
    TestConstructor();
    TestNullHandles();
    TestUnwind();
    return EXIT_SUCCESS;
}
