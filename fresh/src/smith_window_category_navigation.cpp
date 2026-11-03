#include "pal4/smith_window.h"

#include <bit>
#include <cstdint>

namespace pal4 {
namespace {

std::int32_t CategoryCount(const std::uint32_t object,
                           const std::uint32_t begin,
                           ISmithWindowCategoryNavigationRuntime& runtime) {
    const auto end = runtime.ReadWord(object + 0xd0u);
    return std::bit_cast<std::int32_t>(end - begin) >> 2;
}

bool CanNavigate(const std::uint32_t object,
                 ISmithWindowCategoryNavigationRuntime& runtime) {
    if (runtime.ReadByte(object + 0xacu) == 0u) return false;
    if (runtime.ReadWord(object + 0xb4u) == 0u) {
        runtime.RefreshAfterInventoryChange(object);
        return false;
    }
    return true;
}

}  // namespace

std::uint8_t SmithWindow::OnCategoryPreviousClick(
    const std::uint32_t object, const std::uint32_t,
    ISmithWindowCategoryNavigationRuntime& runtime) {
    if (!CanNavigate(object, runtime)) return 1u;
    const auto begin = runtime.ReadWord(object + 0xccu);
    if (begin == 0u ||
        static_cast<std::uint32_t>(CategoryCount(object, begin, runtime)) <= 1u) {
        return 1u;
    }
    const auto index_address = object + 0xb0u;
    const auto index = runtime.ReadWord(index_address) - 1u;
    runtime.WriteWord(index_address, index);
    if (std::bit_cast<std::int32_t>(index) < 0) {
        const auto current_begin = runtime.ReadWord(object + 0xccu);
        runtime.WriteWord(index_address,
                          current_begin == 0u
                              ? 0xffffffffu
                              : static_cast<std::uint32_t>(
                                    CategoryCount(object, current_begin, runtime) - 1));
    }
    runtime.UpdateCategoryPageButtons(object);
    return 1u;
}

std::uint8_t SmithWindow::OnCategoryNextClick(
    const std::uint32_t object, const std::uint32_t,
    ISmithWindowCategoryNavigationRuntime& runtime) {
    if (!CanNavigate(object, runtime)) return 1u;
    const auto begin = runtime.ReadWord(object + 0xccu);
    if (begin == 0u ||
        static_cast<std::uint32_t>(CategoryCount(object, begin, runtime)) <= 1u) {
        return 1u;
    }
    const auto index_address = object + 0xb0u;
    const auto index = runtime.ReadWord(index_address) + 1u;
    runtime.WriteWord(index_address, index);
    const auto current_begin = runtime.ReadWord(object + 0xccu);
    if (current_begin == 0u ||
        index >= static_cast<std::uint32_t>(
                     CategoryCount(object, current_begin, runtime))) {
        runtime.WriteWord(index_address, 0u);
    }
    runtime.UpdateCategoryPageButtons(object);
    return 1u;
}

}  // namespace pal4
