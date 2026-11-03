#include "pal4/frame_toolbar.h"
#include "pal4/frame_toolbar_registration.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct RegistrationRuntime final : pal4::IFrameToolbarRegistrationRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {0x8b1398u, 0x8b1400u}};
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x2000u;
    std::uint32_t allocation_size = 0u;
    std::uint32_t freed = 0u;
    std::uint32_t layout_name = 0u;
    std::uint32_t registered_name = 0u;
    std::uint32_t factory = 0u;
    std::uint32_t tree = 0u;
    std::uint32_t key = 0u;
    std::uint32_t handler = 0u;
    std::uint32_t destroyed = 0u;
    bool throw_construct = false;
    bool throw_insert = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write");
        words[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.emplace_back("allocate");
        allocation_size = bytes;
        return allocation;
    }
    void Free(std::uint32_t address) override {
        calls.emplace_back("free");
        freed = address;
    }
    std::uint32_t Construct(std::uint32_t object,
                            std::uint32_t name) override {
        calls.emplace_back("construct");
        Check(object == 0x2000u, "constructor object");
        layout_name = name;
        if (throw_construct) throw std::runtime_error("construct");
        return 0x2004u;
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x3000u;
    }
    std::uint32_t RegisterFactory(std::uint32_t manager,
                                   std::uint32_t name,
                                   std::uint32_t creator) override {
        calls.emplace_back("register-factory");
        Check(manager == 0x3000u, "manager address");
        registered_name = name;
        factory = creator;
        return 0x55u;
    }
    void InsertOrReplace(std::uint32_t target,
                         std::uint32_t event_key,
                         std::uint32_t value) override {
        calls.emplace_back("insert");
        tree = target;
        key = event_key;
        handler = value;
        if (throw_insert) throw std::runtime_error("insert");
    }
    void DestroyHandler(std::uint32_t value,
                        std::uint32_t flags) override {
        calls.emplace_back("destroy-handler");
        Check(flags == 1u, "deleting destructor flag");
        destroyed = value;
    }
};

struct ToolbarRuntime final : pal4::IFrameToolbarRuntime {
    std::vector<std::string> calls;
    std::unordered_map<std::uint32_t, std::uint32_t> words{{0x7050u, 0x8100u}};
    std::string requested_name;
    std::uint32_t subscribed_button = 0u;
    std::uint32_t subscribed_callback = 0u;
    std::uint32_t released_connection = 0u;
    std::uint32_t released_names = 0u;
    std::uint32_t parent = 0u;
    std::uint32_t nested = 0u;
    std::uint32_t event_tree = 0u;
    std::uint32_t event_key = 0u;
    std::uint32_t event_first = 1u;
    std::uint32_t event_second = 1u;
    bool parent_visible = false;
    bool nested_visible = false;
    bool enabled = true;
    bool null_callback = false;
    bool throw_first_layout = false;
    bool throw_nested_layout = false;
    bool throw_initialize = false;
    bool throw_nested_destroy = false;

    void LoadLayout(std::uint32_t object,
                    std::string_view name) override {
        calls.emplace_back(object == 0x7000u ? "load-base" : "load-nested");
        Check((object == 0x7000u && name == "frameToolbar") ||
                  (object == 0x702cu && name == "decorator"),
              "layout object and name");
        if ((object == 0x7000u && throw_first_layout) ||
            (object == 0x702cu && throw_nested_layout))
            throw std::runtime_error("layout");
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write");
        words[address] = value;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    std::string_view WindowName(std::uint32_t object) override {
        calls.emplace_back("name");
        Check(object == 0x7000u, "name owner");
        return "frameToolbar/";
    }
    std::uint32_t GetWindow(std::uint32_t object,
                            std::string_view name) override {
        calls.emplace_back("get-window");
        Check(object == 0x7000u, "button lookup owner");
        requested_name = name;
        return 0x8000u;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.emplace_back("allocate");
        Check(bytes == 0xcu, "close callback allocation size");
        return null_callback ? 0u : 0x9000u;
    }
    std::uint32_t SubscribeClicked(std::uint32_t button,
                                   std::uint32_t callback) override {
        calls.emplace_back("subscribe");
        subscribed_button = button;
        subscribed_callback = callback;
        return 0xa000u;
    }
    void ReleaseConnection(std::uint32_t connection) override {
        calls.emplace_back("release-connection");
        released_connection = connection;
    }
    void SetEnabled(std::uint32_t window, bool next) override {
        calls.emplace_back("enabled");
        Check(window == 0x8100u, "nested root window");
        enabled = next;
        if (throw_initialize) throw std::runtime_error("initialize");
    }
    std::uint32_t ReleaseNameBuffer() override {
        calls.emplace_back("release-name");
        ++released_names;
        return 0x55u;
    }
    std::uint32_t DestroyBase(std::uint32_t object) override {
        calls.emplace_back(object == 0x702cu ? "destroy-nested" : "destroy-base");
        if (object == 0x702cu && throw_nested_destroy)
            throw std::runtime_error("nested destruction");
        return object == 0x7000u ? 0x55u : 0x66u;
    }
    void SetParent(std::uint32_t object, bool visible,
                   std::uint32_t target) override {
        calls.emplace_back("set-parent");
        Check(object == 0x7000u, "parent owner");
        parent_visible = visible;
        parent = target;
    }
    std::uint32_t SetNestedVisible(std::uint32_t object, bool visible,
                                   std::uint32_t target) override {
        calls.emplace_back("set-nested-visible");
        Check(target == 0u, "nested parent argument");
        nested = object;
        nested_visible = visible;
        return 0x77u;
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x4000u;
    }
    void ProcessUiEvent(std::uint32_t tree, std::uint32_t key,
                        std::uint32_t first,
                        std::uint32_t second) override {
        calls.emplace_back("event");
        event_tree = tree;
        event_key = key;
        event_first = first;
        event_second = second;
    }
};

void CheckRegistration() {
    using Registration = pal4::FrameToolbarRegistration;
    RegistrationRuntime runtime;
    Check(Registration::InitializeNameThunk(runtime) == 0x8b1400u &&
              runtime.words.at(0x8eacb4u) == 0x8b1400u &&
              runtime.calls == (std::vector<std::string>{"read", "write"}),
          "frameToolbar name pointer");
    runtime.calls.clear();
    Check(Registration::RegisterFactoryThunk(runtime) == 0x55u &&
              runtime.registered_name == 0x8b1400u &&
              runtime.factory == 0x4a5ff0u &&
              runtime.calls == (std::vector<std::string>{
                  "manager", "read", "register-factory"}),
          "factory registration");
    runtime.calls.clear();
    Check(Registration::Create(0x9000u, runtime) == 0x2004u &&
              runtime.allocation_size == 0x54u &&
              runtime.layout_name == 0x9000u &&
              runtime.calls == (std::vector<std::string>{
                  "allocate", "construct"}),
          "toolbar factory uses 0x54-byte object");
    runtime.calls.clear();
    Check(Registration::RegisterHandlerThunk(runtime) == 0x8eacb0u &&
              runtime.allocation_size == 8u &&
              runtime.words.at(0x2000u) == 0x843494u &&
              runtime.words.at(0x2004u) == 0x8b1400u &&
              runtime.tree == 0x3104u && runtime.key == 0x8b1394u &&
              runtime.handler == 0x2000u && runtime.destroyed == 0x2000u &&
              runtime.calls == (std::vector<std::string>{
                  "read", "allocate", "write", "write", "manager",
                  "insert", "destroy-handler"}),
          "show_frame_tool handler and ownership");
}

void CheckRegistrationFailures() {
    using Registration = pal4::FrameToolbarRegistration;
    RegistrationRuntime empty;
    empty.allocation = 0u;
    Check(Registration::Create(0x9000u, empty) == 0u &&
              empty.layout_name == 0u &&
              Registration::RegisterHandler(empty) == 0x8eacb0u &&
              empty.handler == 0u && empty.destroyed == 0u,
          "null allocation branches");
    RegistrationRuntime construct_failure;
    construct_failure.throw_construct = true;
    bool caught = false;
    try { (void)Registration::Create(0x9000u, construct_failure); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && construct_failure.freed == 0x2000u,
          "factory exception releases object");
    RegistrationRuntime insertion_failure;
    insertion_failure.throw_insert = true;
    caught = false;
    try { (void)Registration::RegisterHandler(insertion_failure); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && insertion_failure.destroyed == 0x2000u,
          "insertion exception releases temporary handler");
}

void CheckToolbarLifecycle() {
    ToolbarRuntime runtime;
    Check(pal4::FrameToolbar::Construct(0x7000u, "frameToolbar",
                                         runtime) == 0x7000u &&
              runtime.words.at(0x7000u) == pal4::FrameToolbar::kVtable &&
              runtime.words.at(0x7028u) == 0x8000u &&
              runtime.words.at(0x9004u) == 0x4a63c0u &&
              runtime.words.at(0x9008u) == 0x7000u &&
              runtime.words.at(0x9000u) ==
                  pal4::FrameToolbar::kCloseCallbackVtable &&
              runtime.requested_name == "frameToolbar/btnClose" &&
              runtime.subscribed_button == 0x8000u &&
              runtime.subscribed_callback == 0x9000u &&
              runtime.released_connection == 0xa000u &&
              runtime.released_names == 1u && !runtime.enabled &&
              runtime.calls == (std::vector<std::string>{
                  "load-base", "load-nested", "write", "name",
                  "get-window", "write", "allocate", "write", "write",
                  "write", "subscribe", "release-connection", "read",
                  "enabled", "release-name"}),
          "constructor composes close-button initialization");
    runtime.calls.clear();
    Check(pal4::FrameToolbar::Destroy(0x7000u, runtime) == 0x55u &&
              runtime.calls == (std::vector<std::string>{
                  "destroy-nested", "destroy-base"}),
          "destructor releases nested then base layout");
}

void CheckToolbarControlsAndEvents() {
    ToolbarRuntime empty;
    empty.null_callback = true;
    Check(pal4::FrameToolbar::InitializeControls(0x7000u, empty) ==
              0x55u && empty.subscribed_callback == 0u &&
              empty.released_connection == 0xa000u &&
              empty.released_names == 1u && !empty.enabled,
          "null callback allocation still subscribes and cleans up");

    ToolbarRuntime runtime;
    Check(pal4::FrameToolbar::SetVisible(0x7000u, true,
                                          0x9000u, runtime) == 0x77u &&
              runtime.parent_visible && runtime.parent == 0x9000u &&
              runtime.nested == 0x702cu && runtime.nested_visible &&
              runtime.calls == (std::vector<std::string>{
                  "set-parent", "set-nested-visible"}),
          "visibility forwards to nested decorator after parent");
    runtime.calls.clear();
    Check(pal4::FrameToolbar::SetVisible(0x7000u, false,
                                          0u, runtime) == 0x77u &&
              !runtime.parent_visible && !runtime.nested_visible,
          "hide forwarding");
    runtime.calls.clear();
    Check(pal4::FrameToolbar::OnCloseButtonClick(runtime) == 1u &&
              runtime.event_tree == 0x4104u &&
              runtime.event_key == pal4::FrameToolbar::kCloseEventKey &&
              runtime.event_first == 0u &&
              runtime.event_second == 0u &&
              runtime.calls == (std::vector<std::string>{
                  "manager", "event"}),
          "close button sends close_system_ui with two zero words");
}

void CheckToolbarFailures() {
    ToolbarRuntime first;
    first.throw_first_layout = true;
    bool caught = false;
    try { (void)pal4::FrameToolbar::Construct(0x7000u, "frameToolbar", first); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && first.calls ==
              (std::vector<std::string>{"load-base"}),
          "first layout failure has no constructed member to release");
    ToolbarRuntime nested;
    nested.throw_nested_layout = true;
    caught = false;
    try { (void)pal4::FrameToolbar::Construct(0x7000u, "frameToolbar", nested); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && nested.calls == (std::vector<std::string>{
              "load-base", "load-nested", "destroy-base"}),
          "nested layout failure releases base");
    ToolbarRuntime initialize;
    initialize.throw_initialize = true;
    caught = false;
    try { (void)pal4::FrameToolbar::Construct(0x7000u, "frameToolbar", initialize); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && initialize.released_names == 1u &&
              initialize.calls[initialize.calls.size() - 3u] ==
                  "release-name" &&
              initialize.calls[initialize.calls.size() - 2u] ==
                  "destroy-nested" &&
              initialize.calls.back() == "destroy-base",
          "decorator failure releases both constructed layouts");
    ToolbarRuntime destroy;
    destroy.throw_nested_destroy = true;
    caught = false;
    try { (void)pal4::FrameToolbar::Destroy(0x7000u, destroy); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && destroy.calls == (std::vector<std::string>{
              "destroy-nested", "destroy-base"}),
          "nested destructor failure still releases base");
}

}  // namespace

int main() {
    try {
        CheckRegistration();
        CheckRegistrationFailures();
        CheckToolbarLifecycle();
        CheckToolbarControlsAndEvents();
        CheckToolbarFailures();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
