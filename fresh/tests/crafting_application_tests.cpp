#include "pal4/crafting_application.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICraftingApplicationRuntime {
    static constexpr std::uint32_t recipe_data = 0x2000u;
    static constexpr std::uint32_t object = 0x3000u;
    static constexpr std::uint32_t selected_entry = 0x4000u;
    static constexpr std::uint32_t created_entry = 0x5000u;

    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::set<std::uint32_t> direct_items{10u};
    std::vector<std::string> calls;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> removed_direct;
    bool player_condition = true;
    bool selected_present = true;
    int created_count = 0;
    int removed_selected_count = 0;
    int fees = 0;

    Runtime() {
        Set(0, 10u, 2);
        Set(1, 20u, 3);
        Set(2, 10u, 9);
        Set(3, 30u, 0);
        words[recipe_data + 0x68u] = 77u;
        words[selected_entry + 0x90u] = 20u;
        words[created_entry + 0x190u] = 44u;
        bytes[object] = 0u;
    }
    void Set(std::uint32_t index, std::uint32_t item,
             std::int32_t required) {
        words[recipe_data + 0x98u + index * 4u] = item;
        words[recipe_data + 0xa8u + index * 4u] =
            static_cast<std::uint32_t>(required);
    }
    void PlayMusic(std::string_view name, std::uint32_t mode) override {
        Check(name == "wa09.wav" && mode == 1u, "crafting sound");
        calls.push_back("music");
    }
    std::uint32_t GameDb() override {
        calls.push_back("db");
        return 0x1000u;
    }
    std::uint32_t FindRecipe(std::uint32_t collection,
                             std::uint32_t recipe) override {
        Check(collection == 0x1080u && recipe == 88u,
              "recipe lookup arguments");
        calls.push_back("recipe");
        return recipe_data;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == object, "crafting object flag address");
        calls.push_back("flag");
        return bytes.at(address);
    }
    bool IsDirectItem(std::uint32_t database,
                       std::uint32_t item) override {
        Check(database == 0x1000u, "direct item database");
        calls.push_back("direct? " + std::to_string(item));
        return direct_items.contains(item);
    }
    void FindOrLogDirectItem(std::uint32_t database,
                             std::uint32_t item) override {
        Check(database == 0x1000u && direct_items.contains(item),
              "direct item logging");
        calls.push_back("log " + std::to_string(item));
    }
    std::uint32_t Inventory() override {
        calls.push_back("inventory");
        return 0x6000u;
    }
    void RemoveDirectItem(std::uint32_t inventory,
                          std::uint32_t item,
                          std::uint32_t quantity) override {
        Check(inventory == 0x6000u, "direct item inventory");
        calls.push_back("remove " + std::to_string(item));
        removed_direct.emplace_back(item, quantity);
    }
    std::uint32_t FindInventoryEntry(std::uint32_t collection,
                                     std::uint32_t key,
                                     std::uint32_t category) override {
        Check(collection == 0x607cu && key == 99u && category == 7u,
              "selected entry lookup arguments");
        calls.push_back("selected");
        return selected_present ? selected_entry : 0u;
    }
    std::uint32_t ProcessGameDataCollection(
        std::uint32_t collection, std::uint32_t property) override {
        Check(collection == 0x607cu && property == 77u,
              "product creation arguments");
        calls.push_back("create");
        ++created_count;
        return created_entry;
    }
    bool CheckPlayerCondition(std::uint32_t entry) override {
        Check(entry == selected_entry, "player condition entry");
        calls.push_back("condition");
        return player_condition;
    }
    void ManagePlayerItem(std::uint32_t owner,
                          std::uint32_t item,
                          std::uint32_t result_item) override {
        Check(owner == object && item == 99u && result_item == 44u,
              "player item update arguments");
        calls.push_back("manage player");
    }
    void RemoveInventoryEntry(std::uint32_t collection,
                              std::uint32_t entry) override {
        Check(collection == 0x607cu && entry == selected_entry,
              "selected entry removal arguments");
        calls.push_back("remove selected");
        ++removed_selected_count;
    }
    void UpdateAndClampCounter(std::uint32_t inventory,
                               std::int32_t delta) override {
        Check(inventory == 0x6000u && delta == -80,
              "crafting charge arguments");
        calls.push_back("charge");
        ++fees;
    }
};

bool Before(const std::vector<std::string>& calls,
            const std::string& first, const std::string& second) {
    std::size_t first_index = calls.size();
    std::size_t second_index = calls.size();
    for (std::size_t i = 0; i < calls.size(); ++i) {
        if (calls[i] == first && first_index == calls.size())
            first_index = i;
        if (calls[i] == second && second_index == calls.size())
            second_index = i;
    }
    return first_index < second_index;
}

void CheckSelectedSuccess() {
    Runtime runtime;
    Check(pal4::CraftingApplication::Apply(
              runtime.object, 88u, 99u, runtime) == 0 &&
              runtime.removed_direct ==
                  (std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {10u, 2u}}) &&
              runtime.created_count == 1 &&
              runtime.removed_selected_count == 1 && runtime.fees == 1,
          "direct and selected ingredients produce item and charge once");
    Check(runtime.calls.at(0) == "music" &&
              runtime.calls.at(1) == "db" &&
              runtime.calls.at(2) == "recipe" &&
              Before(runtime.calls, "direct? 10", "direct? 20") &&
              Before(runtime.calls, "log 10", "remove 10") &&
              Before(runtime.calls, "read 8296", "create") &&
              Before(runtime.calls, "create", "manage player") &&
              Before(runtime.calls, "manage player", "remove selected") &&
              Before(runtime.calls, "remove selected", "charge"),
          "crafting transaction preserves IDA service order");

    Runtime no_player;
    no_player.player_condition = false;
    Check(pal4::CraftingApplication::Apply(
              no_player.object, 88u, 99u, no_player) == 0 &&
              no_player.removed_selected_count == 1 &&
              !Before(no_player.calls, "manage player", "remove selected"),
          "selected ingredient is removed when player condition is false");
}

void CheckUnresolvedRequirements() {
    Runtime no_selection;
    Check(pal4::CraftingApplication::Apply(
              no_selection.object, 88u, 0xffffffffu, no_selection) == -1 &&
              no_selection.removed_direct ==
                  (std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {10u, 2u}}) &&
              no_selection.created_count == 0 && no_selection.fees == 0,
          "remaining category item fails after direct ingredient removal");

    Runtime mismatch;
    mismatch.words[mismatch.selected_entry + 0x90u] = 21u;
    Check(pal4::CraftingApplication::Apply(
              mismatch.object, 88u, 99u, mismatch) == -1 &&
              mismatch.created_count == 0 &&
              mismatch.removed_selected_count == 0 && mismatch.fees == 0,
          "mismatched selection leaves requirement pending");

    Runtime missing;
    missing.selected_present = false;
    Check(pal4::CraftingApplication::Apply(
              missing.object, 88u, 99u, missing) == -1 &&
              missing.created_count == 0,
          "missing selected record does not satisfy category item");
}

void CheckUniqueSortedDirectItems() {
    Runtime runtime;
    runtime.Set(0, 30u, 4);
    runtime.Set(1, 10u, 2);
    runtime.Set(2, 30u, 9);
    runtime.Set(3, 40u, -1);
    runtime.direct_items = {10u, 30u};
    runtime.bytes[runtime.object] = 1u;
    Check(pal4::CraftingApplication::Apply(
              runtime.object, 88u, 0xffffffffu, runtime) == 0 &&
              runtime.removed_direct ==
                  (std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {10u, 2u}, {30u, 4u}}) &&
              runtime.created_count == 1 && runtime.fees == 0,
          "tree sorts unsigned keys and retains first duplicate quantity");
}

}  // namespace

int main() {
    try {
        CheckSelectedSuccess();
        CheckUnresolvedRequirements();
        CheckUniqueSortedDirectItems();
        std::cout << "crafting_application_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
