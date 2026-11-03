#pragma once

#include <cstdint>

namespace pal4 {

struct PropertyCollectionState {
    std::uint32_t vtable{};
    std::uint32_t entries{};
    std::int32_t count{};
};
static_assert(sizeof(PropertyCollectionState) == 12u);

struct PropertyScalarState {
    std::uint32_t vtable{};
    std::uint32_t value_offset{};
    std::int32_t index_count{};
    std::uint32_t index_offset{};
};
static_assert(sizeof(PropertyScalarState) == 16u);

class IPropertyAccessRuntime {
public:
    virtual ~IPropertyAccessRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    virtual void WriteFloat(std::uint32_t address, float value) = 0;
    [[nodiscard]] virtual std::int32_t DispatchByIndex(
        const PropertyCollectionState& collection, std::uint32_t target,
        std::int32_t index, std::uint32_t vtable_offset) = 0;
};

class PropertyAccess final {
public:
    // IDA 0x5ed7c0.
    [[nodiscard]] static std::int32_t ProcessByName(
        const PropertyCollectionState& collection, std::uint32_t target,
        const char* name, IPropertyAccessRuntime& runtime);
    // IDA 0x5ed860..0x5ed8e0.
    [[nodiscard]] static std::uint32_t SetFloat(
        const PropertyScalarState& property, std::uint32_t target, float value,
        IPropertyAccessRuntime& runtime);
    [[nodiscard]] static double GetFloat(
        const PropertyScalarState& property, std::uint32_t target,
        IPropertyAccessRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetInt(
        const PropertyScalarState& property, std::uint32_t target,
        std::uint32_t value, IPropertyAccessRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetInt(
        const PropertyScalarState& property, std::uint32_t target,
        IPropertyAccessRuntime& runtime);
    [[nodiscard]] static std::int32_t SetIndex(
        const PropertyScalarState& property, std::uint32_t target,
        std::int32_t index, IPropertyAccessRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetIndex(
        const PropertyScalarState& property, std::uint32_t target,
        IPropertyAccessRuntime& runtime);
};

}  // namespace pal4
