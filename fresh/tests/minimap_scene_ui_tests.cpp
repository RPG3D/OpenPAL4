#include "pal4/minimap_scene_ui.h"
#include "pal4/minimap_scene_ui_buttons.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct ComponentRuntime final : pal4::ISceneUiComponentRuntime {
    std::vector<std::string> calls;
    std::string name;
    bool throw_layout = false;
    int finds = 0;

    void LoadLayout(std::uint32_t address, std::string_view layout,
                    std::uint32_t parent, std::uint32_t flags) override {
        Check(address == 0x502cu && layout == "minimap" &&
                  parent == 0u && flags == 0u,
              "embedded component layout and address");
        calls.emplace_back("layout");
        if (throw_layout) throw std::runtime_error("component layout");
    }
    void DestroyBase(std::uint32_t) override {
        calls.emplace_back("destroy");
    }
    void InitializeNameBuffer() override {
        calls.emplace_back("buffer");
    }
    void AssignPrefix(std::uint32_t source, std::uint32_t offset,
                      std::uint32_t count) override {
        Check(source == 0x5040u && offset == 0u &&
                  count == 0xffffffffu,
              "embedded component prefix address");
        name = "minimap/";
    }
    void AppendSuffix(std::string_view suffix, bool) override {
        name += suffix;
    }
    std::string_view CurrentName() override { return name; }
    std::uint32_t FindWindow(std::uint32_t address,
                             std::string_view) override {
        Check(address == 0x502cu, "embedded component window lookup");
        return 0x6100u + static_cast<std::uint32_t>(finds++);
    }
    void SetHorizontalFormatting(std::uint32_t window,
                                 std::uint32_t format) override {
        Check(window == 0x6103u && format == 2u,
              "embedded StaticScene formatting");
    }
    std::uint32_t CleanupNameBuffer() override {
        calls.emplace_back("cleanup");
        return 0u;
    }
    void SetParent(std::uint32_t, std::uint8_t,
                   std::uint32_t) override {}
};

struct Runtime final : pal4::IMinimapSceneUiRuntime {
    struct Subscription {
        std::uint32_t window;
        pal4::MinimapUiEvent event;
        std::uint32_t handler;
    };
    std::vector<std::string> calls;
    std::vector<Subscription> subscriptions;
    std::vector<std::uint32_t> commands;
    std::vector<std::uint32_t> released;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;
    std::map<std::uint32_t, std::uint8_t> text_references;
    std::vector<std::uint32_t> freed_text;
    std::vector<std::uint32_t> destroyed_bases;
    std::uint32_t next_storage = 0x9000u;
    std::uint32_t current_handler = 0u;
    int allocation_count = 0;
    int subscribe_count = 0;
    int manager_count = 0;
    int unwind_stage = -1;
    bool text_allocation = true;
    bool callback_allocation = true;
    bool throw_subscribe = false;
    bool throw_insert = false;
    bool throw_root = false;

    void LoadLayout(pal4::MinimapSceneUiState& state,
                    std::uint32_t address, std::uint32_t name,
                    std::uint32_t parent, std::uint32_t flags) override {
        Check(address == 0x5000u && name == 0x8b0460u &&
                  parent == 0u && flags == 0u,
              "root dialog layout arguments");
        calls.emplace_back("root-layout");
        if (throw_root) throw std::runtime_error("root layout");
        state.WriteWord(0x24u, 0x7000u);
    }
    void ProcessUtf8(std::uint32_t address) override {
        Check(address == 0x506cu, "UTF8 member address");
        calls.emplace_back("utf8");
    }
    void ResetText(std::uint32_t address, std::uint8_t error) override {
        Check((address == 0x50d0u || address == 0x50e0u) && error == 0u,
              "two text-buffer resets");
        calls.push_back(address == 0x50d0u ? "reset-timer" : "reset-other");
    }
    bool AllocateText(std::uint32_t address, std::string_view text,
                      std::uint8_t mode) override {
        Check(address == 0x50d0u && text == "SceneTimer" && mode == 1u,
              "SceneTimer allocation");
        calls.emplace_back("allocate-text");
        return text_allocation;
    }
    void SetTextSize(std::uint32_t address,
                     std::uint32_t size) override {
        Check(address == 0x50d0u && size == 10u,
              "SceneTimer copy and size");
        calls.emplace_back("set-text-size");
    }
    void SetClippedByParent(std::uint32_t window,
                            bool clipped) override {
        Check(window == 0x7000u && !clipped,
              "root window clipping disabled");
        calls.emplace_back("clip");
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0x18u, "callback allocation size");
        ++allocation_count;
        calls.emplace_back("allocate-callback");
        if (!callback_allocation) return 0u;
        const auto storage = next_storage;
        next_storage += 0x20u;
        return storage;
    }
    void WriteCallbackWord(std::uint32_t address,
                           std::uint32_t value) override {
        writes.emplace_back(address, value);
        if ((address & 0x1fu) == 8u) current_handler = value;
    }
    std::uint32_t ConstructCallback(
        std::uint32_t storage, std::uint32_t handler,
        std::uint32_t argument, std::uint32_t owner,
        bool command_callback) override {
        Check(argument == 0u && owner == 0x5000u,
              "constructed callback arguments");
        Check(command_callback == (allocation_count > 10),
              "command callback constructor variant");
        current_handler = handler;
        calls.emplace_back("construct-callback");
        return storage;
    }
    void Subscribe(std::uint32_t window, pal4::MinimapUiEvent event,
                   std::uint32_t callback) override {
        if (callback_allocation) Check(callback != 0u, "event callback exists");
        subscriptions.push_back({window, event, current_handler});
        ++subscribe_count;
        calls.emplace_back("subscribe");
        if (throw_subscribe && subscribe_count == 4) {
            throw std::runtime_error("subscribe");
        }
    }
    std::uint32_t FrameManager() override {
        ++manager_count;
        calls.emplace_back("manager");
        return 0x8000u;
    }
    void InsertCommand(std::uint32_t tree,
                       std::uint32_t key_pointer_cell,
                       std::uint32_t callback) override {
        Check(tree == 0x8104u &&
                  (key_pointer_cell == 0x8a80c4u ||
                   key_pointer_cell == 0x8a80a8u) &&
                  (callback != 0u || !callback_allocation),
              "command tree, key cell and callback");
        commands.push_back(key_pointer_cell);
        calls.emplace_back("insert-command");
        if (throw_insert) throw std::runtime_error("insert");
    }
    void ReleaseCallback(std::uint32_t callback) override {
        released.push_back(callback);
        calls.emplace_back("release");
    }
    void UnwindConstruction(std::uint32_t address,
                             std::uint32_t stage) override {
        Check(address == 0x5000u, "unwind object address");
        unwind_stage = static_cast<int>(stage);
        calls.emplace_back("unwind");
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.push_back("read-ref:" + std::to_string(address));
        return text_references.at(address);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        calls.push_back("write-ref:" + std::to_string(address));
        text_references[address] = value;
    }
    void FreeText(std::uint32_t address) override {
        calls.push_back("free-text:" + std::to_string(address));
        freed_text.push_back(address);
    }
    std::uint32_t DestroyUtf8(std::uint32_t address) override {
        calls.push_back("destroy-utf8:" + std::to_string(address));
        return address + 1u;
    }
    std::uint32_t DestroyBase(std::uint32_t address) override {
        calls.push_back("destroy-base:" + std::to_string(address));
        destroyed_bases.push_back(address);
        return address + 2u;
    }
};

struct ButtonRuntime final : pal4::IMinimapSceneUiButtonRuntime {
    explicit ButtonRuntime(Runtime& parent_runtime) : parent(parent_runtime) {}
    Runtime& parent;
    std::string name;
    int finds = 0;
    int subscriptions = 0;
    int releases = 0;
    std::uint32_t next_callback = 0xb000u;

    void InitializeNameBuffer() override {
        parent.calls.emplace_back("buttons");
    }
    void AssignPrefix(std::uint32_t address, std::uint32_t offset,
                      std::uint32_t count) override {
        Check(address == 0x5014u && offset == 0u &&
                  count == 0xffffffffu,
              "button name prefix record");
        name = "portrait/";
    }
    void AppendSuffix(std::string_view suffix, bool) override {
        name += suffix;
    }
    std::string_view CurrentName() override { return name; }
    std::uint32_t FindWindow(std::uint32_t owner,
                             std::string_view) override {
        Check(owner == 0x5000u, "button lookup root address");
        return 0x6500u + static_cast<std::uint32_t>(finds++);
    }
    std::uint32_t AllocateCallback(std::uint32_t size) override {
        Check(size == 0x18u, "button callback allocation size");
        const auto storage = next_callback;
        next_callback += 0x20u;
        return storage;
    }
    std::uint32_t ConstructCallback(
        std::uint32_t storage, std::uint32_t,
        std::uint32_t argument, std::uint32_t owner) override {
        Check(storage != 0u && argument == 0u && owner == 0x5000u,
              "button callback construction");
        return storage;
    }
    void Subscribe(std::uint32_t window, pal4::MinimapUiEvent,
                   std::uint32_t callback) override {
        Check((window == 0x6500u || window == 0x6502u ||
               window == 0x6503u) && callback != 0u,
              "button callback target");
        ++subscriptions;
    }
    void ReleaseCallback(std::uint32_t callback) override {
        Check(callback != 0u, "button callback release");
        ++releases;
    }
    std::uint32_t CleanupNameBuffer() override { return 0u; }
};

void CheckSuccess() {
    pal4::MinimapSceneUiState state;
    ComponentRuntime component;
    Runtime runtime;
    ButtonRuntime buttons(runtime);
    Check(pal4::MinimapSceneUi::Construct(
              state, 0x5000u, 0x8b0460u,
              component, buttons, runtime) == 0x5000u,
          "main constructor returns object address");
    Check(component.finds == 6 && component.calls ==
              std::vector<std::string>{"layout", "buffer", "cleanup"} &&
              state.component.Read(0u) == 0x843ad0u,
          "embedded component constructed at +0x2c");
    Check(state.ReadWord(0u) == 0x843b08u &&
              state.ReadWord(0x28u) == 0x843af0u &&
              state.ReadWord(0x2cu) == 0x843ad0u &&
              state.ReadWord(0x54u) == 0x6100u &&
              state.ReadWord(0x60u) == 0x6101u &&
              state.ReadWord(0x64u) == 0x6102u &&
              state.ReadWord(0x68u) == 0x6104u &&
              state.ReadWord(0xc0u) == 0x6501u &&
              state.ReadWord(0xccu) == 0u &&
              state.ReadByte(0xf4u) == 0u &&
              state.ReadByte(0xf5u) == 1u &&
              state.ReadByte(0xf7u) == 1u,
          "vtable and final state offsets");
    Check(runtime.allocation_count == 12 &&
              buttons.finds == 7 && buttons.subscriptions == 9 &&
              buttons.releases == 9 &&
              runtime.subscriptions.size() == 10u &&
              runtime.released.size() == 12u &&
              runtime.manager_count == 2 &&
              runtime.commands ==
                  std::vector<std::uint32_t>{0x8a80c4u, 0x8a80a8u},
          "ten events and two commands own distinct callback records");
    const std::array<Runtime::Subscription, 10> expected{{
        {0x6101u, pal4::MinimapUiEvent::Clicked, 0x4791f0u},
        {0x6101u, pal4::MinimapUiEvent::MouseDown, 0x47a940u},
        {0x6101u, pal4::MinimapUiEvent::MouseUp, 0x47a950u},
        {0x6104u, pal4::MinimapUiEvent::Clicked, 0x47a790u},
        {0x6104u, pal4::MinimapUiEvent::MouseDown, 0x47a940u},
        {0x6104u, pal4::MinimapUiEvent::MouseUp, 0x47a950u},
        {0x6102u, pal4::MinimapUiEvent::Clicked, 0x47a900u},
        {0x6102u, pal4::MinimapUiEvent::MouseDown, 0x47a940u},
        {0x6102u, pal4::MinimapUiEvent::MouseUp, 0x47a950u},
        {0x6100u, pal4::MinimapUiEvent::MouseMove, 0x479210u},
    }};
    for (std::size_t index = 0; index < expected.size(); ++index) {
        const auto& actual = runtime.subscriptions[index];
        const auto& wanted = expected[index];
        Check(actual.window == wanted.window &&
                  actual.event == wanted.event &&
                  actual.handler == wanted.handler,
              "event window, type and handler in IDA order");
    }
    Check(runtime.writes.size() == 8u &&
              runtime.writes[0] == std::pair{0x9008u, 0x4791f0u} &&
              runtime.writes[1] == std::pair{0x900cu, 0u} &&
              runtime.writes[2] == std::pair{0x9010u, 0x5000u} &&
              runtime.writes[3] == std::pair{0x9000u, 0x843ae8u},
          "first two callback records use inline vtable layout");
    Check(runtime.calls[0] == "root-layout" &&
              runtime.calls[1] == "utf8" &&
              runtime.calls[2] == "reset-timer" &&
              runtime.calls[3] == "allocate-text" &&
              runtime.calls[4] == "set-text-size" &&
              runtime.calls[5] == "reset-other" &&
              runtime.calls[6] == "buttons" &&
              runtime.calls[7] == "clip" &&
              runtime.unwind_stage == -1,
          "layout, text, controls and clip initialization order");
}

void CheckAlternateAndFailurePaths() {
    pal4::MinimapSceneUiState state;
    ComponentRuntime component;
    Runtime runtime;
    ButtonRuntime buttons(runtime);
    runtime.text_allocation = false;
    runtime.callback_allocation = false;
    (void)pal4::MinimapSceneUi::Construct(
        state, 0x5000u, 0x8b0460u, component, buttons, runtime);
    Check(runtime.allocation_count == 12 && runtime.released.empty() &&
              runtime.subscriptions.size() == 10u &&
              runtime.commands.size() == 2u,
          "null callback allocation still attempts event and command binding");
    for (const auto& call : runtime.calls) {
        Check(call != "set-text-size", "failed timer allocation skips text copy");
    }

    pal4::MinimapSceneUiState failed_state;
    ComponentRuntime failed_component;
    Runtime failed_runtime;
    ButtonRuntime failed_buttons(failed_runtime);
    failed_runtime.throw_subscribe = true;
    bool caught = false;
    try {
        (void)pal4::MinimapSceneUi::Construct(
            failed_state, 0x5000u, 0x8b0460u,
            failed_component, failed_buttons, failed_runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && failed_runtime.subscribe_count == 4 &&
              failed_runtime.released.size() == 4u &&
              failed_runtime.unwind_stage == 4,
          "subscription failure releases local callback then unwinds object");

    ComponentRuntime component_failure;
    component_failure.throw_layout = true;
    Runtime parent_failure;
    ButtonRuntime parent_buttons(parent_failure);
    caught = false;
    try {
        (void)pal4::MinimapSceneUi::Construct(
            failed_state, 0x5000u, 0x8b0460u,
            component_failure, parent_buttons, parent_failure);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && parent_failure.unwind_stage == 0 &&
              parent_failure.calls ==
                  std::vector<std::string>{"root-layout", "unwind"},
          "failed embedded layout unwinds only root construction stage");

    ComponentRuntime root_component;
    Runtime root_failure;
    ButtonRuntime root_buttons(root_failure);
    root_failure.throw_root = true;
    caught = false;
    try {
        (void)pal4::MinimapSceneUi::Construct(
            failed_state, 0x5000u, 0x8b0460u,
            root_component, root_buttons, root_failure);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && root_failure.unwind_stage == -1 &&
              root_component.calls.empty() &&
              root_failure.calls ==
                  std::vector<std::string>{"root-layout"},
          "root layout failure has no constructed-member unwind");

    ComponentRuntime command_component;
    Runtime command_failure;
    ButtonRuntime command_buttons(command_failure);
    command_failure.throw_insert = true;
    caught = false;
    try {
        (void)pal4::MinimapSceneUi::Construct(
            failed_state, 0x5000u, 0x8b0460u,
            command_component, command_buttons, command_failure);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && command_failure.subscriptions.size() == 10u &&
              command_failure.commands ==
                  std::vector<std::uint32_t>{0x8a80c4u} &&
              command_failure.released.size() == 11u &&
              command_failure.unwind_stage == 4 &&
              command_failure.calls[command_failure.calls.size() - 2u] ==
                  "release" &&
              command_failure.calls.back() == "unwind",
          "command insertion failure releases local owner before unwind");
}

void CheckDestruction() {
    pal4::MinimapSceneUiState state;
    Runtime runtime;
    state.WriteWord(0xe4u, 0xa100u);
    state.WriteWord(0xe8u, 7u);
    state.WriteWord(0xecu, 9u);
    state.WriteWord(0xd4u, 0xa200u);
    state.WriteWord(0xd8u, 3u);
    state.WriteWord(0xdcu, 5u);
    runtime.text_references[0xa0ffu] = 2u;
    runtime.text_references[0xa1ffu] = 0u;
    Check(pal4::MinimapSceneUi::DestroyUtf8Thunk(
              0x506cu, runtime) == 0x506du &&
              runtime.calls ==
                  std::vector<std::string>{"destroy-utf8:20588"},
          "UTF8 destructor thunk forwards and returns result");
    runtime.calls.clear();
    Check(pal4::MinimapSceneUi::Destroy(
              state, 0x5000u, runtime) == 0x5002u,
          "destructor returns root base result");
    Check(state.ReadWord(0xe4u) == 0u &&
              state.ReadWord(0xe8u) == 0u &&
              state.ReadWord(0xecu) == 0u &&
              state.ReadWord(0xd4u) == 0u &&
              state.ReadWord(0xd8u) == 0u &&
              state.ReadWord(0xdcu) == 0u &&
              runtime.text_references[0xa0ffu] == 1u &&
              runtime.freed_text == std::vector<std::uint32_t>{0xa1ffu} &&
              runtime.destroyed_bases ==
                  std::vector<std::uint32_t>{0x502cu, 0x5000u},
          "reference counts, text metadata and base teardown");
    const std::vector<std::string> expected{
        "read-ref:41215", "write-ref:41215", "read-ref:41471",
        "free-text:41471", "destroy-utf8:20588",
        "destroy-base:20524", "destroy-base:20480"};
    Check(runtime.calls == expected,
          "two text records and three subobjects tear down in IDA order");

    pal4::MinimapSceneUiState empty_state;
    Runtime empty_runtime;
    (void)pal4::MinimapSceneUi::Destroy(empty_state, 0x5000u, empty_runtime);
    Check(empty_runtime.freed_text.empty() &&
              empty_runtime.calls.size() == 3u,
          "null text records skip reference reads");

    pal4::MinimapSceneUiState sentinel_state;
    Runtime sentinel_runtime;
    sentinel_state.WriteWord(0xe4u, 0xa300u);
    sentinel_runtime.text_references[0xa2ffu] = 0xffu;
    (void)pal4::MinimapSceneUi::Destroy(
        sentinel_state, 0x5000u, sentinel_runtime);
    Check(sentinel_runtime.freed_text ==
              std::vector<std::uint32_t>{0xa2ffu},
          "0xff reference marker frees the allocation");
}

}  // namespace

int main() {
    CheckSuccess();
    CheckAlternateAndFailurePaths();
    CheckDestruction();
}
