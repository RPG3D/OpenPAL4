#include "pal4/combat_event_registry_initializer.h"

#include "pal4/combat_event_data_registry.h"

#include <array>
#include <bit>

namespace pal4 {
namespace {

struct Registration {
    std::string_view name;
    std::uint32_t object;
    std::uint32_t guard;
    std::uint8_t bit;
    std::uint8_t table_index;
    std::uint8_t kind;
};

constexpr std::array<Registration, 77> kRegistrations{{
    {"AI_TYPE", 0x8fa8bcu, 0x8fa8ccu, 0x1u, 57u, 1u},
    {"CA_MAX_HP", 0x8fa8acu, 0x8fa8ccu, 0x2u, 1u, 1u},
    {"CA_HP", 0x8fa89cu, 0x8fa8ccu, 0x4u, 2u, 1u},
    {"CA_RAGE", 0x8fa88cu, 0x8fa8ccu, 0x8u, 3u, 1u},
    {"CA_ADDITIONAL_RAGE", 0x8fa87cu, 0x8fa8ccu, 0x10u, 4u, 1u},
    {"CA_MAX_MP", 0x8fa86cu, 0x8fa8ccu, 0x20u, 5u, 1u},
    {"CA_MP", 0x8fa85cu, 0x8fa8ccu, 0x40u, 6u, 1u},
    {"CA_PHYSICAL", 0x8fa84cu, 0x8fa8ccu, 0x80u, 7u, 1u},
    {"CA_TOUGHNESS", 0x8fa838u, 0x8fa848u, 0x1u, 8u, 1u},
    {"CA_SPEED", 0x8fa828u, 0x8fa848u, 0x2u, 9u, 1u},
    {"CA_LUCKY", 0x8fa818u, 0x8fa848u, 0x4u, 10u, 1u},
    {"CA_WILL", 0x8fa808u, 0x8fa848u, 0x8u, 11u, 1u},
    {"EFE_WATER", 0x8fa7f8u, 0x8fa848u, 0x10u, 12u, 1u},
    {"EFE_FIRE", 0x8fa7e8u, 0x8fa848u, 0x20u, 13u, 1u},
    {"EFE_THUNDER", 0x8fa7d8u, 0x8fa848u, 0x40u, 14u, 1u},
    {"EFE_AIR", 0x8fa7c8u, 0x8fa848u, 0x80u, 15u, 1u},
    {"EFE_EARTH", 0x8fa7b4u, 0x8fa7c4u, 0x1u, 16u, 1u},
    {"CA_ADDITIONAL_CRITICAL", 0x8fa7a4u, 0x8fa7c4u, 0x2u, 17u, 1u},
    {"CA_FEND_OFF", 0x8fa794u, 0x8fa7c4u, 0x4u, 18u, 1u},
    {"CA_ADDITIONAL_HITTING", 0x8fa784u, 0x8fa7c4u, 0x8u, 19u, 1u},
    {"CA_ATTACK_PHYSICAL_ADDITIONAL", 0x8fa774u, 0x8fa7c4u, 0x10u, 21u, 1u},
    {"CA_ATTACK_WATER_ADDITIONAL", 0x8fa764u, 0x8fa7c4u, 0x20u, 23u, 1u},
    {"CA_ATTACK_FIRE_ADDITIONAL", 0x8fa754u, 0x8fa7c4u, 0x40u, 24u, 1u},
    {"CA_ATTACK_THUNDER_ADDITIONAL", 0x8fa744u, 0x8fa7c4u, 0x80u, 25u, 1u},
    {"CA_ATTACK_AIR_ADDITIONAL", 0x8fa730u, 0x8fa740u, 0x1u, 26u, 1u},
    {"CA_ATTACK_EARTH_ADDITIONAL", 0x8fa720u, 0x8fa740u, 0x2u, 27u, 1u},
    {"CA_ATTACK_PHYSICAL_REACT", 0x8fa710u, 0x8fa740u, 0x4u, 22u, 1u},
    {"CA_ATTACK_WATER_REACT", 0x8fa700u, 0x8fa740u, 0x8u, 33u, 1u},
    {"CA_ATTACK_FIRE_REACT", 0x8fa6f0u, 0x8fa740u, 0x10u, 34u, 1u},
    {"CA_ATTACK_THUNDER_REACT", 0x8fa6e0u, 0x8fa740u, 0x20u, 35u, 1u},
    {"CA_ATTACK_AIR_REACT", 0x8fa6d0u, 0x8fa740u, 0x40u, 36u, 1u},
    {"CA_ATTACK_EARTH_REACT", 0x8fa6c0u, 0x8fa740u, 0x80u, 37u, 1u},
    {"CA_ATTACK_PHYSICAL_EXTRACT", 0x8fa6acu, 0x8fa6bcu, 0x1u, 20u, 1u},
    {"CA_ATTACK_WATER_EXTRACT", 0x8fa69cu, 0x8fa6bcu, 0x2u, 28u, 1u},
    {"CA_ATTACK_FIRE_EXTRACT", 0x8fa68cu, 0x8fa6bcu, 0x4u, 29u, 1u},
    {"CA_ATTACK_THUNDER_EXTRACT", 0x8fa67cu, 0x8fa6bcu, 0x8u, 30u, 1u},
    {"CA_ATTACK_AIR_EXTRACT", 0x8fa66cu, 0x8fa6bcu, 0x10u, 31u, 1u},
    {"CA_ATTACK_EARTH_EXTRACT", 0x8fa65cu, 0x8fa6bcu, 0x20u, 32u, 1u},
    {"EAST_A", 0x8fa64cu, 0x8fa6bcu, 0x40u, 39u, 0u},
    {"EAST_B", 0x8fa63cu, 0x8fa6bcu, 0x80u, 40u, 0u},
    {"EAST_C", 0x8fa628u, 0x8fa638u, 0x1u, 41u, 0u},
    {"EAST_D", 0x8fa618u, 0x8fa638u, 0x2u, 42u, 0u},
    {"EAST_E", 0x8fa608u, 0x8fa638u, 0x4u, 43u, 0u},
    {"EAST_F", 0x8fa5f8u, 0x8fa638u, 0x8u, 44u, 0u},
    {"EAST_G", 0x8fa5e8u, 0x8fa638u, 0x10u, 45u, 0u},
    {"EAST_H", 0x8fa5d8u, 0x8fa638u, 0x20u, 46u, 0u},
    {"EAST_I", 0x8fa5c8u, 0x8fa638u, 0x40u, 47u, 0u},
    {"CA_PAY_FOR_SPARE", 0x8fa5b8u, 0x8fa638u, 0x80u, 38u, 1u},
    {"CA_PHYSICAL_ATTACK_PARAM", 0x8fa5a4u, 0x8fa5b4u, 0x1u, 48u, 1u},
    {"CA_MAGIC_ATTACK_PARAM", 0x8fa594u, 0x8fa5b4u, 0x2u, 49u, 1u},
    {"ERR_HUMAN_RACE_ATK", 0x8fa584u, 0x8fa5b4u, 0x4u, 50u, 1u},
    {"ERR_DEMON_RACE_ATK", 0x8fa574u, 0x8fa5b4u, 0x8u, 51u, 1u},
    {"ERR_FAIRY_RACE_ATK", 0x8fa564u, 0x8fa5b4u, 0x10u, 52u, 1u},
    {"ERR_GHOST_RACE_ATK", 0x8fa554u, 0x8fa5b4u, 0x20u, 53u, 1u},
    {"ERR_SPIRIT_RACE_ATK", 0x8fa544u, 0x8fa5b4u, 0x40u, 55u, 1u},
    {"ERR_BEAST_RACE_ATK", 0x8fa534u, 0x8fa5b4u, 0x80u, 56u, 1u},
    {"ERR_IMMORTAL_RACE_ATK", 0x8fa520u, 0x8fa530u, 0x1u, 54u, 1u},
    {"EASA_WATER_IMMUNITY", 0x8fa510u, 0x8fa530u, 0x2u, 58u, 1u},
    {"EASA_FIRE_IMMUNITY", 0x8fa500u, 0x8fa530u, 0x4u, 59u, 1u},
    {"EASA_THUNDER_IMMUNITY", 0x8fa4f0u, 0x8fa530u, 0x8u, 60u, 1u},
    {"EASA_AIR_IMMUNITY", 0x8fa4e0u, 0x8fa530u, 0x10u, 61u, 1u},
    {"EASA_EARTH_IMMUNITY", 0x8fa4d0u, 0x8fa530u, 0x20u, 62u, 1u},
    {"EASB_BONDAGE_IMMUNITY", 0x8fa4c0u, 0x8fa530u, 0x40u, 63u, 1u},
    {"EASB_SEAL_IMMUNITY", 0x8fa4b0u, 0x8fa530u, 0x80u, 64u, 1u},
    {"EASB_FORBIDEN_IMMUNITY", 0x8fa49cu, 0x8fa4acu, 0x1u, 65u, 1u},
    {"EASB_SLEEP_IMMUNITY", 0x8fa48cu, 0x8fa4acu, 0x2u, 66u, 1u},
    {"EASB_MAD_IMMUNITY", 0x8fa47cu, 0x8fa4acu, 0x4u, 67u, 1u},
    {"EASB_DISORDER_IMMUNITY", 0x8fa46cu, 0x8fa4acu, 0x8u, 68u, 1u},
    {"ESI_SIPHON", 0x8fa45cu, 0x8fa4acu, 0x10u, 69u, 1u},
    {"ESI_SIPHON", 0x8fa44cu, 0x8fa4acu, 0x20u, 70u, 1u},
    {"ESI_SIPHON", 0x8fa43cu, 0x8fa4acu, 0x40u, 71u, 1u},
    {"ESI_SIPHON", 0x8fa42cu, 0x8fa4acu, 0x80u, 72u, 1u},
    {"ESI_SIPHON", 0x8fa418u, 0x8fa428u, 0x1u, 73u, 1u},
    {"ESI_SIPHON", 0x8fa408u, 0x8fa428u, 0x2u, 74u, 1u},
    {"ESI_SIPHON", 0x8fa3f8u, 0x8fa428u, 0x4u, 75u, 1u},
    {"ESI_SIPHON", 0x8fa3e8u, 0x8fa428u, 0x8u, 76u, 1u},
    {"ESI_MORTAL", 0x8fa3d8u, 0x8fa428u, 0x10u, 77u, 1u},
}};

}  // namespace

std::int32_t CombatEventRegistryInitializer::Initialize(
    ICombatEventRegistryInitRuntime& runtime) {
    for (std::uint32_t index = 0u; index < 78u; ++index)
        runtime.WriteWord(CombatEventDataRegistry::kBase + index * 4u, 0u);

    const auto game = runtime.GameCollection();
    const auto data = runtime.DataCollection();
    std::int32_t result = 0;
    for (std::uint32_t ordinal = 0u; ordinal < kRegistrations.size();
         ++ordinal) {
        const auto& entry = kRegistrations[ordinal];
        const auto game_element = runtime.FindElementByName(game, entry.name);
        const auto data_element = runtime.FindElementByName(data, entry.name);
        result = std::bit_cast<std::int32_t>(data_element);
        const auto guard = runtime.ReadByte(entry.guard);
        if ((guard & entry.bit) == 0u) {
            runtime.WriteByte(entry.guard,
                              static_cast<std::uint8_t>(guard | entry.bit));
            runtime.InitializeObject(entry.object, game_element,
                                     data_element, entry.kind);
            result = runtime.RegisterAtExit(0x583540u - ordinal * 16u);
        }
        runtime.WriteWord(CombatEventDataRegistry::kBase +
                              static_cast<std::uint32_t>(entry.table_index) * 4u,
                          entry.object);
    }
    return result;
}

}  // namespace pal4
