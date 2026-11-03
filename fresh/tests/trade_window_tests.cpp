#include "pal4/trade_window.h"

#include <array>
#include <cstdlib>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::ITradeWindowRuntime {
public:
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::string, std::uint32_t> windows;
    std::vector<std::pair<std::uint32_t, std::string>> texts;
    std::vector<std::tuple<std::uint32_t, std::uint32_t, std::uint32_t>> events;
    std::vector<std::tuple<std::uint32_t, pal4::TradeUiEvent, std::uint32_t>> subscriptions;
    std::vector<std::pair<std::string, std::string>> images;
    std::vector<std::uint32_t> released_callbacks;
    std::vector<std::uint32_t> destroyed_texts;
    std::vector<std::uint32_t> freed;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> horizontal;
    std::vector<std::pair<std::uint32_t, std::string>> fonts;
    std::vector<std::tuple<std::uint32_t, std::uint32_t, std::uint32_t>> formatting;
    std::uint32_t layout_frame = 0;
    std::uint32_t layout_name = 0;
    std::uint32_t node = 0;
    std::uint32_t destroyed_node = 0;
    std::uint32_t destroyed_base = 0;
    std::vector<std::string> list_steps;
    std::uint32_t configured_list = 0;
    std::uint32_t removed_columns = 0;
    std::uint32_t released_connections = 0;
    std::uint32_t next_allocation = 0x10000;
    std::uint32_t next_window = 0x20000;
    std::uint32_t columns = 3;
    unsigned throw_on_event = 0;
    bool throw_on_subscribe = false;
    bool throw_on_image = false;
    bool throw_on_text = false;
    bool null_allocation = false;
    bool null_game_object = false;
    std::vector<std::string> visibility_steps;
    std::uint32_t parent = 0;
    bool visibility = false;
    std::uint32_t effect_manager = 0;
    std::array<float, 3> effect_position{};
    std::array<float, 3> effect_scale{};
    std::array<float, 3> flower_position{};
    std::array<float, 3> flower_scale{};
    std::uint32_t flower_collection = 0;
    std::vector<std::uint32_t> removed_objects;
    std::uint32_t effect_event = 0xffffffffu;
    std::uint32_t effect_handle = 0;
    std::uint32_t text_window = 0;
    std::string visible_text;
    std::vector<std::uint32_t> trade_items;
    std::map<std::uint32_t, bool> item_text_checks;
    std::map<std::uint32_t, std::uint32_t> resolved_items;
    std::vector<std::uint32_t> cleared_groups;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> appended_items;
    std::vector<std::uint32_t> removed_active;
    std::vector<std::uint32_t> inserted_active;
    std::vector<std::pair<std::uint32_t, bool>> window_visibility;
    std::vector<std::uint32_t> updated_pages;
    std::uint32_t copied_attribute_source = 0;
    std::uint32_t released_attribute = 0;
    bool throw_on_trade_item = false;
    std::array<std::array<std::int32_t, 7>, 3> inventory_sizes{};
    std::uint32_t inventory_calls = 0;
    std::vector<std::string> binding_calls;
    std::uint32_t bound_collection = 0;
    std::uint32_t bound_identifier = 0;
    std::uint32_t bound_delay = 0;
    std::uint32_t bound_frame = 0;
    std::uint32_t bound_handler = 0;
    std::uint32_t bound_value = 0;
    bool bound_flag = false;
    std::uint32_t processed_tree = 0u;
    std::uint32_t processed_key = 0u;
    std::uint32_t processed_first = 0u;
    std::uint32_t processed_second = 0u;

    void LoadLayout(std::uint32_t frame, std::uint32_t name) override {
        layout_frame = frame;
        layout_name = name;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    void AssignText(std::uint32_t buffer, std::string_view text) override {
        if (throw_on_text) throw std::runtime_error("text");
        texts.emplace_back(buffer, text);
    }
    void ConstructNameNode(std::uint32_t address) override { node = address; }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 12u, "callback allocation size");
        if (null_allocation) return 0u;
        const auto result = next_allocation;
        next_allocation += 0x10u;
        return result;
    }
    std::uint32_t FrameManager() override {
        binding_calls.emplace_back("manager");
        return 0x8000u;
    }
    void InsertOrReplace(std::uint32_t tree, std::uint32_t key,
                         std::uint32_t callback) override {
        events.emplace_back(tree, key, callback);
        if (events.size() == throw_on_event) throw std::runtime_error("event");
    }
    void ReleaseCallback(std::uint32_t callback) override {
        released_callbacks.push_back(callback);
    }
    void Free(std::uint32_t storage) override { freed.push_back(storage); }
    std::uint32_t FindWindow(std::string_view name) override {
        const auto [it, inserted] = windows.try_emplace(std::string(name), next_window);
        if (inserted) next_window += 4u;
        return it->second;
    }
    std::uint32_t GetWindow(std::string_view name) override {
        return FindWindow(name);
    }
    void SetSortDirection(std::uint32_t list,
                          std::uint32_t direction) override {
        Check(direction == 0u, "list sort direction");
        configured_list = list;
        list_steps.emplace_back("sort");
    }
    void SetColumnDraggingEnabled(std::uint32_t list,
                                  bool enabled) override {
        Check(list == configured_list && !enabled, "column dragging disabled");
        list_steps.emplace_back("drag");
    }
    void SetColumnSizingEnabled(std::uint32_t list,
                                bool enabled) override {
        Check(list == configured_list && !enabled, "column sizing disabled");
        list_steps.emplace_back("size");
    }
    void SetUserSortEnabled(std::uint32_t list,
                            bool enabled) override {
        Check(list == configured_list && !enabled, "user sorting disabled");
        list_steps.emplace_back("user-sort");
    }
    void ResetList(std::uint32_t list) override {
        Check(configured_list == 0u || list == configured_list,
              "list reset target");
        list_steps.emplace_back("reset");
        visibility_steps.emplace_back("reset-list");
    }
    std::uint32_t ColumnCount(std::uint32_t) override { return columns; }
    void RemoveFirstColumn(std::uint32_t) override { ++removed_columns; }
    void SetHorizontalFormatting(std::uint32_t window,
                                 std::uint32_t mode) override {
        horizontal.emplace_back(window, mode);
    }
    void SetFont(std::uint32_t window, std::string_view font) override {
        fonts.emplace_back(window, font);
    }
    void SetFormatting(std::uint32_t window, std::uint32_t h,
                       std::uint32_t v) override {
        formatting.emplace_back(window, h, v);
    }
    std::uint32_t Subscribe(std::uint32_t window, pal4::TradeUiEvent event,
                            std::uint32_t callback) override {
        if (throw_on_subscribe) throw std::runtime_error("subscribe");
        subscriptions.emplace_back(window, event, callback);
        return 0x9000u;
    }
    void ReleaseConnection(std::uint32_t connection) override {
        Check(connection == 0x9000u, "connection identity");
        ++released_connections;
    }
    std::uint32_t ResolveImage(std::string_view imageset,
                               std::string_view image) override {
        if (throw_on_image) throw std::runtime_error("image");
        images.emplace_back(imageset, image);
        return 0x30000u + static_cast<std::uint32_t>(images.size());
    }
    void DestroyText(std::uint32_t buffer) override {
        destroyed_texts.push_back(buffer);
    }
    void DestroyNameNode(std::uint32_t address) override {
        destroyed_node = address;
    }
    std::uint32_t DestroyBase(std::uint32_t frame) override {
        destroyed_base = frame;
        return frame + 1u;
    }
    void SetParent(std::uint32_t frame, bool visible,
                   std::uint32_t parent_window) override {
        Check(frame == 0x7000u, "visible frame");
        visibility = visible;
        parent = parent_window;
        visibility_steps.emplace_back("parent");
    }
    std::pair<std::uint32_t, std::uint32_t> CreateEffect(
        std::uint32_t manager, std::string_view name,
        const std::array<float, 3>& position,
        std::uint32_t selector, bool flag) override {
        Check(name == "LH" && selector == 0xffffffffu && !flag,
              "effect creation arguments");
        effect_manager = manager;
        effect_position = position;
        visibility_steps.emplace_back("create-effect");
        return {0x8800u, 0x8804u};
    }
    void ResetTemporaryEffect(std::uint32_t first,
                              std::uint32_t second) override {
        Check(first == 0x8800u && second == 0x8804u,
              "temporary effect handle");
        visibility_steps.emplace_back("reset-effect");
    }
    void SetEffectProperties(std::uint32_t handle,
                             const std::array<float, 3>& scale,
                             bool flag) override {
        Check(handle == 0x7128u && !flag, "effect properties target");
        effect_scale = scale;
        visibility_steps.emplace_back("properties");
    }
    void MarkEffectForUpdate(std::uint32_t handle) override {
        Check(handle == 0x7128u, "effect update target");
        visibility_steps.emplace_back("update-effect");
    }
    void AddAndProcessScript() override {
        visibility_steps.emplace_back("script");
    }
    std::uint32_t CreateGameObject(std::uint32_t collection,
        std::string_view path, std::string_view name, bool flag) override {
        Check(path == "ui\\model\\" && name == "_ui_flower" && flag,
              "flower model arguments");
        flower_collection = collection;
        visibility_steps.emplace_back("create-flower");
        return null_game_object ? 0u : 0xa000u;
    }
    void SetGameObjectScale(std::uint32_t object,
                            const std::array<float, 3>& scale) override {
        Check(object == 0xa000u, "flower scale target");
        flower_scale = scale;
        visibility_steps.emplace_back("scale-flower");
    }
    void SetGameObjectPosition(std::uint32_t object,
        const std::array<float, 3>& position) override {
        Check(object == 0xa000u, "flower position target");
        flower_position = position;
        visibility_steps.emplace_back("position-flower");
    }
    void CleanupScripts() override {
        visibility_steps.emplace_back("cleanup-scripts");
    }
    void SetGameObjectDisplay(std::uint32_t frame,
                              std::uint32_t a, std::uint32_t b,
                              std::uint32_t c) override {
        Check(frame == 0x7000u && a == 0u && b == 0u && c == 0u,
              "hide game object display");
        visibility_steps.emplace_back("hide-objects");
    }
    void RemoveGameObject(std::uint32_t collection,
                          std::uint32_t id) override {
        Check(collection == 0x8500u, "remove collection");
        removed_objects.push_back(id);
        visibility_steps.emplace_back("remove-object");
    }
    void SetWindowText(std::uint32_t window,
                       std::string_view text) override {
        text_window = window;
        visible_text = text;
        visibility_steps.emplace_back("clear-text");
    }
    void HandleEffectEvent(std::uint32_t handle,
                           std::uint32_t event) override {
        effect_handle = handle;
        effect_event = event;
        visibility_steps.emplace_back("effect-event");
    }
    std::uint32_t CopyShopId(std::uint32_t source) override {
        copied_attribute_source = source;
        return 0x6010u;
    }
    void ReleaseShopId(std::uint32_t attribute) override {
        released_attribute = attribute;
    }
    std::uint32_t GameDatabase() override { return 0x80000u; }
    std::uint32_t TradeItemCount(std::uint32_t data) override {
        Check(data == 0x81000u, "trade data identity");
        return static_cast<std::uint32_t>(trade_items.size());
    }
    std::uint32_t TradeItemAt(std::uint32_t data,
                              std::uint32_t index) override {
        Check(data == 0x81000u, "trade item data identity");
        if (throw_on_trade_item) throw std::runtime_error("item");
        return trade_items.at(index);
    }
    bool ShopIdDiffers(std::uint32_t item_text,
                    std::uint32_t attribute) override {
        Check(attribute == 0x6010u, "floor attribute identity");
        return item_text_checks.at(item_text - 0x14u);
    }
    void ClearTradeItems(std::uint32_t frame,
                          std::uint32_t group) override {
        Check(frame == 0x7000u, "clear trade frame");
        cleared_groups.push_back(group);
        words[frame + 0x134u + group * 0x10u] = 0u;
        words[frame + 0x138u + group * 0x10u] = 0u;
    }
    std::uint32_t GameDataOwner() override { return 0x82000u; }
    std::int32_t GameDataValue(std::uint32_t game,
                             std::uint32_t category) override {
        Check(game == 0x82000u, "player game identity");
        return category == 2u ? 0 : 200;
    }
    std::uint32_t ResolveTradeItem(std::uint32_t group,
                                   std::uint32_t id) override {
        Check(group <= 2u, "lookup group");
        return resolved_items.at(id);
    }
    void AppendTradeItem(std::uint32_t frame, std::uint32_t group,
                          std::uint32_t item) override {
        Check(frame == 0x7000u, "append trade frame");
        appended_items.emplace_back(group, item);
        const auto begin_address = frame + 0x134u + group * 0x10u;
        const auto end_address = begin_address + 4u;
        if (words[begin_address] == 0u) {
            words[begin_address] = 0xc000u + group * 0x100u;
            words[end_address] = words[begin_address];
        }
        words[words[end_address]] = item;
        words[end_address] += 4u;
        words[item + (group == 0u ? 0x94u : group == 1u ? 4u : 0x64u)] =
            item;
    }
    void RemoveActiveFrame(std::uint32_t array,
                            std::uint32_t callback) override {
        Check(array == 0x8134u, "active callback array");
        removed_active.push_back(callback);
        binding_calls.emplace_back("remove-active");
    }
    void InsertActiveFrame(std::uint32_t array,
                            std::uint32_t callback) override {
        Check(array == 0x8134u, "active callback array insertion");
        inserted_active.push_back(callback);
    }
    void SetWindowVisible(std::uint32_t window,
                           bool visible) override {
        window_visibility.emplace_back(window, visible);
    }
    void UpdateTradePage(std::uint32_t frame, std::uint32_t group,
                          std::uint32_t a, std::uint32_t b) override {
        Check(frame == 0x7000u && a == 0u && b == 1u,
              "trade page update arguments");
        updated_pages.push_back(group);
    }
    std::uint32_t InventorySubsystem() override {
        ++inventory_calls;
        return 0x83000u;
    }
    std::int32_t InventoryElementSize(std::uint32_t collection,
                                      std::uint32_t index) override {
        Check(collection == 0x8307cu, "shop inventory collection");
        return inventory_sizes[0][index];
    }
    std::int32_t InventoryCalculatedValue(std::uint32_t inventory,
                                          std::uint32_t index) override {
        Check(inventory == 0x83000u, "character inventory collection");
        return inventory_sizes[1][index];
    }
    std::int32_t InventorySubSize(std::uint32_t collection,
                                  std::uint32_t index) override {
        Check(collection == 0x83104u, "passive inventory collection");
        return inventory_sizes[2][index];
    }
    void RemoveObjectByIdentifier(std::uint32_t collection,
                                  std::uint32_t identifier) override {
        Check(collection == 0x8400u && identifier == 0x70fcu,
              "object identifier removal");
        binding_calls.emplace_back("remove-object");
    }
    void RemoveUiElementByIdentifier(std::uint32_t collection,
                                     std::uint32_t identifier) override {
        Check(collection == 0x8400u && identifier == 0x710cu,
              "UI identifier removal");
        binding_calls.emplace_back("remove-ui");
    }
    void AddEventHandler(std::uint32_t collection,
                         std::uint32_t identifier,
                         std::uint32_t delay_bits,
                         std::uint32_t frame,
                         std::uint32_t handler,
                         std::uint32_t event_value,
                         bool flag) override {
        bound_collection = collection;
        bound_identifier = identifier;
        bound_delay = delay_bits;
        bound_frame = frame;
        bound_handler = handler;
        bound_value = event_value;
        bound_flag = flag;
        binding_calls.emplace_back("add-handler");
    }
    std::uint32_t TradeSystem() override {
        binding_calls.emplace_back("trade-system");
        return 0x8600u;
    }
    void ResetTradeSystem(std::uint32_t trade) override {
        Check(trade == 0x8600u, "trade system reset target");
        binding_calls.emplace_back("reset-trade");
    }
    void ProcessUiEvent(std::uint32_t event_tree,
                        std::uint32_t key,
                        std::uint32_t first,
                        std::uint32_t second) override {
        processed_tree = event_tree;
        processed_key = key;
        processed_first = first;
        processed_second = second;
        binding_calls.emplace_back("process-ui");
    }
};

void TestConstruction() {
    Runtime runtime;
    constexpr std::uint32_t frame = 0x7000u;
    Check(pal4::TradeWindow::Construct(frame, 0x6000u, runtime) == frame,
          "constructor result");
    Check(runtime.layout_frame == frame && runtime.layout_name == 0x6000u,
          "layout arguments");
    Check(runtime.words.at(frame) == pal4::TradeWindow::kVtable &&
              runtime.words.at(frame + 0x19cu) == 0x843d68u &&
              runtime.words.at(frame + 0x1a8u) == 0x843d50u &&
              runtime.words.at(frame + 0x1a0u) == frame &&
              runtime.words.at(frame + 0x1acu) == frame,
          "vtable and self pointers");
    Check(runtime.bytes.at(frame + 0xf8u) == 1u &&
              runtime.bytes.at(frame + 0xfcu) == 0u &&
              runtime.words.at(frame + 0x154u) == 0u &&
              runtime.node == frame + 0x128u &&
              runtime.texts == std::vector<std::pair<std::uint32_t, std::string>>{
                  {frame + 0xfcu, "updateTradeItem"},
                  {frame + 0x10cu, "regUpdateEvent"}},
          "initial members and two text values");
    constexpr std::array<std::uint32_t, 5> keys{
        0x8b0a48u, 0x8b0a4cu, 0x8b185cu, 0x8b1860u, 0x8b1864u};
    constexpr std::array<std::uint32_t, 5> handlers{
        0x48bea0u, 0x48c5c0u, 0x48cc20u, 0x48cc80u, 0x48cce0u};
    Check(runtime.events.size() == 5u &&
              runtime.released_callbacks.size() == 5u,
          "five frame event callbacks");
    for (std::size_t i = 0; i < keys.size(); ++i) {
        const auto [tree, key, callback] = runtime.events[i];
        Check(tree == 0x8104u && key == keys[i] &&
                  runtime.words.at(callback) == pal4::TradeWindow::kFrameCallbackVtable &&
                  runtime.words.at(callback + 4u) == handlers[i] &&
                  runtime.words.at(callback + 8u) == frame &&
                  runtime.released_callbacks[i] == callback,
              "frame event identity and release");
    }
    Check(runtime.configured_list == runtime.windows.at("TradeWindow/List") &&
              runtime.removed_columns == 3u &&
              runtime.list_steps == std::vector<std::string>{
                  "sort", "drag", "size", "user-sort", "reset"} &&
              runtime.words.at(frame + 0x28u) == runtime.configured_list,
          "list setup and column removal");
    Check(runtime.words.at(frame + 0x34u) ==
              runtime.windows.at("TradeWindow/StaticMoney") &&
              runtime.horizontal.size() == 2u &&
              runtime.words.at(frame + 0xd8u) ==
              runtime.windows.at("TradeWindow/daoju") &&
              runtime.words.at(frame + 0x4cu) ==
              runtime.windows.at("TradeWindow/BtnHelp"),
          "fixed controls and formatting");
    Check(runtime.words.at(frame + 0x54u) ==
              runtime.windows.at("TradeWindow/ImageRole1") &&
              runtime.words.at(frame + 0xd0u) ==
              runtime.windows.at("TradeWindow/StaticLevel4") &&
              runtime.fonts.size() == 4u &&
              runtime.fonts[0].second == "Level_font" &&
              runtime.formatting.size() == 4u &&
              std::get<1>(runtime.formatting[0]) == 3u &&
              std::get<2>(runtime.formatting[0]) == 2u,
          "four role groups and level formatting");
    Check(runtime.subscriptions.size() == 8u &&
              runtime.released_connections == 8u &&
              std::get<0>(runtime.subscriptions[6]) ==
                  runtime.windows.at("TradeWindow/BtnClose") &&
              runtime.words.at(std::get<2>(runtime.subscriptions[7]) + 4u) ==
                  0x48d710u,
          "eight UI subscriptions and close button");
    Check(runtime.images.size() == 15u &&
              runtime.images[0] == std::pair<std::string, std::string>{"zi0", "wuqi"} &&
              runtime.images[14] == std::pair<std::string, std::string>{"zi0", "duanye"} &&
              runtime.words.at(frame + 0x198u) == 0x3000fu,
          "ordered image references");
    constexpr std::array<std::string_view, 15> image_names{
        "wuqi", "toubu", "shenti", "zhubu", "peidai", "xinzeng",
        "zi", "zi1", "zi2", "zi3", "zi4", "xinzeng", "rongzhu",
        "xinzeng", "duanye"};
    for (std::size_t i = 0; i < image_names.size(); ++i) {
        Check(runtime.images[i].first == "zi0" &&
                  runtime.images[i].second == image_names[i] &&
                  runtime.words.at(frame + 0x160u + static_cast<std::uint32_t>(i) * 4u) ==
                      0x30001u + i,
              "all image names and ordered storage offsets");
    }
    Check(runtime.destroyed_base == 0u, "successful constructor keeps frame");
}

void TestFailures() {
    constexpr std::uint32_t frame = 0x7000u;
    Runtime event_failure;
    event_failure.throw_on_event = 2u;
    try { static_cast<void>(pal4::TradeWindow::Construct(frame, 1u, event_failure)); }
    catch (const std::runtime_error&) {}
    Check(event_failure.released_callbacks.size() == 2u &&
              event_failure.destroyed_base == frame &&
              event_failure.destroyed_node == frame + 0x128u &&
              event_failure.destroyed_texts == std::vector<std::uint32_t>{
                  frame + 0x150u, frame + 0x140u, frame + 0x130u,
                  frame + 0x10cu, frame + 0xfcu},
          "event failure releases callback and unwinds members");

    Runtime text_failure;
    text_failure.throw_on_text = true;
    try { static_cast<void>(pal4::TradeWindow::Construct(frame, 1u, text_failure)); }
    catch (const std::runtime_error&) {}
    Check(text_failure.destroyed_texts ==
              std::vector<std::uint32_t>{frame + 0xfcu} &&
              text_failure.destroyed_node == 0u &&
              text_failure.destroyed_base == frame,
          "first text failure unwinds initialized base members");

    Runtime image_failure;
    image_failure.throw_on_image = true;
    try { static_cast<void>(pal4::TradeWindow::Construct(frame, 1u, image_failure)); }
    catch (const std::runtime_error&) {}
    Check(image_failure.subscriptions.size() == 8u &&
              image_failure.destroyed_base == frame,
          "image failure occurs after subscriptions and unwinds");

    Runtime subscription_failure;
    subscription_failure.throw_on_subscribe = true;
    try { static_cast<void>(pal4::TradeWindow::Construct(frame, 1u, subscription_failure)); }
    catch (const std::runtime_error&) {}
    Check(subscription_failure.released_callbacks.size() == 6u &&
              subscription_failure.subscriptions.empty() &&
              subscription_failure.destroyed_base == frame,
          "failed UI subscription releases its temporary callback");

    Runtime null_callbacks;
    null_callbacks.null_allocation = true;
    null_callbacks.columns = 0u;
    Check(pal4::TradeWindow::Construct(frame, 1u, null_callbacks) == frame &&
              null_callbacks.released_callbacks.empty() &&
              null_callbacks.removed_columns == 0u &&
              std::get<2>(null_callbacks.events[0]) == 0u &&
              std::get<2>(null_callbacks.subscriptions[0]) == 0u,
          "null callback allocation and empty list preserve construction");
}

void TestDestruction() {
    Runtime runtime;
    constexpr std::uint32_t frame = 0x7000u;
    runtime.words[frame + 0x154u] = 0x9100u;
    runtime.words[frame + 0x144u] = 0u;
    runtime.words[frame + 0x134u] = 0x9300u;
    runtime.words[frame + 0x110u] = 0x9501u;
    runtime.words[frame + 0x100u] = 0x9601u;
    runtime.bytes[0x9500u] = 3u;
    runtime.bytes[0x9600u] = 0xffu;
    Check(pal4::TradeWindow::Destroy(frame, runtime) == frame + 1u &&
              runtime.freed == std::vector<std::uint32_t>{
                  0x9100u, 0u, 0x9300u, 0x9600u} &&
              runtime.bytes.at(0x9500u) == 2u &&
              runtime.destroyed_node == frame + 0x128u &&
              runtime.destroyed_base == frame,
          "destructor releases arrays and reference-counted text in order");
    for (const auto offset : {0x134u, 0x138u, 0x13cu, 0x144u, 0x148u,
                              0x14cu, 0x154u, 0x158u, 0x15cu,
                              0x100u, 0x104u, 0x108u,
                              0x110u, 0x114u, 0x118u}) {
        Check(runtime.words.at(frame + offset) == 0u,
              "destructor clears owned pointer and size fields");
    }
}

void TestVisibleBranch() {
    Runtime runtime;
    constexpr std::uint32_t frame = 0x7000u;
    runtime.words[0x819cu] = 0x8500u;
    runtime.words[0xa170u] = 0x5600u;
    pal4::TradeWindow::SetVisible(frame, true, 0x4000u, runtime);
    Check(runtime.visibility && runtime.parent == 0x4000u &&
              runtime.effect_manager == 0x8000u &&
              runtime.effect_position == std::array<float, 3>{
                  -59.669998f, 220.86f, 298.26001f} &&
              runtime.effect_scale == std::array<float, 3>{2.2f, 2.2f, 2.2f} &&
              runtime.flower_position == std::array<float, 3>{
                  -59.669998f, 329.85999f, 298.26001f} &&
              runtime.flower_scale == std::array<float, 3>{3.5f, 3.5f, 3.5f} &&
              runtime.flower_collection == 0x8500u &&
              runtime.words.at(frame + 0x128u) == 0x8800u &&
              runtime.words.at(frame + 0x12cu) == 0x8804u &&
              runtime.words.at(frame + 0x120u) == 0x5600u,
          "visible branch effect and flower values");
    Check(runtime.visibility_steps == std::vector<std::string>{
              "parent", "create-effect", "reset-effect", "properties",
              "update-effect", "script", "create-flower", "scale-flower",
              "position-flower", "cleanup-scripts"},
          "visible branch call order");

    Runtime absent;
    absent.null_game_object = true;
    absent.words[0x819cu] = 0x8500u;
    pal4::TradeWindow::SetVisible(frame, true, 0u, absent);
    Check(absent.words.count(frame + 0x120u) == 0u &&
              absent.visibility_steps.back() == "cleanup-scripts",
          "missing flower still cleans scripts");
}

void TestHiddenBranch() {
    Runtime runtime;
    constexpr std::uint32_t frame = 0x7000u;
    runtime.words[0x819cu] = 0x8500u;
    runtime.words[frame + 0x120u] = 0x5600u;
    runtime.words[frame + 0x124u] = 0x5700u;
    runtime.words[frame + 0x28u] = 0x5800u;
    runtime.words[frame + 0x38u] = 0x5900u;
    pal4::TradeWindow::SetVisible(frame, false, 0x4000u, runtime);
    Check(!runtime.visibility && runtime.parent == 0x4000u &&
              runtime.removed_objects == std::vector<std::uint32_t>{
                  0x5600u, 0x5700u} &&
              runtime.words.at(frame + 0x120u) == 0u &&
              runtime.words.at(frame + 0x124u) == 0u &&
              runtime.text_window == 0x5900u && runtime.visible_text.empty() &&
              runtime.effect_handle == frame + 0x128u &&
              runtime.effect_event == 0u,
          "hidden branch removes objects and clears UI");
    Check(runtime.visibility_steps == std::vector<std::string>{
              "parent", "hide-objects", "remove-object", "remove-object",
              "reset-list", "clear-text", "effect-event"},
          "hidden branch call order");
}

Runtime MakeTradeRuntime(std::uint32_t group) {
    Runtime runtime;
    constexpr std::uint32_t frame = 0x7000u;
    constexpr std::uint32_t event = 0x5000u;
    runtime.words[event] = group;
    runtime.words[event + 4u] = 0x6000u;
    runtime.words[0x80074u] = 0x81000u;
    runtime.bytes[frame + 0xf8u] = 1u;
    runtime.words[frame + 0xd4u] = 0x9000u;
    runtime.words[frame + 0xd8u] = 0x9004u;
    runtime.words[frame + 0x44u] = 0x9008u;
    runtime.words[frame + 0x48u] = 0x900cu;
    for (std::uint32_t i = 0; i < 7; ++i) {
        const auto item = 0xa000u + i * 0x24u;
        runtime.trade_items.push_back(item);
        runtime.words[item + 4u] = i % 3u;
        runtime.words[item + 8u] = 100u + i;
        runtime.words[item + 0xcu] = i;
        runtime.words[item + 0x10u] = 100u;
        runtime.item_text_checks[item] = i == 0u;
        runtime.resolved_items[100u + i] = i == 3u ? 0u : 0xb000u + i;
    }
    return runtime;
}

void TestTradeItemUpdate() {
    constexpr std::uint32_t frame = 0x7000u;
    for (std::uint32_t group = 0; group <= 2; ++group) {
        auto runtime = MakeTradeRuntime(group);
        Check(pal4::TradeWindow::UpdateTradeItems(frame, 0x5000u, runtime),
              "trade update returns true");
        Check(runtime.words.at(frame + 0xe8u) == group &&
                  runtime.copied_attribute_source == 0x6004u &&
                  runtime.released_attribute == 0x6010u &&
                  runtime.cleared_groups == std::vector<std::uint32_t>{group} &&
                  runtime.removed_active == std::vector<std::uint32_t>{
                      frame + 0x19cu, frame + 0x1a8u} &&
                  runtime.inserted_active == std::vector<std::uint32_t>{
                      frame + (group == 1u ? 0x1a8u : 0x19cu)} &&
                  runtime.updated_pages == std::vector<std::uint32_t>{group},
              "group data and callback selection");
        Check(runtime.window_visibility ==
                  std::vector<std::pair<std::uint32_t, bool>>{
                      {0x9000u, group == 0u}, {0x9004u, group != 0u},
                      {0x9008u, group == 1u}, {0x900cu, group == 1u}},
              "page and navigation visibility");
        if (group == 0u) {
            Check(runtime.appended_items ==
                      std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                          {0u, 0xb006u}},
                  "shop filters text, rejects null lookup and keeps item six");
        } else if (group == 1u) {
            Check(runtime.appended_items ==
                      std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                          {1u, 0xb001u}, {1u, 0xb004u}},
                  "character items keep matching affordable entry");
        } else {
            Check(runtime.appended_items ==
                      std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                          {2u, 0xb005u}},
                  "passive items skip unaffordable entry");
        }
    }
}

void TestTradeItemBoundaries() {
    constexpr std::uint32_t frame = 0x7000u;
    auto inactive = MakeTradeRuntime(1u);
    inactive.bytes[frame + 0xf8u] = 0u;
    Check(pal4::TradeWindow::UpdateTradeItems(frame, 0x5000u, inactive) &&
              inactive.copied_attribute_source == 0u &&
              inactive.cleared_groups.empty() &&
              inactive.updated_pages == std::vector<std::uint32_t>{1u},
          "inactive data flag skips item processing but updates UI");

    auto unknown = MakeTradeRuntime(9u);
    Check(pal4::TradeWindow::UpdateTradeItems(frame, 0x5000u, unknown) &&
              unknown.cleared_groups.empty() &&
              unknown.inserted_active.empty() &&
              unknown.window_visibility.size() == 2u &&
              unknown.released_attribute == 0x6010u,
          "unknown group still filters and refreshes trade buttons");

    auto failure = MakeTradeRuntime(0u);
    failure.throw_on_trade_item = true;
    bool threw = false;
    try {
        static_cast<void>(pal4::TradeWindow::UpdateTradeItems(frame, 0x5000u,
                                                               failure));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && failure.released_attribute == 0x6010u &&
              failure.removed_active.empty(),
          "item-read exception releases attribute before UI mutation");
}

void TestDataAndMoneyQueries() {
    Runtime runtime;
    runtime.words[0x80074u] = 0x81000u;
    Check(pal4::TradeWindow::GetDataPtr(0x8006cu, runtime) == 0x81000u,
          "data pointer is third word of database field");
    runtime.words[0xa00cu] = 1u;
    runtime.words[0xa010u] = 201u;
    Check(!pal4::TradeWindow::MeetsOpenCondition(0xa000u, runtime),
          "money below required price");
    runtime.words[0xa010u] = 200u;
    Check(pal4::TradeWindow::MeetsOpenCondition(0xa000u, runtime),
          "money equal to required price");
    runtime.words[0xa00cu] = 2u;
    runtime.words[0xa010u] = 0xffffffffu;
    Check(pal4::TradeWindow::MeetsOpenCondition(0xa000u, runtime),
          "money comparison treats negative required value as signed");
}

void TestTradeQueries() {
    constexpr std::uint32_t frame = 0x7000u;
    for (std::uint32_t group = 0u; group <= 2u; ++group) {
        Runtime runtime;
        runtime.words[frame + 0xe8u] = group;
        runtime.bytes[frame + 0xf8u] = 1u;
        const auto base = frame + 0x130u + group * 0x10u;
        const auto field = group == 0u ? 0x94u : group == 1u ? 4u : 0x64u;
        runtime.words[base + 4u] = 0u;
        runtime.words[base + 8u] = 0u;
        Check(!pal4::TradeWindow::HasMultipleCategories(frame, runtime),
              "null active group cannot execute");
        Check(!pal4::TradeWindow::HasTradeItems(frame, runtime),
              "null active group has no items");
        runtime.words[base + 4u] = 0xc000u;
        runtime.words[base + 8u] = 0xc001u;
        Check(!pal4::TradeWindow::HasTradeItems(frame, runtime),
              "sub-word vector span contains no complete element");
        runtime.words[base + 8u] = 0xc004u;
        runtime.words[0xc000u] = 0xd000u;
        runtime.words[0xd000u + field] = 7u;
        Check(!pal4::TradeWindow::HasMultipleCategories(frame, runtime) &&
                  pal4::TradeWindow::HasTradeItems(frame, runtime),
              "one active item is present but cannot trade");
        runtime.words[base + 8u] = 0xc008u;
        runtime.words[0xc004u] = 0xd200u;
        runtime.words[0xd200u + field] = 7u;
        Check(!pal4::TradeWindow::HasMultipleCategories(frame, runtime),
              "equal active item properties cannot trade");
        runtime.words[0xd200u + field] = 8u;
        Check(pal4::TradeWindow::HasMultipleCategories(frame, runtime),
              "different active item properties permit trade");
    }

    for (std::uint32_t group = 0u; group <= 2u; ++group) {
        Runtime runtime;
        runtime.words[frame + 0xe8u] = group;
        runtime.bytes[frame + 0xf8u] = 0u;
        runtime.inventory_sizes[group][0] = 2;
        Check(!pal4::TradeWindow::HasMultipleCategories(frame, runtime) &&
                  pal4::TradeWindow::HasTradeItems(frame, runtime),
              "one populated inventory slot has items but cannot trade");
        runtime.inventory_sizes[group][1] = 1;
        Check(pal4::TradeWindow::HasMultipleCategories(frame, runtime) &&
                  pal4::TradeWindow::HasTradeItems(frame, runtime),
              "two populated inventory slots permit trade");
        const auto expected = group == 0u ? 7u : group == 1u ? 5u : 3u;
        Check(runtime.inventory_calls == expected * 4u,
              "inventory singleton queried once per slot per call");
    }

    Runtime unknown;
    unknown.words[frame + 0xe8u] = 9u;
    unknown.bytes[frame + 0xf8u] = 1u;
    Check(!pal4::TradeWindow::HasMultipleCategories(frame, unknown) &&
              !pal4::TradeWindow::HasTradeItems(frame, unknown),
          "unsupported trade group has no executable entries");
}

void TestActiveState() {
    Runtime runtime;
    constexpr std::uint32_t frame = 0x7000u;
    runtime.words[frame + 0xdcu] = 0x9100u;
    runtime.words[frame + 0xe0u] = 0x9200u;
    for (const auto [value, expected] :
         {std::pair<std::uint32_t, bool>{0xffffffffu, false},
          {0u, false}, {1u, true}}) {
        runtime.words[0x5000u] = value;
        Check(pal4::TradeWindow::SetActiveState(frame, 0x5000u, runtime) &&
                  runtime.bytes.at(frame + 0xf8u) == (expected ? 1u : 0u) &&
                  runtime.window_visibility[runtime.window_visibility.size() - 2u] ==
                      std::pair<std::uint32_t, bool>{0x9100u, expected} &&
                  runtime.window_visibility.back() ==
                      std::pair<std::uint32_t, bool>{0x9200u, !expected},
              "active state and passive controls are mutually exclusive");
    }
}

void TestListEventRebinding() {
    for (const bool escape : {true, false}) {
        Runtime runtime;
        runtime.words[0x81a0u] = 0x8400u;
        runtime.words[0x5000u] = 0x1234u;
        const auto result = escape
            ? pal4::TradeWindow::OnQuantityDecreaseDown(0x7000u, 0x5000u, runtime)
            : pal4::TradeWindow::OnQuantityIncreaseDown(0x7000u, 0x5000u, runtime);
        Check(result && runtime.binding_calls ==
                  std::vector<std::string>{
                      "manager", "remove-object", "manager", "remove-ui",
                      "manager", "add-handler"} &&
                  runtime.bound_collection == 0x8400u &&
                  runtime.bound_identifier == 0x710cu &&
                  runtime.bound_delay == 0x3e4ccccdu &&
                  runtime.bound_frame == 0x7000u &&
                  runtime.bound_handler == 0x48cd80u &&
                  runtime.bound_value == 0x1234u &&
                  runtime.bound_flag == escape,
              "escape and tab events rebind with distinct final flag");
    }

    Runtime clear;
    clear.words[0x81a0u] = 0x8400u;
    Check(pal4::TradeWindow::OnQuantityButtonUp(0x7000u, clear) &&
              clear.binding_calls == std::vector<std::string>{
                  "manager", "remove-object", "manager", "remove-ui"},
          "button update only removes prior bindings");
}

void TestCloseAndGlobalUpdate() {
    for (const bool thunk : {false, true}) {
        Runtime runtime;
        const auto result = thunk
            ? pal4::TradeWindow::OnCloseThunk(0x7000u, runtime)
            : pal4::TradeWindow::OnClose(0x7000u, runtime);
        Check(result && runtime.removed_active ==
                  std::vector<std::uint32_t>{0x719cu, 0x71a8u} &&
                  runtime.processed_tree == 0x8104u &&
                  runtime.processed_key == 0x8b0980u &&
                  runtime.processed_first == 1u &&
                  runtime.processed_second == 0u &&
                  runtime.binding_calls == std::vector<std::string>{
                      "manager", "remove-active", "manager", "remove-active",
                      "trade-system", "reset-trade", "manager", "process-ui"},
              "close thunk and direct entry remove callbacks and show selector");
    }

    Runtime global;
    Check(pal4::TradeWindow::GlobalUpdateHandler(global) &&
              global.processed_tree == 0x8104u &&
              global.processed_key == 0x8b1c30u &&
              global.processed_first == 1u &&
              global.processed_second == 4u &&
              global.binding_calls == std::vector<std::string>{
                  "manager", "process-ui"},
          "global update emits show-help event and ignores input");
}

}  // namespace

int main() {
    TestConstruction();
    TestFailures();
    TestDestruction();
    TestVisibleBranch();
    TestHiddenBranch();
    TestTradeItemUpdate();
    TestTradeItemBoundaries();
    TestDataAndMoneyQueries();
    TestTradeQueries();
    TestActiveState();
    TestListEventRebinding();
    TestCloseAndGlobalUpdate();
    return EXIT_SUCCESS;
}
