#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

struct MultiListColour {
    float red;
    float green;
    float blue;
    float alpha;
};

class IMultiListTextItemRuntime {
public:
    virtual ~IMultiListTextItemRuntime() = default;
    virtual void ConstructBase(std::uint32_t item, std::uint32_t text,
                               std::uint32_t id, std::uint32_t data,
                               std::uint32_t disabled,
                               std::uint32_t auto_delete) = 0;
    virtual void InitColourRect(std::uint32_t address,
                                MultiListColour colour) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void SetTextColours(std::uint32_t item,
                                MultiListColour colour) = 0;
    virtual void SetSelectionBrush(std::uint32_t item,
                                   std::string_view imageset,
                                   std::string_view image) = 0;
    [[nodiscard]] virtual std::uint32_t BaseTextVtable() = 0;
    [[nodiscard]] virtual std::uint32_t BaseItemVtable() = 0;
    virtual void DestroyString(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ModulateColours(
        std::uint32_t item, std::uint32_t colour_rect,
        std::uint32_t cache, std::uint32_t mode) = 0;
    virtual void CacheImage(std::uint32_t cache, std::uint32_t image,
                            std::uint32_t area, float horizontal_inset,
                            float alpha, std::uint32_t colours) = 0;
    [[nodiscard]] virtual std::uint32_t GetFont(
        std::uint32_t item) = 0;
    [[nodiscard]] virtual std::uint32_t CacheText(
        std::uint32_t target, std::uint32_t text,
        std::uint32_t font, float x, float y,
        std::uint32_t mode, std::uint32_t colours) = 0;
};

class MultiListTextItem final {
public:
    static constexpr std::uint32_t kVtable = 0x844108u;

    // IDA 0x4af780.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t item, std::uint32_t text,
        IMultiListTextItemRuntime& runtime);
    // IDA 0x4afa00.
    static void Destruct(std::uint32_t item,
                         IMultiListTextItemRuntime& runtime);
    // IDA 0x4afa70.
    [[nodiscard]] static std::uint32_t Render(
        std::uint32_t item, std::uint32_t area,
        std::uint32_t text_target, std::uint32_t cache,
        std::uint32_t mode, float alpha,
        IMultiListTextItemRuntime& runtime);
};

}  // namespace pal4
