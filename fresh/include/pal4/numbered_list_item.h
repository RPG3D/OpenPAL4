#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

class INumberedListRenderRuntime;

class INumberedListItemConstructionRuntime {
public:
    virtual ~INumberedListItemConstructionRuntime() = default;
    virtual void ConstructBase(std::uint32_t item,
                               std::string_view text,
                               std::uint32_t id,
                               std::uint32_t data,
                               bool disabled,
                               bool auto_delete) = 0;
    [[nodiscard]] virtual std::uint8_t InitialNameByte() = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void SetErrorFlag(std::uint32_t address,
                              std::uint8_t value) = 0;
    virtual void AssignName(std::uint32_t destination,
                            std::uint32_t source,
                            std::uint32_t offset,
                            std::uint32_t length) = 0;
    virtual void CopyBytes(std::uint32_t destination,
                           std::uint32_t source,
                           std::uint32_t length) = 0;
    [[nodiscard]] virtual std::uint32_t FindImage(
        std::string_view imageset, std::string_view image) = 0;
};

class INumberedListItemRuntime : public INumberedListItemConstructionRuntime {
public:
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t BaseItemVtable() = 0;
    virtual void DestroyString(std::uint32_t address) = 0;
};

class NumberedListItem final {
public:
    static constexpr std::uint32_t kVtable = 0x844190u;

    // IDA 0x4b2020.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t item, std::int32_t index,
        std::uint32_t record, std::uint32_t secondary,
        bool alternate_image, std::uint32_t name,
        INumberedListItemConstructionRuntime& runtime);
    // IDA 0x4b23c0.
    [[nodiscard]] static std::uint32_t ConditionalDestruct(
        std::uint32_t item, std::uint8_t flags,
        INumberedListItemRuntime& runtime);
    // IDA 0x4b23e0.
    static void Destruct(std::uint32_t item,
                         INumberedListItemRuntime& runtime);
    // IDA 0x4b2480.
    static void Render(std::uint32_t item, std::uint32_t cache,
                       const std::array<float, 4>& rect,
                       float alpha, float modulation,
                       const std::array<float, 4>& clip,
                       INumberedListRenderRuntime& runtime);
    // IDA 0x4b3180/0x4b31a0.
    [[nodiscard]] static std::uint32_t GetSize(
        std::uint32_t item, std::uint32_t output,
        INumberedListRenderRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetSize(
        std::uint32_t item, std::uint32_t second,
        INumberedListRenderRuntime& runtime);
};

}  // namespace pal4
