#include "pal4/equipment_window.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Subscription {
    std::uint32_t window{};
    std::string event;
    std::uint32_t callback{};
};

struct Runtime final : pal4::EquipmentWindowRuntimeStub {
    std::vector<std::string> calls;
    std::vector<std::pair<std::string, bool>> names;
    std::vector<Subscription> subscriptions;
    std::vector<std::uint32_t> deleted_references;
    std::vector<std::uint32_t> deleted_callbacks;
    std::vector<std::uint32_t> freed_callbacks;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> mouse_handlers;
    std::vector<std::string> images;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::uint32_t next_callback{0x5000u};
    std::uint32_t columns{2u};
    std::uint32_t fail_member{};
    bool fail_name{};
    bool fail_insert{};
    bool fail_lookup{};
    bool fail_subscribe{};
    bool null_callbacks{};

    void LoadLayout(pal4::EquipmentWindowState& state,
                    std::uint32_t name, std::uint32_t first,
                    std::uint32_t second) override {
        Check(name == 0x7000u && first == 0u && second == 0u,
              "equipment layout receives name and zero flags");
        calls.emplace_back("layout");
        state.Write(0x14u, 0x8000u);
    }
    void ConstructMember(pal4::EquipmentWindowState&,
                         std::uint32_t offset) override {
        calls.emplace_back("member:" + std::to_string(offset));
        if (fail_member == offset) throw std::runtime_error("member");
    }
    void CleanupMember(pal4::EquipmentWindowState&,
                       std::uint32_t offset) override {
        calls.emplace_back("cleanup:" + std::to_string(offset));
    }
    void ConstructNameNode(pal4::EquipmentWindowState&) override {
        calls.emplace_back("name-node");
        if (fail_name) throw std::runtime_error("name-node");
    }
    void ResetNameNode(pal4::EquipmentWindowState&) override {
        calls.emplace_back("reset-name-node");
    }
    void DestroyDialogBase(pal4::EquipmentWindowState&) override {
        calls.emplace_back("destroy-base");
    }
    void FreeBuffer(std::uint32_t pointer) override {
        calls.emplace_back("free-buffer:" + std::to_string(pointer));
    }
    std::uint32_t AllocateCallback(std::uint32_t bytes) override {
        Check(bytes == 0x18u, "callback storage is 24 bytes");
        calls.emplace_back("allocate-callback");
        if (null_callbacks) return 0u;
        const auto result = next_callback;
        next_callback += 0x20u;
        return result;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    void FreeCallback(std::uint32_t callback) override {
        freed_callbacks.push_back(callback);
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x9000u;
    }
    void InsertOrReplace(std::uint32_t tree,
                         std::string_view command,
                         std::uint32_t& callback) override {
        Check(tree == 0x9104u &&
                  (command == "use_equipment" ||
                   command == "discard_equipment"),
              "commands enter manager tree");
        if (!null_callbacks)
            Check(callback == (command == "use_equipment" ?
                       0x5000u : 0x5020u),
                  "command callback address matches allocation order");
        calls.emplace_back("command:" + std::string(command));
        if (fail_insert) throw std::runtime_error("insert");
    }
    void DeleteCallback(std::uint32_t callback,
                        std::uint32_t flags) override {
        Check(flags == 1u, "command owner uses deleting destructor");
        deleted_callbacks.push_back(callback);
    }
    std::uint32_t FindWindowByName(std::string_view name) override {
        if (fail_lookup) throw std::runtime_error("lookup");
        names.emplace_back(std::string(name), true);
        return 0x2000u + static_cast<std::uint32_t>(names.size()) * 0x100u;
    }
    std::uint32_t FindWindow(std::string_view name) override {
        if (fail_lookup) throw std::runtime_error("lookup");
        names.emplace_back(std::string(name), false);
        return 0x2000u + static_cast<std::uint32_t>(names.size()) * 0x100u;
    }
    void SetFont(std::uint32_t window,
                 std::string_view font) override {
        Check(window == 0x2c00u && font == "Level_font",
              "level control receives Level_font");
        calls.emplace_back("font");
    }
    void SetFormatting(std::uint32_t window,
                       std::uint32_t horizontal,
                       std::uint32_t vertical) override {
        Check(window == 0x2c00u && horizontal == 2u && vertical == 2u,
              "level formatting is 2,2");
        calls.emplace_back("format");
    }
    std::uint32_t Subscribe(std::uint32_t window,
                            std::string_view event,
                            std::uint32_t callback) override {
        if (fail_subscribe) throw std::runtime_error("subscribe");
        subscriptions.push_back({window, std::string(event), callback});
        return 0xb000u + static_cast<std::uint32_t>(subscriptions.size()) * 4u;
    }
    std::uint32_t ReferenceCount(std::uint32_t) override { return 1u; }
    void SetReferenceCount(std::uint32_t,
                           std::uint32_t count) override {
        Check(count == 0u, "subscription local owner decremented");
    }
    void DeleteReference(std::uint32_t reference,
                         std::uint32_t flags) override {
        Check(flags == 1u, "subscription reference deleted");
        deleted_references.push_back(reference);
    }
    void SetupMouseEventHandlers(std::uint32_t member,
                                 std::uint32_t window) override {
        mouse_handlers.emplace_back(member, window);
    }
    void ActivateMember(std::uint32_t member) override {
        Check(member == 0x6118u, "secondary member activated");
        calls.emplace_back("activate");
    }
    void SetAlwaysOnTop(std::uint32_t window,
                        bool enabled) override {
        Check(window == 0x2100u && enabled, "list stays on top");
        calls.emplace_back("top");
    }
    std::uint32_t ColumnCount(std::uint32_t list) override {
        Check(list == 0x2100u, "column count uses list");
        return columns;
    }
    void RemoveColumn(std::uint32_t list,
                      std::uint32_t index) override {
        Check(list == 0x2100u && index == 0u && columns != 0u,
              "existing columns removed at zero");
        --columns;
        calls.emplace_back("remove-column");
    }
    void SetUserColumnSizingEnabled(std::uint32_t list,
                                     bool enabled) override {
        Check(list == 0x2100u && !enabled, "column sizing disabled");
        calls.emplace_back("size");
    }
    void SetUserColumnDraggingEnabled(std::uint32_t list,
                                       bool enabled) override {
        Check(list == 0x2100u && !enabled, "column dragging disabled");
        calls.emplace_back("drag");
    }
    void SetUserSortControlEnabled(std::uint32_t list,
                                    bool enabled) override {
        Check(list == 0x2100u && !enabled, "user sort disabled");
        calls.emplace_back("sort");
    }
    void AddColumn(std::uint32_t list, std::string_view title,
                   std::uint32_t id, float width) override {
        Check(list == 0x2100u && title.empty() &&
                  (id == 0u ? width == 0.60000002f :
                              id == 1u && width == 0.34999999f),
              "two blank columns retain IDA widths");
        calls.emplace_back("column:" + std::to_string(id));
    }
    void SetColumnLabelImage(std::uint32_t list,
                             std::uint32_t column,
                             std::string_view image,
                             std::string_view label) override {
        Check(list == 0x2100u && image == "zi0" &&
                  (column == 0u ? label == "mingcheng" :
                                   column == 1u && label == "dengji"),
              "column images and labels match IDA literals");
        calls.emplace_back("label:" + std::to_string(column));
    }
    void SetEnabled(std::uint32_t window,
                    bool enabled) override {
        Check(!enabled && (window == 0x2500u || window == 0x2600u),
              "description and property start disabled");
        calls.emplace_back("disable:" + std::to_string(window));
    }
    std::uint32_t GetImage(std::string_view imageset,
                           std::string_view image) override {
        Check(imageset == "zi0", "equipment icons use zi0 imageset");
        images.emplace_back(image);
        return 0x8000u + static_cast<std::uint32_t>(images.size());
    }
};

void CheckNormal() {
    pal4::EquipmentWindowState state{};
    Runtime runtime;
    Check(&pal4::EquipmentWindow::Construct(state, 0x6000u,
                                            0x7000u, runtime) == &state,
          "constructor returns object state");
    Check(state.Read(0u) == 0x843838u &&
                  state.Read(0x28u) == 0x843850u &&
                  state.Read(0x10cu) == 0x8438acu &&
                  state.Read(0x118u) == 0x843878u &&
                  state.Read(0xf0u) == 5u &&
                  state.Read(0xf4u) == 0u && state.Read(0x100u) == 0u &&
                  state.Read(0x110u) == 0x6000u &&
                  state.Read(0x130u) == 0x6000u,
          "constructor stores vtables, counters and two owner fields");
    Check(runtime.calls[0] == "layout" &&
                  runtime.calls[1] == "member:40" &&
                  runtime.calls[2] == "name-node" &&
                  runtime.calls[3] == "member:280" &&
                  runtime.calls[6] == "command:use_equipment" &&
                  runtime.calls[9] == "command:discard_equipment" &&
                  runtime.calls.back() == "activate",
          "base objects and commands initialize before UI controls");
    Check(runtime.deleted_callbacks ==
                  std::vector<std::uint32_t>{0x5000u, 0x5020u} &&
                  runtime.words.at(0x5000u) == 0x84382cu &&
                  runtime.words.at(0x5008u) == 0x4502b0u &&
                  runtime.words.at(0x5020u) == 0x84382cu &&
                  runtime.words.at(0x5028u) == 0x450370u &&
                  runtime.words.at(0x5010u) == 0x6000u &&
                  runtime.words.at(0x5030u) == 0x6000u,
          "two command callbacks have IDA handlers and owner");
    Check(runtime.names.size() == 38u &&
                  runtime.names[0] ==
                      std::pair<std::string, bool>{"EquipmentWindow/List", true} &&
                  runtime.names[6] ==
                      std::pair<std::string, bool>{"EquipmentWindow/PanelList", false} &&
                  runtime.names[12] ==
                      std::pair<std::string, bool>{"EquipmentWindow/BtnEquipmentClass0", true} &&
                  runtime.names[22] ==
                      std::pair<std::string, bool>{"EquipmentWindow/StaticAttr1", true} &&
                  runtime.names[32] ==
                      std::pair<std::string, bool>{"EquipmentWindow/ImageRoleEquip1", true},
          "fixed and indexed controls use correct lookup variants");
    Check(state.Read(0x40u) == 0x2100u &&
                  state.Read(0xb4u) == 0x2c00u &&
                  state.Read(0x50u) == 0x2d00u &&
                  state.Read(0xe0u) == 0x3300u &&
                  state.Read(0x70u) == 0x3700u &&
                  state.Read(0x98u) == 0x4100u &&
                  state.Read(0xacu) == 0x4600u,
          "control arrays occupy IDA state offsets");
    constexpr std::uint32_t equip_tags[]{0u, 2u, 3u, 1u, 4u, 5u};
    for (std::uint32_t i = 0u; i < 6u; ++i) {
        Check(runtime.words.at(0x2d00u + i * 0x100u + 0x118u) == i &&
                      runtime.words.at(0x4100u + i * 0x100u + 0x118u) ==
                          equip_tags[i],
              "category and equipped item indices stored at +0x118");
    }
    for (std::uint32_t i = 0u; i < 10u; ++i)
        Check(runtime.words.at(0x3700u + i * 0x100u + 0x118u) == i,
              "attribute control index stored at +0x118");
    Check(runtime.subscriptions.size() == 16u &&
                  runtime.deleted_references.size() == 16u &&
                  runtime.freed_callbacks.empty() &&
                  runtime.subscriptions[0].event == "MouseClick" &&
                  runtime.subscriptions[1].event == "HoverRowChanged" &&
                  runtime.subscriptions[2].window == 0x2d00u &&
                  runtime.subscriptions[8].window == 0x4100u &&
                  runtime.subscriptions[14].window == 0x2300u &&
                  runtime.subscriptions[15].window == 0x2400u,
          "16 event handlers subscribe in observed order");
    constexpr std::uint32_t event_handlers[]{
        0x452bd0u, 0x452bf0u, 0x452c20u, 0x452c20u,
        0x452c20u, 0x452c20u, 0x452c20u, 0x452c20u,
        0x452e40u, 0x452e40u, 0x452e40u, 0x452e40u,
        0x452e40u, 0x452e40u, 0x452ca0u, 0x452d70u};
    for (std::uint32_t i = 0u; i < 16u; ++i) {
        const auto callback = runtime.subscriptions[i].callback;
        Check(runtime.words.at(callback) == 0x843824u &&
                      runtime.words.at(callback + 8u) == event_handlers[i] &&
                      runtime.words.at(callback + 0x10u) == 0x6000u,
              "event callback vtable, handler and owner");
    }
    Check(runtime.mouse_handlers ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x6028u, 0x3200u}, {0x6028u, 0x2d00u},
                      {0x6028u, 0x2e00u}, {0x6028u, 0x2f00u},
                      {0x6028u, 0x3000u}, {0x6028u, 0x3100u},
                      {0x6118u, 0x4100u}, {0x6118u, 0x4200u},
                      {0x6118u, 0x4500u}, {0x6118u, 0x4400u},
                      {0x6118u, 0x4300u}, {0x6118u, 0x4600u}},
          "two member adapters receive controls in IDA order");
    Check(runtime.columns == 0u &&
                  runtime.images == std::vector<std::string>{
                      "wuqi", "toubu", "shenti", "zhubu", "peidai", "xinzeng"} &&
                  state.Read(0xc8u) == 0x8001u &&
                  state.Read(0xdcu) == 0x8006u,
          "list columns and six equipment icons initialized");
}

void CheckUnwind() {
    using Window = pal4::EquipmentWindow;
    pal4::EquipmentWindowState state{};
    Runtime runtime;
    runtime.fail_member = 0x28u;
    try {
        (void)Window::Construct(state, 0x6000u, 0x7000u, runtime);
        Check(false, "primary member failure must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.calls == std::vector<std::string>{
                  "layout", "member:40", "destroy-base"},
              "primary member failure cleans base");
    }
    state = {};
    runtime = Runtime{};
    runtime.fail_name = true;
    try {
        (void)Window::Construct(state, 0x6000u, 0x7000u, runtime);
        Check(false, "name-node failure must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.calls == std::vector<std::string>{
                  "layout", "member:40", "name-node",
                  "cleanup:40", "destroy-base"},
              "name-node failure cleans primary member and base");
    }
    state = {};
    runtime = Runtime{};
    runtime.fail_member = 0x118u;
    try {
        (void)Window::Construct(state, 0x6000u, 0x7000u, runtime);
        Check(false, "secondary member failure must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.calls == std::vector<std::string>{
                  "layout", "member:40", "name-node", "member:280",
                  "reset-name-node", "cleanup:40", "destroy-base"},
              "secondary member failure unwinds completed subobjects");
    }
    state = {};
    runtime = Runtime{};
    runtime.fail_insert = true;
    try {
        (void)Window::Construct(state, 0x6000u, 0x7000u, runtime);
        Check(false, "command insertion failure must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.deleted_callbacks ==
                  std::vector<std::uint32_t>{0x5000u} &&
                  runtime.calls[runtime.calls.size() - 4u] ==
                      "cleanup:280" &&
                  runtime.calls.back() == "destroy-base",
              "failed command releases callback and constructed layers");
    }
    state = {};
    runtime = Runtime{};
    runtime.fail_lookup = true;
    try {
        (void)Window::Construct(state, 0x6000u, 0x7000u, runtime);
        Check(false, "control lookup failure must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.deleted_callbacks.size() == 2u &&
                  runtime.calls.back() == "destroy-base",
              "lookup failure cleans both members and base");
    }
    state = {};
    runtime = Runtime{};
    runtime.fail_subscribe = true;
    try {
        (void)Window::Construct(state, 0x6000u, 0x7000u, runtime);
        Check(false, "subscription failure must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.freed_callbacks ==
                  std::vector<std::uint32_t>{0x5040u} &&
                  runtime.subscriptions.empty() &&
                  runtime.calls.back() == "destroy-base",
              "failed subscription frees untransferred callback");
    }
}

void CheckDestroy() {
    pal4::EquipmentWindowState state{};
    state.Write(0x120u, 0x5000u);
    state.Write(0x124u, 4u);
    state.Write(0x128u, 8u);
    state.Write(0x30u, 0x6000u);
    state.Write(0x34u, 4u);
    state.Write(0x38u, 8u);
    Runtime runtime;
    Check(pal4::EquipmentWindow::Destroy(state, 0x7000u, runtime) ==
                  0x7000u &&
                  runtime.calls == std::vector<std::string>{
                      "free-buffer:20480", "reset-name-node",
                      "free-buffer:24576", "destroy-base"} &&
                  state.Read(0x120u) == 0u &&
                  state.Read(0x124u) == 0u &&
                  state.Read(0x128u) == 0u &&
                  state.Read(0x30u) == 0u &&
                  state.Read(0x34u) == 0u &&
                  state.Read(0x38u) == 0u,
          "destructor frees both buffers and resets both triples");
}

}  // namespace

int main() {
    CheckNormal();
    CheckUnwind();
    CheckDestroy();
    std::cout << "Equipment window constructor tests passed\n";
}
