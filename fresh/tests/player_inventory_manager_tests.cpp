#include "pal4/player_inventory_manager.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IPlayerInventoryRuntime {
    std::array<std::uint32_t, 4> players{0u, 0x3000u, 0x4000u, 0u};
    std::array<bool, 4> contains{false, false, true, false};
    std::vector<std::string> calls;
    std::int32_t slot = 2;
    std::uint32_t refresh_result = 0x1234u;
    std::uint32_t written_address = 0u;
    std::uint32_t written_value = 0u;

    std::uint32_t InventorySubsystem() override {
        calls.push_back("inventory");
        return 0x1000u;
    }
    std::uint32_t FindInventoryEntry(std::uint32_t collection,
                                     std::uint32_t key,
                                     std::uint32_t category) override {
        Check(collection == 0x107cu && key == 88u && category == 7u,
              "inventory lookup arguments");
        calls.push_back("entry");
        return 0x2000u;
    }
    std::uint32_t PlayerManager() override {
        calls.push_back("manager");
        return 0x2500u;
    }
    std::uint32_t GetPlayer(std::uint32_t manager,
                            std::uint32_t index) override {
        Check(manager == 0x2500u && index < 4u, "player lookup arguments");
        calls.push_back("player " + std::to_string(index));
        return players[index];
    }
    bool Contains(std::uint32_t slots, std::uint32_t item) override {
        Check(item == 77u, "item membership argument");
        calls.push_back("contains");
        if (slots == 0x3920u) return contains[1];
        if (slots == 0x4920u) return contains[2];
        throw std::runtime_error("unexpected slots");
    }
    std::int32_t FindSlot(std::uint32_t slots,
                          std::uint32_t entry) override {
        Check(slots == 0x4920u && entry == 0x2000u,
              "slot classification arguments");
        calls.push_back("find slot");
        return slot;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x2190u, "item value address");
        calls.push_back("read item");
        return 0xabcdefu;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("write slot");
        written_address = address;
        written_value = value;
    }
    std::uint32_t Refresh(std::uint32_t component,
                          std::uint32_t vtable_slot) override {
        Check(component == 0x4654u && vtable_slot == 0x114u,
              "player component virtual call");
        calls.push_back("refresh");
        return refresh_result;
    }
};

void CheckFoundPlayer() {
    Runtime runtime;
    Check(pal4::PlayerInventoryManager::Manage(77u, 88u, runtime) == 0x34u &&
              runtime.written_address == 0x4928u &&
              runtime.written_value == 0xabcdefu &&
              runtime.calls == (std::vector<std::string>{
                  "inventory", "entry", "manager", "player 0",
                  "manager", "player 1", "contains", "manager",
                  "player 2", "contains", "find slot", "read item",
                  "write slot", "refresh"}),
          "matching player updates selected slot then returns refresh byte");

    for (const auto out_of_range : {-1, 6, 99}) {
        Runtime invalid;
        invalid.slot = out_of_range;
        Check(pal4::PlayerInventoryManager::Manage(77u, 88u, invalid) ==
                  0x34u &&
                  invalid.written_address == 0u &&
                  invalid.calls.back() == "refresh",
              "invalid classification still refreshes without writing");
    }
}

void CheckNoMatchingPlayer() {
    Runtime runtime;
    runtime.contains.fill(false);
    Check(pal4::PlayerInventoryManager::Manage(77u, 88u, runtime) == 0u &&
              runtime.written_address == 0u &&
              runtime.calls.back() == "player 3" &&
              runtime.calls.size() == 12u,
          "four-player search returns zero after final miss");
}

}  // namespace

int main() {
    try {
        CheckFoundPlayer();
        CheckNoMatchingPlayer();
        std::cout << "player_inventory_manager_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
