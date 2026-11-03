#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace pal4 {

class IEquipmentItemPropertyRuntime {
public:
    virtual ~IEquipmentItemPropertyRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
};

class EquipmentItemPropertyRuntimeStub :
    public IEquipmentItemPropertyRuntime {
public:
    std::uint32_t ReadWord(std::uint32_t) override { return 0; }
};

class EquipmentItemPropertySort final {
public:
    [[nodiscard]] static bool Compare(
        std::uint32_t left, std::uint32_t right,
        std::uint32_t property_index,
        IEquipmentItemPropertyRuntime& runtime);
    [[nodiscard]] static std::size_t Partition(
        std::vector<std::uint32_t>& items,
        std::size_t begin, std::size_t end, std::uint32_t pivot_item,
        std::uint32_t property_index,
        IEquipmentItemPropertyRuntime& runtime);
    [[nodiscard]] static std::size_t QuickSort(
        std::vector<std::uint32_t>& items,
        std::size_t begin, std::size_t end,
        std::uint32_t property_index,
        IEquipmentItemPropertyRuntime& runtime);
    [[nodiscard]] static std::vector<std::uint32_t>::iterator Sort(
        std::vector<std::uint32_t>& items,
        std::uint32_t property_index,
        IEquipmentItemPropertyRuntime& runtime);
};

}  // namespace pal4
