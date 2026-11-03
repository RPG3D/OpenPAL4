#include "pal4/script_inventory_query.h"

namespace pal4 {

bool ScriptInventoryQuery::HasAtLeast(
    std::int32_t item_id, std::int32_t required,
    IScriptInventoryQueryRuntime& runtime) {
    const auto element = runtime.FindElement(runtime.Inventory(), item_id, 5u);
    return element != 0u &&
           static_cast<std::int32_t>(runtime.ReadWord(element + 8u)) >= required;
}

std::int32_t ScriptInventoryQuery::Count(
    std::int32_t item_id, IScriptInventoryQueryRuntime& runtime) {
    const auto element = runtime.FindElement(runtime.Inventory(), item_id, 5u);
    return element == 0u ? 0 :
        static_cast<std::int32_t>(runtime.ReadWord(element + 8u));
}

std::int32_t ScriptInventoryQuery::Money(
    IScriptInventoryQueryRuntime& runtime) {
    return static_cast<std::int32_t>(
        runtime.ReadWord(runtime.Inventory() + 0x134u));
}

}  // namespace pal4
