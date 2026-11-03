#include "pal4/native_player_slot_process_runtime.h"

#include "pal4/indexed_collection_reset.h"
#include "pal4/inventory_item_slots.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <span>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

std::uint32_t Address(const void* pointer) {
    return static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(pointer));
}

struct SkillCallbacks final : pal4::INativePlayerSkillCleanupCallbacks {
    std::uint32_t expected_script{};
    std::vector<std::uint32_t> resolved;
    std::vector<std::uint32_t> invoked;
    std::uint32_t ResolveSkillRecord(std::uint32_t skill) override {
        resolved.push_back(skill);
        return skill + 0x1000u;
    }
    std::uint32_t InvokeVirtual(
        std::uint32_t receiver, std::uint32_t slot,
        std::span<const std::uint32_t> arguments) override {
        Check(receiver == expected_script && slot == 304u &&
                  arguments.size() == 1u,
              "skill cleanup invokes the script's slot 304");
        invoked.push_back(arguments[0]);
        return 37u;
    }
};

}  // namespace

int main() {
    std::array<std::uint8_t, 0xb14u> slot{};
    const auto object = Address(slot.data());
    pal4::IndexedCollectionResetState collections;
    for (std::uint32_t i = 0u; i < 5u; ++i) {
        collections.values[i].begin = 0x1000u + i * 0x10u;
        collections.values[i].end = collections.values[i].begin + 8u;
        collections.key_values[i].begin = 0x2000u + i * 0x10u;
        collections.key_values[i].end =
            collections.key_values[i].begin + 8u;
    }
    collections.key_values[5].begin = 0x3000u;
    collections.key_values[5].end = 0x3010u;
    std::memcpy(slot.data() + 0x938u, &collections,
                sizeof(collections));
    pal4::InventoryItemSlotsState equipment;
    equipment.ids = {11u, 12u, 13u, 14u, 15u, 16u};
    std::memcpy(slot.data() + 0x920u, &equipment, sizeof(equipment));

    pal4::PlayerManagerSlotProcessRuntimeStub remaining;
    SkillCallbacks skills;
    pal4::NativePlayerSlotProcessRuntime runtime(remaining, skills);
    runtime.ClearCollectionFirstFive(object + 0x938u);
    std::memcpy(&collections, slot.data() + 0x938u,
                sizeof(collections));
    for (std::uint32_t i = 0u; i < 5u; ++i)
        Check(collections.values[i].end ==
                  collections.values[i].begin &&
                  collections.key_values[i].end ==
                      collections.key_values[i].begin,
              "first five native collection pairs are empty");
    Check(collections.key_values[5].end == 0x3010u,
          "sixth collection remains populated until fixed-array clear");
    runtime.ClearFixedSortedValues(object + 0x938u);
    std::memcpy(&collections, slot.data() + 0x938u,
                sizeof(collections));
    Check(collections.key_values[5].end == 0x3000u,
          "fixed sorted array clears the sixth slot");

    runtime.ResetInventorySlots(object + 0x920u);
    std::memcpy(&equipment, slot.data() + 0x920u,
                sizeof(equipment));
    Check(equipment.ids == std::array<std::uint32_t, 6u>{},
          "native inventory reset clears six equipment IDs");
    runtime.WriteDword(object + 0xa48u, 0x1234u);
    Check(runtime.ReadDword(object + 0xa48u) == 0x1234u,
          "native processor reads and writes the slot image");
    for (const auto member : {object + 0x658u, object + 0x7a8u}) {
        std::memset(reinterpret_cast<void*>(member), 0xff, 0x150u);
        runtime.InitializeMember(member);
        Check(runtime.ReadDword(member) == 7u &&
                  runtime.ReadDword(member + 0x90u) == 0u &&
                  runtime.ReadDword(member + 0xdcu) == 1u &&
                  runtime.ReadDword(member + 0xf4u) == 0x3f800000u &&
                  runtime.ReadDword(member + 0x148u) == 6u &&
                  runtime.ReadDword(member + 0x14cu) == 0xffffffffu,
              "native member reset applies race and game-data defaults");
    }
    const auto data = object + 0x7a8u;
    runtime.WriteDword(data + 4u, 80u);
    runtime.WriteDword(data + 12u, 50u);
    Check(runtime.GetElementByIndex(data, 0u) == 80u &&
              runtime.GetElementByIndex(data, 2u) == 50u &&
              runtime.GetElementByIndex(data, 8u) == 8u,
          "native property getter reads inline maxima");
    Check(runtime.SetProperty(data, 8u, 145u) == 100u &&
              runtime.ReadDword(data + 59u * 4u) == 100u &&
              runtime.SetProperty(data, 7u, 0xffffffffu) == 0u &&
              runtime.ReadDword(data + 58u * 4u) == 0u &&
              runtime.SetProperty(data, 9u, 95u) == 50u &&
              runtime.ReadDword(data + 60u * 4u) == 50u,
          "native property setter applies the original validation bounds");
    const auto script = object + 0x654u;
    skills.expected_script = script;
    std::array<std::uint32_t, 6u> timed{10u, 1u, 2u, 20u, 3u, 4u};
    std::array<std::uint32_t, 2u> untimed{30u, 5u};
    runtime.WriteDword(script + 680u, Address(timed.data()));
    runtime.WriteDword(script + 684u, Address(timed.data()) +
                                      sizeof(timed));
    runtime.WriteDword(script + 696u, Address(untimed.data()));
    runtime.WriteDword(script + 700u, Address(untimed.data()) +
                                      sizeof(untimed));
    runtime.CleanupScriptLists(script);
    Check(runtime.ReadDword(script + 684u) == Address(timed.data()) &&
              runtime.ReadDword(script + 700u) == Address(untimed.data()) &&
              skills.resolved == std::vector<std::uint32_t>{10u, 20u, 30u} &&
              skills.invoked ==
                  std::vector<std::uint32_t>{0x100au, 0x1014u, 0x101eu},
          "native script cleanup removes timed and untimed skills in order");
    std::cout << "PASS: native slot processor resets owned arrays\n";
}
