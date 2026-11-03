#include "pal4/property_window_lifecycle.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IPropertyWindowLifecycleRuntime {
    struct Subscription {
        std::uint32_t window;
        pal4::PropertyWindowEvent event;
        std::uint32_t handler;
        std::uint32_t owner;
        std::uint32_t vtable;
    };
    std::vector<std::string> calls;
    std::vector<std::string> paths;
    std::vector<Subscription> subscriptions;
    std::vector<std::uint32_t> mouse_buttons;
    std::vector<std::uint32_t> image_values;
    std::uint32_t columns = 2u;
    bool fail_name_node{};
    bool fail_lookup{};
    bool fail_image{};
    pal4::PropertyWindowInitStage cleanup_stage{};
    unsigned cleanup_count{};

    void LoadLayout(pal4::PropertyWindowState&, std::uint32_t name) override {
        Check(name == 0x7000u, "layout receives supplied name");
        calls.emplace_back("layout");
    }
    void InitializeMember(pal4::PropertyWindowState&) override {
        calls.emplace_back("member");
    }
    void InitializeNameNode(pal4::PropertyWindowState& state) override {
        Check(state.Read(40u) == 0x843628u &&
                  state.Read(116u) == 0xffffffffu &&
                  state.Read(120u) == 0u && state.Read(124u) == 0u &&
                  state.Read(128u) == 0u,
              "member fields precede name-node construction");
        calls.emplace_back("name-node");
        if (fail_name_node) throw std::runtime_error("node");
    }
    void InitializeUiObject(pal4::PropertyWindowState&) override {
        calls.emplace_back("ui-object");
    }
    void Cleanup(pal4::PropertyWindowState&,
                 pal4::PropertyWindowInitStage stage) override {
        cleanup_stage = stage;
        ++cleanup_count;
        calls.emplace_back("cleanup");
    }
    std::uint32_t ObjectAddress(
        const pal4::PropertyWindowState& state) override {
        Check(state.Read(0u) == 0x843a2cu &&
                  state.Read(40u) == 0x843a44u,
              "final vtables precede owner access");
        calls.emplace_back("owner");
        return 0x4000u;
    }
    void RegisterCommand(std::string_view command,
                         std::uint32_t handler,
                         std::uint32_t owner,
                         std::uint32_t vtable) override {
        Check(owner == 0x4000u && vtable == 0x843a20u,
              "command callback has owner and vtable");
        calls.push_back("command:" + std::string(command) + ":" +
                        std::to_string(handler));
    }
    std::uint32_t FindWindow(std::string_view path) override {
        paths.emplace_back(path);
        calls.push_back("find:" + std::string(path));
        if (fail_lookup) throw std::runtime_error("lookup");
        return 0x2000u + static_cast<std::uint32_t>(paths.size() * 4u);
    }
    void Subscribe(std::uint32_t window, pal4::PropertyWindowEvent event,
                   std::uint32_t handler, std::uint32_t owner,
                   std::uint32_t vtable) override {
        subscriptions.push_back({window, event, handler, owner, vtable});
        calls.emplace_back("subscribe");
    }
    void SetButtonTag(std::uint32_t button,
                      std::uint32_t tag) override {
        calls.push_back("tag:" + std::to_string(button) + ":" +
                        std::to_string(tag));
    }
    void SetVisible(std::uint32_t, bool visible) override {
        Check(!visible, "arrow buttons start hidden");
        calls.emplace_back("hide");
    }
    void SetEnabled(std::uint32_t, bool enabled) override {
        Check(!enabled, "description and prop start disabled");
        calls.emplace_back("disable");
    }
    std::uint32_t ColumnCount(std::uint32_t list) override {
        Check(list == 0x2004u, "list operations use PropertyList");
        calls.emplace_back("column-count");
        return columns;
    }
    void RemoveColumn(std::uint32_t list,
                      std::uint32_t column) override {
        Check(list == 0x2004u && column == 0u && columns != 0u,
              "existing columns removed from index zero");
        --columns;
        calls.emplace_back("remove-column");
    }
    void AddColumn(std::uint32_t list, std::string_view title,
                   std::uint32_t id, float width) override {
        Check(list == 0x2004u && title.empty() &&
                  ((id == 0u && width == 0.5f) ||
                   (id == 1u && width == 0.40000001f)),
              "two empty list columns have observed widths");
        calls.push_back("add-column:" + std::to_string(id));
    }
    void SetColumnLabelImage(std::uint32_t list, std::uint32_t id,
                             std::string_view imageset,
                             std::string_view image) override {
        Check(list == 0x2004u && imageset == "zi0" &&
                  ((id == 0u && image == "mingcheng") ||
                   (id == 1u && image == "shuliang")),
              "column labels use zi0 images");
        calls.push_back("column-image:" + std::to_string(id));
    }
    void ConfigureList(std::uint32_t list) override {
        Check(list == 0x2004u, "list configuration uses PropertyList");
        calls.emplace_back("configure-list");
    }
    std::uint32_t Image(std::string_view imageset,
                        std::string_view image) override {
        Check(imageset == "zi0", "category images use zi0 imageset");
        calls.push_back("image:" + std::string(image));
        if (fail_image) throw std::runtime_error("image");
        const auto value = 0x6000u +
            static_cast<std::uint32_t>(image_values.size() * 4u);
        image_values.push_back(value);
        return value;
    }
    void SetupMouseHandlers(pal4::PropertyWindowState&,
                            std::uint32_t button) override {
        mouse_buttons.push_back(button);
        calls.emplace_back("mouse");
    }
    void ResetNameNode(pal4::PropertyWindowState&) override {
        calls.emplace_back("reset-node");
    }
    void FreeMemberBuffer(std::uint32_t address) override {
        Check(address == 0xdeadbeefu,
              "destructor frees member buffer at +0x30");
        calls.emplace_back("free-buffer");
    }
    std::uint32_t DestroyDialogBase(
        pal4::PropertyWindowState& state) override {
        Check(state.Read(48u) == 0u && state.Read(52u) == 0u &&
                  state.Read(56u) == 0u,
              "member buffer fields clear before base destructor");
        calls.emplace_back("destroy-base");
        return 0x9876u;
    }
};

std::size_t Position(const std::vector<std::string>& calls,
                     std::string_view value) {
    const auto it = std::find(calls.begin(), calls.end(), value);
    Check(it != calls.end(), "expected call exists");
    return static_cast<std::size_t>(it - calls.begin());
}

void TestConstruct() {
    pal4::PropertyWindowState state;
    Runtime runtime;
    Check(&pal4::PropertyWindowLifecycle::Construct(
              state, 0x7000u, runtime) == &state,
          "constructor returns original object");
    Check(runtime.cleanup_count == 0u && state.Read(180u) == 0x4000u &&
              state.Read(40u) == 0x843a44u,
          "successful constructor keeps final owner and member vtable");
    Check(runtime.calls[0] == "layout" &&
              runtime.calls[1] == "member" &&
              runtime.calls[2] == "name-node" &&
              runtime.calls[3] == "ui-object" &&
              runtime.calls[4] == "owner",
          "base and member initialization precede bindings");
    Check(runtime.calls[5] == "command:use_property:4665120" &&
              runtime.calls[6] == "command:discard_property:4665616" &&
              runtime.calls[7] == "command:after_use_property:4666144",
          "three commands register in IDA order with observed handlers");
    Check(runtime.paths.size() == 16u &&
              runtime.paths[0] == "PropertyWindow/PropertyList" &&
              runtime.paths[7] == "PropertyWindow/PropertyWindow3" &&
              runtime.paths[8] == "PropertyWindow/NewProp" &&
              runtime.paths[13] == "PropertyWindow/PropScenario" &&
              runtime.paths[15] ==
                  "PropertyWindow/PropertyRightArrowButton",
          "control lookup paths and order match constructor");
    Check(state.Read(88u) == 0x2004u &&
              state.Read(84u) == 0x2024u &&
              state.Read(64u) == 0x2028u &&
              state.Read(80u) == 0x2038u,
          "list and category controls occupy observed offsets");
    Check(runtime.subscriptions.size() == 8u &&
              runtime.subscriptions[0].handler == 0x472920u &&
              runtime.subscriptions[0].event ==
                  pal4::PropertyWindowEvent::MouseClick &&
              runtime.subscriptions[1].handler == 0x472f00u &&
              runtime.subscriptions[1].event ==
                  pal4::PropertyWindowEvent::HoverRowChanged &&
              runtime.subscriptions[2].handler == 0x4728b0u &&
              runtime.subscriptions[7].event ==
                  pal4::PropertyWindowEvent::Clicked,
          "list and six buttons subscribe to observed handlers");
    Check(Position(runtime.calls, "configure-list") <
              Position(runtime.calls, "image:zi") &&
              Position(runtime.calls, "image:xinzeng") <
              Position(runtime.calls, "mouse"),
          "columns precede image lookup and mouse setup");
    Check(runtime.columns == 0u && runtime.image_values.size() == 6u &&
              state.Read(152u) == 0x6000u &&
              state.Read(172u) == 0x6014u &&
              runtime.mouse_buttons == std::vector<std::uint32_t>{
                  0x2024u, 0x2028u, 0x202cu, 0x2030u,
                  0x2034u, 0x2038u},
          "columns, six category images and mouse handlers are initialized");
}

void TestCleanup() {
    pal4::PropertyWindowState state;
    Runtime runtime;
    runtime.fail_name_node = true;
    try {
        (void)pal4::PropertyWindowLifecycle::Construct(
            state, 0x7000u, runtime);
        Check(false, "name-node failure propagates");
    } catch (const std::runtime_error&) {
        Check(runtime.cleanup_count == 1u &&
                  runtime.cleanup_stage ==
                      pal4::PropertyWindowInitStage::Member,
              "name-node failure cleans initialized member");
    }
    runtime = Runtime{};
    runtime.fail_lookup = true;
    try {
        (void)pal4::PropertyWindowLifecycle::Construct(
            state, 0x7000u, runtime);
        Check(false, "lookup failure propagates");
    } catch (const std::runtime_error&) {
        Check(runtime.cleanup_count == 1u &&
                  runtime.cleanup_stage ==
                      pal4::PropertyWindowInitStage::UiObject,
              "lookup failure cleans fully initialized subobjects");
    }
    runtime = Runtime{};
    runtime.fail_image = true;
    try {
        (void)pal4::PropertyWindowLifecycle::Construct(
            state, 0x7000u, runtime);
        Check(false, "image failure propagates");
    } catch (const std::runtime_error&) {
        Check(runtime.cleanup_count == 1u &&
                  runtime.cleanup_stage ==
                      pal4::PropertyWindowInitStage::UiObject,
              "image failure cleans fully initialized subobjects");
    }
}

void TestDestroy() {
    pal4::PropertyWindowState state;
    state.Write(48u, 0xdeadbeefu);
    state.Write(52u, 4u);
    state.Write(56u, 8u);
    Runtime runtime;
    Check(pal4::PropertyWindowLifecycle::Destroy(state, runtime) ==
                  0x9876u &&
              runtime.calls == std::vector<std::string>{
                  "reset-node", "free-buffer", "destroy-base"},
          "destructor resets node, releases buffer and destroys base");
}

}  // namespace

int main() {
    TestConstruct();
    TestCleanup();
    TestDestroy();
    std::cout << "Property window lifecycle tests passed\n";
}
