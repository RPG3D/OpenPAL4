#include "pal4/item_use_effects.h"

#include <bit>
#include <cstdint>
#include <map>

namespace pal4 {
namespace {

float ReadFloat(const std::uint32_t address,
                IItemUseEffectsRuntime& runtime) {
    return std::bit_cast<float>(runtime.ReadWord(address));
}

void WriteFloat(const std::uint32_t address, const float value,
                IItemUseEffectsRuntime& runtime) {
    runtime.WriteWord(address, std::bit_cast<std::uint32_t>(value));
}

std::int32_t ScaleInteger(const std::uint32_t base,
                          const std::uint32_t factor, const double scale,
                          IItemUseEffectsRuntime& runtime) {
    const auto product = std::bit_cast<std::int32_t>(base * factor);
    return runtime.FloatToInt(static_cast<double>(product) * scale);
}

void ApplyIntegerEffects(const std::uint32_t recipe,
                         const std::uint32_t player,
                         const bool percentage,
                         IItemUseEffectsRuntime& runtime) {
    for (std::uint32_t i = 0u; i < 8u; ++i) {
        const auto modifier = runtime.ReadWord(recipe + 0xb8u + i * 4u);
        auto amount = modifier;
        if (percentage) {
            const auto base = runtime.ReadWord(player + 4u + i * 4u);
            amount = static_cast<std::uint32_t>(
                ScaleInteger(base, modifier, 0.01, runtime));
        }
        const auto address = player + 4u + i * 4u;
        runtime.WriteWord(address, runtime.ReadWord(address) + amount);
    }

    for (std::uint32_t i = 0u; i < 5u; ++i) {
        const auto modifier = runtime.ReadWord(recipe + 0xd8u + i * 4u);
        auto amount = modifier;
        if (percentage) {
            const auto base = runtime.ReadWord(player + 0x7cu + i * 4u);
            amount = static_cast<std::uint32_t>(ScaleInteger(
                base, modifier, i < 4u ? -0.01 : 0.01, runtime));
        }
        const auto address = player + 0x7cu + i * 4u;
        const auto current = runtime.ReadWord(address);
        runtime.WriteWord(address,
                          percentage && i < 4u ? current - amount
                                                 : current + amount);
    }

    for (std::uint32_t i = 0u; i < 6u; ++i) {
        const auto modifier = runtime.ReadWord(recipe + 0xecu + i * 4u);
        auto amount = modifier;
        if (percentage) {
            const auto base = runtime.ReadWord(player + 0x34u + i * 4u);
            amount = static_cast<std::uint32_t>(ScaleInteger(
                base, modifier, i < 5u ? -0.01 : 0.01, runtime));
        }
        const auto address = player + 0x34u + i * 4u;
        const auto current = runtime.ReadWord(address);
        runtime.WriteWord(address,
                          percentage && i < 5u ? current - amount
                                                 : current + amount);
    }
}

void ApplyFloatEffects(const std::uint32_t recipe,
                       const std::uint32_t player,
                       const bool percentage,
                       IItemUseEffectsRuntime& runtime) {
    for (std::uint32_t i = 0u; i < 6u; ++i) {
        const auto modifier = ReadFloat(recipe + 0x104u + i * 4u, runtime);
        const auto address = player + 0x64u + i * 4u;
        const auto base = ReadFloat(address, runtime);
        auto delta = percentage
                               ? static_cast<double>(base) * modifier * 0.01
                               : static_cast<double>(modifier);
        // IDA5912A0: the last skill spills its increment with FSTP m32
        // before reloading it for the addition; other skills retain ST(0).
        if (percentage && i == 5u) delta = static_cast<float>(delta);
        WriteFloat(address, static_cast<float>(base + delta), runtime);
    }

    for (std::uint32_t i = 0u; i < 6u; ++i) {
        const auto modifier = ReadFloat(recipe + 0x11cu + i * 4u, runtime);
        const auto address = player + 0x4cu + i * 4u;
        const auto base = ReadFloat(address, runtime);
        const auto delta = percentage
                               ? static_cast<double>(base) * modifier * 0.01
                               : static_cast<double>(modifier);
        WriteFloat(address, static_cast<float>(base + delta), runtime);
    }

    for (std::uint32_t i = 0u; i < 4u; ++i) {
        const auto modifier = ReadFloat(recipe + 0x134u + i * 4u, runtime);
        const auto address = player + 0x24u + i * 4u;
        const auto base = ReadFloat(address, runtime);
        auto delta = percentage
                               ? static_cast<double>(base) * modifier * 0.01
                               : static_cast<double>(modifier);
        // The final attribute has the same explicit FSTP m32 boundary.
        if (percentage && i == 3u) delta = static_cast<float>(delta);
        WriteFloat(address, static_cast<float>(base + delta), runtime);
    }
}

}  // namespace

std::int32_t ItemUseEffects::Apply(
    const std::uint32_t object, const std::uint32_t recipe,
    const std::uint32_t item, IItemUseEffectsRuntime& runtime) {
    const auto recipe_data = runtime.FindRecipe(runtime.GameDb() + 0x80u,
                                                recipe);
    std::map<std::uint32_t, std::uint32_t> remaining;
    for (std::uint32_t i = 0u; i < 4u; ++i) {
        const auto material = runtime.ReadWord(recipe_data + 0x98u + i * 4u);
        const auto quantity = runtime.ReadWord(recipe_data + 0xa8u + i * 4u);
        if (static_cast<std::int32_t>(quantity) > 0)
            remaining.emplace(material, quantity);
    }

    for (auto current = remaining.begin(); current != remaining.end();) {
        const auto material = current->first;
        if (runtime.IsDirectItem(runtime.GameDb(), material)) {
            runtime.FindOrLogDirectItem(runtime.GameDb(), material);
            runtime.RemoveDirectItem(runtime.Inventory(), material,
                                     current->second);
            current = remaining.erase(current);
        } else {
            ++current;
        }
    }
    if (!remaining.empty()) return -2;

    const auto inventory = runtime.Inventory();
    const auto player = runtime.FindInventoryEntry(inventory + 0x7cu,
                                                   item, 7u);
    const auto difference = runtime.ReadWord(player + 0x184u) -
                            runtime.ReadWord(recipe_data + 0x6cu);
    std::int32_t chance = 100;
    if ((difference & 0x80000000u) != 0u) {
        switch (difference) {
        case 0xffffffffu: chance = 10; break;
        case 0xfffffffeu: chance = 9; break;
        case 0xfffffffdu: chance = 8; break;
        case 0xfffffffcu: chance = 7; break;
        default: chance = 6; break;
        }
    }
    runtime.SetRandomSeed(runtime.CurrentTime());
    const auto roll = runtime.Random() % 100;
    runtime.WriteWord(player + 0x184u, difference);

    if (roll >= chance) {
        runtime.PlayMusic("wa09a.wav", 1u);
        if (runtime.ReadByte(object) == 0u)
            runtime.UpdateAndClampCounter(runtime.Inventory(), -80);
        return -1;
    }

    const auto percentage = runtime.ReadWord(recipe_data + 0x144u) != 0u;
    ApplyIntegerEffects(recipe_data, player, percentage, runtime);
    ApplyFloatEffects(recipe_data, player, percentage, runtime);
    runtime.PlayMusic("wa09.wav", 1u);
    if (runtime.ReadByte(object) == 0u)
        runtime.UpdateAndClampCounter(runtime.Inventory(), -80);
    return 0;
}

}  // namespace pal4
