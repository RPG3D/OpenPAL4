#include "pal4/crafting_extended_application.h"

#include <map>
#include <utility>

namespace pal4 {
namespace {

class TemporaryText final {
public:
    TemporaryText(ICraftingExtendedRuntime& runtime, std::uint8_t mode)
        : runtime_(runtime), address_(runtime.CreateTemporaryText(mode)) {}
    ~TemporaryText() { runtime_.ReleaseTemporaryText(address_); }

    TemporaryText(const TemporaryText&) = delete;
    TemporaryText& operator=(const TemporaryText&) = delete;

    [[nodiscard]] std::uint32_t Address() const { return address_; }

private:
    ICraftingExtendedRuntime& runtime_;
    std::uint32_t address_;
};

}  // namespace

std::int32_t CraftingExtendedApplication::Apply(
    const std::uint32_t object, const std::uint32_t recipe,
    const std::uint32_t target_item, ICraftingExtendedRuntime& runtime) {
    runtime.PlayMusic("wa09.wav", 1u);
    const auto recipe_data = runtime.FindRecipe(runtime.GameDb() + 0x80u,
                                                recipe);

    std::map<std::uint32_t, std::uint32_t> remaining;
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        const auto item = runtime.ReadWord(recipe_data + 0x98u + index * 4u);
        const auto quantity = runtime.ReadWord(
            recipe_data + 0xa8u + index * 4u);
        if (static_cast<std::int32_t>(quantity) > 0)
            remaining.emplace(item, quantity);
    }

    for (auto current = remaining.begin(); current != remaining.end();) {
        const auto item = current->first;
        if (runtime.IsDirectItem(runtime.GameDb(), item)) {
            runtime.FindOrLogDirectItem(runtime.GameDb(), item);
            runtime.RemoveDirectItem(runtime.Inventory(), item,
                                     current->second);
            current = remaining.erase(current);
        } else {
            ++current;
        }
    }
    if (!remaining.empty()) return -1;

    const auto entry = runtime.FindInventoryEntry(
        runtime.Inventory() + 0x7cu, target_item, 7u);
    runtime.WriteWord(entry + 0x180u,
                      runtime.ReadWord(recipe_data + 0x74u));

    for (const auto [recipe_offset, entry_offset] : {
             std::pair{0x28u, 0xb8u}, std::pair{0x08u, 0x108u}}) {
        auto source = runtime.ReadWord(recipe_data + recipe_offset);
        if (source == 0u) source = runtime.EmptyCString();
        runtime.AssignCString(entry + entry_offset, source,
                              runtime.CStringLength(source));
    }

    const auto state = runtime.ReadWord(entry + 0x94u);
    if (state < 3u) {
        const auto source = recipe_data + 0x34u + state * 0x10u;
        TemporaryText temporary(runtime, runtime.ReadByte(source));
        runtime.AssignText(temporary.Address(), source, 0u, 0xffffffffu);
        runtime.AssignText(entry + 0xf8u, temporary.Address(), 0u,
                           0xffffffffu);
    }

    if (runtime.ReadByte(object) == 0u)
        runtime.UpdateAndClampCounter(runtime.Inventory(), -80);
    return 0;
}

}  // namespace pal4
