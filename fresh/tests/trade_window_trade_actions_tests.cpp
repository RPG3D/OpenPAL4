#include "pal4/trade_window_trade_actions.h"

#include <algorithm>
#include <cstdlib>
#include <map>
#include <stdexcept>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::ITradeWindowTradeRuntime {
public:
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, float> floats;
    std::vector<std::uint32_t> rows;
    std::map<std::uint32_t, std::int32_t> quantities;
    std::vector<pal4::TradeBatchEntry> batch;
    std::vector<std::string> calls;
    std::uint32_t passive_hover = 0u;
    std::uint32_t item_hover = 0u;
    std::uint32_t inventory_hover = 0u;
    std::uint32_t result = 0u;
    std::uint32_t learned_recipe = 0u;
    std::uint32_t bought_item = 0u;
    std::uint32_t sold_item = 0u;
    std::uint32_t updated_mode = 0xffffffffu;
    std::uint32_t updated_selected = 0u;
    std::uint32_t displayed_item = 0u;
    std::uint32_t trigger = 0u;
    float dialog_scale = 0.0f;
    std::int32_t restored_row = -1;
    float restored_scroll = -1.0f;
    std::string text;
    std::map<std::uint32_t, bool> character_exists;
    std::set<std::uint32_t> missing_inventory_ids;
    std::uint32_t total_window = 0u;
    std::string total_text;
    std::uint32_t buy_item_hover = 0u;
    std::uint32_t buy_inventory_hover = 0u;
    std::uint32_t hover_handler = 0u;
    std::uint32_t hover_item = 0u;
    std::uint32_t adjacent_mode = 0xffffffffu;
    std::uint32_t adjacent_current = 0u;
    bool adjacent_next = false;
    std::uint32_t updated_flag = 0xffffffffu;
    bool recipe_owned = false;
    bool player_condition = false;
    bool null_allocation = false;
    bool throw_dialog = false;
    std::uint32_t released_callback = 0u;
    std::uint32_t dialog_type = 0u;
    std::uint32_t dialog_callback = 0u;
    std::uint32_t timer_collection = 0u;
    std::uint32_t timer_identifier = 0u;
    std::uint32_t timer_delay = 0u;
    std::uint32_t timer_frame = 0u;
    std::uint32_t timer_handler = 0u;
    std::int32_t timer_row = -1;
    std::uint8_t timer_decrement = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write-word");
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    float ReadFloat(std::uint32_t address) override {
        return floats.at(address);
    }
    std::uint32_t GetPassiveFromHover(std::uint32_t frame) override {
        Check(frame == 0x7000u, "passive hover frame");
        calls.emplace_back("passive-hover");
        return passive_hover;
    }
    std::uint32_t GetItemFromHover(std::uint32_t frame) override {
        Check(frame == 0x7000u, "item hover frame");
        calls.emplace_back("item-hover");
        return item_hover;
    }
    std::uint32_t GetInventoryFromHover(std::uint32_t frame) override {
        Check(frame == 0x7000u, "inventory hover frame");
        calls.emplace_back("inventory-hover");
        return inventory_hover;
    }
    std::uint32_t GetBuyItemFromHover(std::uint32_t frame) override {
        Check(frame == 0x7000u, "buy item hover frame");
        calls.emplace_back("buy-item-hover");
        return buy_item_hover;
    }
    std::uint32_t GetBuyInventoryFromHover(std::uint32_t frame) override {
        Check(frame == 0x7000u, "buy inventory hover frame");
        calls.emplace_back("buy-inventory-hover");
        return buy_inventory_hover;
    }
    void InvokeHoverHandler(std::uint32_t address,
                            std::uint32_t frame,
                            std::uint32_t item) override {
        Check(frame == 0x7000u, "hover handler frame");
        hover_handler = address;
        hover_item = item;
        calls.emplace_back("hover-handler");
    }
    std::uint32_t TradeSystem() override {
        calls.emplace_back("trade-system");
        return 0x9000u;
    }
    std::uint32_t LearnRecipe(std::uint32_t trade,
                              std::uint32_t recipe) override {
        Check(trade == 0x9000u, "recipe trade system");
        learned_recipe = recipe;
        calls.emplace_back("learn");
        return result;
    }
    std::uint32_t BuyItem(std::uint32_t trade,
                          std::uint32_t item) override {
        Check(trade == 0x9000u, "buy trade system");
        bought_item = item;
        calls.emplace_back("buy");
        return result;
    }
    void SellItem(std::uint32_t trade, std::uint32_t item) override {
        Check(trade == 0x9000u, "sell trade system");
        sold_item = item;
        calls.emplace_back("sell");
    }
    std::uint32_t AddMultipleItems(std::uint32_t trade,
        const std::vector<pal4::TradeBatchEntry>& items) override {
        Check(trade == 0x9000u, "add trade system");
        batch = items;
        calls.emplace_back("add-multiple");
        return result;
    }
    std::uint32_t RemoveMultipleItems(std::uint32_t trade,
        const std::vector<pal4::TradeBatchEntry>& items) override {
        Check(trade == 0x9000u, "remove trade system");
        batch = items;
        calls.emplace_back("remove-multiple");
        return result;
    }
    std::int32_t RowCount(std::uint32_t list) override {
        Check(list == 0x8000u, "row count list");
        calls.emplace_back("row-count");
        return static_cast<std::int32_t>(rows.size());
    }
    std::uint32_t GridItem(std::uint32_t list, std::int32_t row,
                            std::int32_t column) override {
        Check(list == 0x8000u && column == 1 && row >= 0,
              "grid reference column");
        calls.emplace_back("grid-item");
        return rows.at(static_cast<std::size_t>(row));
    }
    std::int32_t SpinnerValue(std::uint32_t item) override {
        calls.emplace_back("spinner");
        return quantities.at(item);
    }
    void SetSpinnerValue(std::uint32_t item,
                         std::int32_t value) override {
        quantities[item] = value;
        calls.emplace_back("set-spinner");
    }
    std::uint32_t GameDatabase() override {
        calls.emplace_back("database");
        return 0xa000u;
    }
    std::uint32_t ResolveCharacterItem(std::uint32_t database,
                                        std::uint32_t id) override {
        Check(database == 0xa000u, "character database");
        calls.emplace_back("character-item");
        return 0xb000u + id * 8u;
    }
    bool ContainsCharacterItem(std::uint32_t database,
                                std::uint32_t id) override {
        Check(database == 0xa000u, "character lookup database");
        calls.emplace_back("contains-character");
        return character_exists.at(id);
    }
    std::uint32_t InventorySubsystem() override {
        calls.emplace_back("inventory");
        return 0xc000u;
    }
    std::uint32_t FindInventoryEntry(std::uint32_t inventory,
        std::uint32_t id, std::uint32_t selector) override {
        Check(inventory == 0xc000u && selector == 5u,
              "inventory entry selector");
        calls.emplace_back("inventory-item");
        if (missing_inventory_ids.contains(id)) return 0u;
        return 0xd000u + id * 8u;
    }
    void InsertBatchRecord(std::vector<pal4::TradeBatchEntry>& items,
                            pal4::TradeBatchEntry entry) override {
        calls.emplace_back("insert-record");
        items.push_back(entry);
    }
    void UpdatePage(std::uint32_t frame, std::uint32_t mode,
                    std::uint32_t selected, std::uint32_t flag) override {
        Check(frame == 0x7000u && flag <= 1u, "update page arguments");
        updated_mode = mode;
        updated_selected = selected;
        updated_flag = flag;
        calls.emplace_back("update-page");
    }
    void UpdateItemDisplay(std::uint32_t frame,
                            std::uint32_t item) override {
        Check(frame == 0x7000u, "item display frame");
        displayed_item = item;
        calls.emplace_back("update-display");
    }
    void SetHoverRowIndex(std::uint32_t list,
                           std::int32_t row) override {
        Check(list == 0x8000u, "restore hover list");
        restored_row = row;
        calls.emplace_back("restore-row");
    }
    void SetScrollPosition(std::uint32_t scrollbar,
                            float position) override {
        Check(scrollbar == 0x8100u, "restore scrollbar");
        restored_scroll = position;
        calls.emplace_back("restore-scroll");
    }
    void CreateCommonDialog(std::uint32_t manager) override {
        Check(manager == 0x8200u, "common dialog manager");
        calls.emplace_back("create-dialog");
    }
    std::string_view TriggerText(std::uint32_t id) override {
        trigger = id;
        text = "trigger-" + std::to_string(id);
        calls.emplace_back("trigger-text");
        return text;
    }
    void ShowDialog(std::string_view value, float x, float y,
                    std::uint32_t type, std::uint32_t flags) override {
        Check(value == text && x == y &&
                  ((type == 4u && flags == 0u) || type == 3u),
              "dialog text and options");
        dialog_scale = x;
        dialog_type = type;
        dialog_callback = flags;
        calls.emplace_back("show-dialog");
        if (throw_dialog) throw std::runtime_error("dialog");
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("frame-manager");
        return 0x8200u;
    }
    void RemoveObjectByIdentifier(std::uint32_t collection,
                                   std::uint32_t identifier) override {
        Check(collection == 0x8400u && identifier == 0x70fcu,
              "timer removal collection and identifier");
        calls.emplace_back("remove-timer");
    }
    void AddTimerHandler(std::uint32_t collection,
                         std::uint32_t identifier,
                         std::uint32_t delay_bits,
                         std::uint32_t frame,
                         std::uint32_t handler,
                         std::int32_t row,
                         std::uint8_t decrement) override {
        timer_collection = collection;
        timer_identifier = identifier;
        timer_delay = delay_bits;
        timer_frame = frame;
        timer_handler = handler;
        timer_row = row;
        timer_decrement = decrement;
        calls.emplace_back("add-timer");
    }
    void SetWindowText(std::uint32_t window,
                       std::string_view value) override {
        total_window = window;
        total_text = value;
        calls.emplace_back("total-text");
    }
    std::uint32_t AdjacentIndex(std::uint32_t mode,
                                std::uint32_t current,
                                bool next) override {
        adjacent_mode = mode;
        adjacent_current = current;
        adjacent_next = next;
        calls.emplace_back("adjacent");
        return next ? current + 2u : current - 2u;
    }
    bool HasInventoryRecipe(std::uint32_t collection,
                            std::uint32_t recipe) override {
        Check(collection == 0xc104u && recipe == 0x1234u,
              "recipe inventory query");
        calls.emplace_back("has-recipe");
        return recipe_owned;
    }
    bool CheckPlayersCondition(std::uint32_t item) override {
        Check(item == inventory_hover, "player condition item");
        calls.emplace_back("player-condition");
        return player_condition;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 12u, "prompt callback allocation size");
        calls.emplace_back("allocate");
        return null_allocation ? 0u : 0xa000u;
    }
    void ReleaseCallback(std::uint32_t callback) override {
        released_callback = callback;
        calls.emplace_back("release-callback");
    }
};

Runtime BaseRuntime(std::uint32_t mode, bool active) {
    Runtime runtime;
    constexpr std::uint32_t frame = 0x7000u;
    runtime.words[frame + 0xe8u] = mode;
    runtime.bytes[frame + 0xf8u] = active ? 1u : 0u;
    runtime.words[frame + 0x28u] = 0x8000u;
    runtime.words[frame + 0xecu] = 11u;
    runtime.words[frame + 0xf0u] = 12u;
    runtime.words[frame + 0xf4u] = 13u;
    runtime.words[0x8000u + 0x4c0u] = 3u;
    runtime.words[0x8000u + 0x4b4u] = 0x8100u;
    runtime.floats[0x8100u + 0x4c0u] = 7.25f;
    return runtime;
}

std::size_t Position(const Runtime& runtime, std::string_view action) {
    const auto found = std::find(runtime.calls.begin(), runtime.calls.end(),
                                 action);
    Check(found != runtime.calls.end(), "expected call missing");
    return static_cast<std::size_t>(found - runtime.calls.begin());
}

void CheckSuccessDialog(const Runtime& runtime) {
    Check(runtime.trigger == 0x108u && runtime.dialog_scale == 0.5f &&
              Position(runtime, "create-dialog") <
                  Position(runtime, "trigger-text"),
          "success dialog trigger, scale and creation order");
}

void CheckErrorDialog(const Runtime& runtime, std::uint32_t error,
                      bool create_first) {
    Check(runtime.trigger == error && runtime.dialog_scale == 0.3f &&
              (Position(runtime, "create-dialog") <
               Position(runtime, "trigger-text")) == create_first,
          "error dialog trigger, scale and creation order");
}

void CheckRestoredList(const Runtime& runtime, std::uint32_t mode,
                       std::uint32_t selected) {
    Check(runtime.updated_mode == mode &&
              runtime.updated_selected == selected &&
              runtime.restored_row == 3 && runtime.restored_scroll == 7.25f &&
              Position(runtime, "update-page") < Position(runtime, "restore-row") &&
              Position(runtime, "restore-row") < Position(runtime, "restore-scroll"),
          "list row and scroll restored after page update");
}

void TestNoAction() {
    Runtime runtime;
    Check(pal4::TradeWindowTradeActions::OnTradeClick(0x7000u, 0u, runtime) &&
              runtime.calls.empty(), "other event is ignored");
    for (const bool active : {false, true}) {
        auto unknown = BaseRuntime(9u, active);
        Check(pal4::TradeWindowTradeActions::OnTradeClick(0x7000u, 1u,
                                                           unknown) &&
                  unknown.calls.empty(), "unknown trade mode is ignored");
    }
}

void TestPassiveRecipe() {
    auto success = BaseRuntime(2u, true);
    success.passive_hover = 0x5000u;
    success.words[0x5000u] = 0x1234u;
    Check(pal4::TradeWindowTradeActions::OnTradeClick(0x7000u, 1u, success) &&
              success.learned_recipe == 0x1234u,
          "passive recipe uses hovered item pointer");
    CheckRestoredList(success, 2u, 13u);
    CheckSuccessDialog(success);
    Check(Position(success, "restore-scroll") <
              Position(success, "create-dialog"),
          "passive success restores list before dialog");

    auto failure = BaseRuntime(2u, true);
    failure.passive_hover = 0x5000u;
    failure.words[0x5000u] = 0x1234u;
    failure.result = 0x209u;
    Check(pal4::TradeWindowTradeActions::OnTradeClick(0x7000u, 1u, failure) &&
              failure.updated_mode == 0xffffffffu,
          "failed recipe skips page refresh");
    CheckErrorDialog(failure, 0x209u, false);

    auto empty = BaseRuntime(2u, true);
    Check(pal4::TradeWindowTradeActions::OnTradeClick(0x7000u, 1u, empty) &&
              empty.calls == std::vector<std::string>{"passive-hover"},
          "missing hovered recipe does nothing");
}

void PopulateRows(Runtime& runtime) {
    runtime.rows = {0x5000u, 0x5100u, 0x5200u};
    runtime.quantities[0x5000u] = 2;
    runtime.quantities[0x5100u] = 0;
    runtime.quantities[0x5200u] = 4;
    runtime.words[0x5000u + 0x138u] = 7u;
    runtime.words[0x5200u + 0x138u] = 8u;
    runtime.words[0xb000u + 7u * 8u] = 0x1700u;
    runtime.words[0xb000u + 8u * 8u] = 0x1800u;
    runtime.words[0xd000u + 7u * 8u + 4u] = 0x2700u;
    runtime.words[0xd000u + 8u * 8u + 4u] = 0x2800u;
}

void TestCharacterBatch() {
    for (const bool active : {true, false}) {
        auto success = BaseRuntime(1u, active);
        PopulateRows(success);
        Check(pal4::TradeWindowTradeActions::OnTradeClick(0x7000u, 1u,
                                                           success) &&
                  success.batch.size() == 2u &&
                  success.batch[0].item == (active ? 0x1700u : 0x2700u) &&
                  success.batch[0].quantity == 2 &&
                  success.batch[1].item == (active ? 0x1800u : 0x2800u) &&
                  success.batch[1].quantity == 4,
              "batch selects only positive spinner quantities");
        if(active)CheckRestoredList(success, 1u, 12u);
        else Check(success.updated_mode==1u && success.updated_selected==12u &&
                       success.restored_row==-1 && success.restored_scroll==-1.0f,
                   "IDA property sale rebuilds page without restoring removed row or scroll");
        CheckSuccessDialog(success);
        Check(Position(success, active ? "add-multiple" : "remove-multiple") <
                  Position(success, "update-page"),
              "batch inventory operation precedes refresh");

        auto failure = BaseRuntime(1u, active);
        PopulateRows(failure);
        failure.result = 0x20au;
        Check(pal4::TradeWindowTradeActions::OnTradeClick(0x7000u, 1u,
                                                           failure) &&
                  failure.updated_mode == 0xffffffffu,
              "failed batch skips page refresh");
        CheckErrorDialog(failure, 0x20au, false);
    }
}

void TestItemPurchase() {
    auto success = BaseRuntime(0u, true);
    success.item_hover = 0x5300u;
    success.words[0x5300u + 0x90u] = 0x345u;
    Check(pal4::TradeWindowTradeActions::OnTradeClick(0x7000u, 1u, success) &&
              success.bought_item == 0x345u &&
              success.displayed_item == 0x5300u,
          "buy item id and display update");
    CheckSuccessDialog(success);
    CheckRestoredList(success, 0u, 11u);
    Check(Position(success, "update-display") <
              Position(success, "create-dialog") &&
              Position(success, "show-dialog") <
              Position(success, "update-page"),
          "buy display, dialog and list refresh order");

    auto failure = BaseRuntime(0u, true);
    failure.item_hover = 0x5300u;
    failure.words[0x5300u + 0x90u] = 0x345u;
    failure.result = 0x20bu;
    Check(pal4::TradeWindowTradeActions::OnTradeClick(0x7000u, 1u, failure) &&
              failure.displayed_item == 0u,
          "failed buy skips item display update");
    CheckErrorDialog(failure, 0x20bu, true);
    CheckRestoredList(failure, 0u, 11u);

    auto empty = BaseRuntime(0u, true);
    Check(pal4::TradeWindowTradeActions::OnTradeClick(0x7000u, 1u, empty) &&
              empty.calls == std::vector<std::string>{"item-hover"},
          "missing hovered item does nothing");
}

void TestItemSale() {
    auto sale = BaseRuntime(0u, false);
    sale.inventory_hover = 0x5400u;
    sale.words[0x5400u + 0x190u] = 0x456u;
    Check(pal4::TradeWindowTradeActions::OnTradeClick(0x7000u, 1u, sale) &&
              sale.sold_item == 0x456u && sale.updated_mode == 0u &&
              sale.updated_selected == 11u,
          "sale uses inventory item and refreshes shop");
    CheckSuccessDialog(sale);
    Check(Position(sale, "sell") < Position(sale, "update-page") &&
              Position(sale, "update-page") < Position(sale, "create-dialog") &&
              sale.restored_row == -1,
          "sale refreshes before dialog without restoring list state");

    auto empty = BaseRuntime(0u, false);
    Check(pal4::TradeWindowTradeActions::OnTradeClick(0x7000u, 1u, empty) &&
              empty.calls == std::vector<std::string>{"inventory-hover"},
          "missing inventory hover does nothing");
}

void TestItemCountAndTimer() {
    auto overflow = BaseRuntime(1u, true);
    overflow.rows={0x5100u};overflow.quantities[0x5100u]=2147483647;
    pal4::TradeWindowTradeActions::ModifyItemCount(0x7000u,0,0u,overflow);
    Check(overflow.quantities.at(0x5100u)==(-2147483647-1),"repeat increment preserves original 32-bit wrap without signed C++ overflow");
    pal4::TradeWindowTradeActions::ModifyItemCount(0x7000u,0,1u,overflow);
    Check(overflow.quantities.at(0x5100u)==2147483647,"repeat decrement preserves original 32-bit wrap");
    for (const bool decrement : {false, true}) {
        auto runtime = BaseRuntime(1u, true);
        runtime.rows = {0x5000u, 0x5100u};
        runtime.quantities[0x5100u] = 0;
        pal4::TradeWindowTradeActions::ModifyItemCount(
            0x7000u, 1, decrement ? 1u : 0u, runtime);
        Check(runtime.quantities.at(0x5100u) == (decrement ? -1 : 1) &&
                  runtime.restored_row == 1 &&
                  Position(runtime, "set-spinner") <
                      Position(runtime, "restore-row"),
              "spinner increments or decrements before hover restoration");
    }

    auto runtime = BaseRuntime(1u, true);
    runtime.words[0x83a0u] = 0x8400u;
    pal4::TradeWindowTradeActions::OnListClick(0x7000u, 7, 0xffu, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "frame-manager", "remove-timer", "frame-manager", "add-timer"} &&
              runtime.timer_collection == 0x8400u &&
              runtime.timer_identifier == 0x70fcu &&
              runtime.timer_delay == 0x3dcccccdu &&
              runtime.timer_frame == 0x7000u &&
              runtime.timer_handler == 0x48cd20u &&
              runtime.timer_row == 7 && runtime.timer_decrement == 0xffu,
          "list click replaces delayed quantity handler and forwards byte flag");
}

void TestTotalPrice() {
    auto other = BaseRuntime(0u, true);
    pal4::TradeWindowTradeActions::UpdateTotalPrice(0x7000u, other);
    Check(other.calls.empty(), "total price applies only to mode one");

    auto empty = BaseRuntime(1u, true);
    empty.words[0x7000u + 0x38u] = 0x8300u;
    pal4::TradeWindowTradeActions::UpdateTotalPrice(0x7000u, empty);
    Check(empty.total_window == 0x8300u && empty.total_text == "0",
          "empty list has deterministic zero total");

    auto active = BaseRuntime(1u, true);
    active.words[0x7000u + 0x38u] = 0x8300u;
    active.rows = {0x5000u, 0x5100u};
    active.words[0x5000u + 0x138u] = 7u;
    active.words[0x5100u + 0x138u] = 8u;
    active.quantities[0x5000u] = 2;
    active.character_exists[7u] = true;
    active.character_exists[8u] = false;
    active.words[0xb000u + 7u * 8u + 0x90u] = 50u;
    pal4::TradeWindowTradeActions::UpdateTotalPrice(0x7000u, active);
    Check(active.total_window == 0x8300u && active.total_text == "100" &&
              std::count(active.calls.begin(), active.calls.end(),
                         "contains-character") == 2 &&
              std::count(active.calls.begin(), active.calls.end(),
                         "character-item") == 1,
          "active total multiplies only existing database items");

    auto inactive = BaseRuntime(1u, false);
    inactive.words[0x7000u + 0x38u] = 0x8300u;
    inactive.rows = {0x5000u, 0x5100u};
    inactive.words[0x5000u + 0x138u] = 7u;
    inactive.words[0x5100u + 0x138u] = 8u;
    inactive.quantities[0x5000u] = 2;
    inactive.missing_inventory_ids.insert(8u);
    inactive.words[0xd000u + 7u * 8u] = 0xe000u;
    inactive.words[0xe000u + 0x90u] = 101u;
    pal4::TradeWindowTradeActions::UpdateTotalPrice(0x7000u, inactive);
    Check(inactive.total_text == "50" &&
              std::count(inactive.calls.begin(), inactive.calls.end(),
                         "inventory-item") == 2,
          "inactive total quarters item value and skips missing inventory");

    inactive.words[0xe000u + 0x90u] = 0xfffffffbu;
    pal4::TradeWindowTradeActions::UpdateTotalPrice(0x7000u, inactive);
    Check(inactive.total_text == "-2",
          "negative value divides toward zero before multiplication");

    active.rows = {0x5000u};
    active.words[0xb000u + 7u * 8u + 0x90u] = 0x7fffffffu;
    pal4::TradeWindowTradeActions::UpdateTotalPrice(0x7000u, active);
    Check(active.total_text == "-2", "total uses 32-bit wrapping arithmetic");
}

void TestHoverDispatch() {
    struct Case {
        std::uint32_t mode;
        bool active;
        std::uint32_t handler;
    };
    for (const auto test : {
         Case{0u, true, 0u}, Case{1u, true, 0x496580u},
         Case{2u, true, 0x493910u}, Case{0u, false, 0x490000u},
         Case{1u, false, 0x496200u}, Case{2u, false, 0u},
         Case{9u, true, 0u}}) {
        auto runtime = BaseRuntime(test.mode, test.active);
        runtime.words[0x7000u + 0x38u] = 0x8300u;
        runtime.item_hover = 0x5500u;
        runtime.buy_item_hover = 0x5504u;
        runtime.passive_hover = 0x5508u;
        runtime.inventory_hover = 0x550cu;
        runtime.buy_inventory_hover = 0x5510u;
        Check(pal4::TradeWindowTradeActions::OnHoverChanged(0x7000u,
                                                             runtime),
              "hover dispatch returns true");
        Check(runtime.hover_handler == test.handler &&
                  runtime.displayed_item ==
                      (test.mode == 0u && test.active ? 0x5500u : 0u),
              "hover handler selected by mode and active state");
        if (test.handler != 0u) {
            const auto expected = test.mode == 0u ? 0x550cu
                : test.mode == 1u && test.active ? 0x5504u
                : test.mode == 1u ? 0x5510u : 0x5508u;
            Check(runtime.hover_item == expected,
                  "hover handler receives matching hovered item");
        }
        Check(runtime.total_text == (test.mode == 1u ? "0" : ""),
              "hover always delegates to mode-gated total update");
    }
}

void TestNavigation() {
    for (std::uint32_t mode = 0u; mode <= 2u; ++mode) {
        for (const bool next : {false, true}) {
            auto runtime = BaseRuntime(mode, true);
            const auto offset = mode == 0u ? 0xecu : mode == 1u ? 0xf0u : 0xf4u;
            const auto current = runtime.words.at(0x7000u + offset);
            const auto result = next
                ? pal4::TradeWindowTradeActions::OnNextButton(0x7000u, runtime)
                : pal4::TradeWindowTradeActions::OnPrevButton(0x7000u, runtime);
            Check(result && runtime.adjacent_mode == mode &&
                      runtime.adjacent_current == current &&
                      runtime.adjacent_next == next &&
                      runtime.words.at(0x7000u + offset) ==
                          (next ? current + 2u : current - 2u) &&
                      runtime.updated_mode == mode &&
                      runtime.updated_selected == runtime.words.at(0x7000u + offset) &&
                      runtime.updated_flag == (next ? 1u : 0u) &&
                      runtime.calls == std::vector<std::string>{
                          "adjacent", "write-word", "update-page"},
                  "navigation writes next index before page refresh");
        }
    }
    auto unknown = BaseRuntime(9u, true);
    Check(pal4::TradeWindowTradeActions::OnNextButton(0x7000u, unknown) &&
              pal4::TradeWindowTradeActions::OnPrevButton(0x7000u, unknown) &&
              unknown.calls.empty(), "unknown mode ignores navigation");
}

void TestPromptWithoutAction() {
    auto empty = BaseRuntime(0u, true);
    Check(pal4::TradeWindowTradeActions::ShowTradePrompt(0x7000u, empty) &&
              empty.calls == std::vector<std::string>{"row-count"},
          "empty list does not create a dialog");

    for (const bool active : {false, true}) {
        auto no_quantity = BaseRuntime(1u, active);
        no_quantity.rows = {0x5000u, 0x5100u};
        no_quantity.quantities[0x5000u] = 0;
        no_quantity.quantities[0x5100u] = -1;
        Check(pal4::TradeWindowTradeActions::ShowTradePrompt(0x7000u,
                                                              no_quantity) &&
                  no_quantity.trigger == (active ? 0x76u : 0x77u) &&
                  no_quantity.dialog_type == 4u &&
                  no_quantity.dialog_scale == 0.5f &&
                  no_quantity.released_callback == 0u,
              "character prompt requires positive spinner quantity");
    }

    auto owned = BaseRuntime(2u, true);
    owned.rows = {0x5000u};
    owned.passive_hover = 0x5500u;
    owned.words[0x5500u] = 0x1234u;
    owned.recipe_owned = true;
    Check(pal4::TradeWindowTradeActions::ShowTradePrompt(0x7000u, owned) &&
              owned.trigger == 0xcbu && owned.dialog_type == 4u &&
              owned.dialog_scale == 0.5f &&
              Position(owned, "create-dialog") <
                  Position(owned, "has-recipe"),
          "owned recipe shows prior-ownership text");

    auto blocked_sale = BaseRuntime(0u, false);
    blocked_sale.rows = {0x5000u};
    blocked_sale.inventory_hover = 0x5500u;
    blocked_sale.player_condition = true;
    Check(pal4::TradeWindowTradeActions::ShowTradePrompt(0x7000u,
                                                          blocked_sale) &&
              blocked_sale.trigger == 0x104u &&
              blocked_sale.dialog_type == 4u &&
              blocked_sale.dialog_scale == 0.5f,
          "sale with player condition uses nonconfirmation text");
}

void CheckConfirmation(const Runtime& runtime) {
    Check(runtime.trigger == 0x2cu && runtime.dialog_type == 3u &&
              runtime.dialog_scale == 0.3f &&
              runtime.dialog_callback == 0xa000u &&
              runtime.words.at(0xa000u) == 0x843d80u &&
              runtime.words.at(0xa004u) == 0x48c600u &&
              runtime.words.at(0xa008u) == 0x7000u &&
              runtime.released_callback == 0xa000u &&
              Position(runtime, "allocate") <
                  Position(runtime, "show-dialog") &&
              Position(runtime, "show-dialog") <
                  Position(runtime, "release-callback"),
          "confirmation callback layout, dialog arguments and release");
}

void TestConfirmations() {
    auto purchase = BaseRuntime(0u, true);
    purchase.rows = {0x5000u};
    Check(pal4::TradeWindowTradeActions::ShowTradePrompt(0x7000u,
                                                          purchase),
          "purchase confirmation returns true");
    CheckConfirmation(purchase);

    auto sale = BaseRuntime(0u, false);
    sale.rows = {0x5000u};
    sale.inventory_hover = 0x5500u;
    Check(pal4::TradeWindowTradeActions::ShowTradePrompt(0x7000u, sale),
          "sale without player condition reaches confirmation");
    CheckConfirmation(sale);

    auto unowned_recipe = BaseRuntime(2u, true);
    unowned_recipe.rows = {0x5000u};
    unowned_recipe.passive_hover = 0x5500u;
    unowned_recipe.words[0x5500u] = 0x1234u;
    Check(pal4::TradeWindowTradeActions::ShowTradePrompt(0x7000u,
                                                          unowned_recipe),
          "unowned recipe reaches confirmation");
    CheckConfirmation(unowned_recipe);

    auto selected = BaseRuntime(1u, true);
    selected.rows = {0x5000u};
    selected.quantities[0x5000u] = 1;
    Check(pal4::TradeWindowTradeActions::ShowTradePrompt(0x7000u, selected),
          "positive quantity reaches confirmation");
    CheckConfirmation(selected);

    auto unsupported = BaseRuntime(9u, true);
    unsupported.rows = {0x5000u};
    Check(pal4::TradeWindowTradeActions::ShowTradePrompt(0x7000u,
                                                          unsupported),
          "unsupported mode follows default confirmation path");
    CheckConfirmation(unsupported);

    auto null_callback = BaseRuntime(0u, true);
    null_callback.rows = {0x5000u};
    null_callback.null_allocation = true;
    Check(pal4::TradeWindowTradeActions::ShowTradePrompt(0x7000u,
                                                          null_callback) &&
              null_callback.dialog_callback == 0u &&
              null_callback.released_callback == 0u &&
              std::find(null_callback.calls.begin(), null_callback.calls.end(),
                        "release-callback") == null_callback.calls.end(),
          "null allocation still shows prompt without callback release");

    auto failure = BaseRuntime(0u, true);
    failure.rows = {0x5000u};
    failure.throw_dialog = true;
    bool caught = false;
    try {
        static_cast<void>(pal4::TradeWindowTradeActions::ShowTradePrompt(
            0x7000u, failure));
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && failure.released_callback == 0xa000u,
          "dialog exception releases temporary callback");
}

void TestPromptEntryPoints() {
    auto ignored = BaseRuntime(1u, true);
    ignored.words[0x5000u + 0x1cu] = 0u;
    Check(pal4::TradeWindowTradeActions::OnItemHover(0x7000u, 0x5000u,
                                                      ignored) &&
              ignored.calls.empty(),
          "mode one hover does not open prompt");
    ignored.words[0x5000u + 0x1cu] = 1u;
    ignored.words[0x7000u + 0xe8u] = 0u;
    Check(pal4::TradeWindowTradeActions::OnItemHover(0x7000u, 0x5000u,
                                                      ignored) &&
              ignored.calls.empty(),
          "nonzero hover event field does not open prompt");

    for (const auto mode : {0u, 2u}) {
        auto hover = BaseRuntime(mode, false);
        hover.rows = {0x5100u};
        hover.words[0x5000u + 0x1cu] = 0u;
        Check(pal4::TradeWindowTradeActions::OnItemHover(0x7000u, 0x5000u,
                                                          hover),
              "eligible hover opens prompt");
        CheckConfirmation(hover);
    }

    auto tooltip = BaseRuntime(0u, true);
    tooltip.rows = {0x5000u};
    Check(pal4::TradeWindowTradeActions::OnItemTooltip(0x7000u, tooltip),
          "tooltip forwards unconditionally to prompt");
    CheckConfirmation(tooltip);
}

}  // namespace

int main() {
    TestNoAction();
    TestPassiveRecipe();
    TestCharacterBatch();
    TestItemPurchase();
    TestItemSale();
    TestItemCountAndTimer();
    TestTotalPrice();
    TestHoverDispatch();
    TestNavigation();
    TestPromptWithoutAction();
    TestConfirmations();
    TestPromptEntryPoints();
    return EXIT_SUCCESS;
}
