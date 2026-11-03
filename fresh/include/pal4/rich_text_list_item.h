#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

class IRichTextListItemRuntime {
public:
    virtual ~IRichTextListItemRuntime() = default;
    virtual void ConstructBase(std::uint32_t item,
                               std::uint32_t text,
                               std::uint32_t id,
                               std::uint32_t data,
                               bool disabled,
                               bool auto_delete) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteFloat(std::uint32_t address, float value) = 0;
    [[nodiscard]] virtual float ReadFloat(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CreateFrame() = 0;
    virtual void DestroyFrame(std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t FindFont(
        std::string_view name) = 0;
    virtual void SetFormatForSC(std::uint32_t frame,
                                bool enabled) = 0;
    virtual void AssignFrameText(std::uint32_t frame,
                                 std::uint32_t text,
                                 std::uint32_t font,
                                 const std::array<float, 4>& colour) = 0;
    virtual void SetFrameRect(std::uint32_t frame,
                              const std::array<float, 4>& rect,
                              std::uint32_t font) = 0;
    [[nodiscard]] virtual std::uint32_t FindImage(
        std::string_view imageset, std::string_view image) = 0;
    [[nodiscard]] virtual std::uint32_t BaseItemVtable() = 0;
    virtual void DestroyString(std::uint32_t address) = 0;
    virtual void RenderFrame(std::uint32_t frame,
                             std::uint32_t context,
                             const std::array<float, 4>& rect,
                             float alpha, float inset,
                             std::uint32_t clip) = 0;
    virtual void CacheImage(std::uint32_t cache,
                            std::uint32_t image,
                            const std::array<float, 4>& rect,
                            float alpha, std::uint32_t clip,
                            const std::array<float, 4>& colour,
                            bool first_flag, bool second_flag) = 0;
    [[nodiscard]] virtual float GetTextHeight(std::uint32_t frame,
                                               float inset) = 0;
    [[nodiscard]] virtual std::array<float, 4> GetParentRect(
        std::uint32_t parent, std::uint32_t virtual_slot) = 0;
    [[nodiscard]] virtual std::int32_t FloatToLong(float value) = 0;
};

class RichTextListItem final {
public:
    static constexpr std::uint32_t kVtable = 0x844170u;

    // IDA 0x4b1ba0.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t item, std::uint32_t text,
        float available_width, IRichTextListItemRuntime& runtime);
    // IDA 0x4b1e20.
    static void Destruct(std::uint32_t item,
                         IRichTextListItemRuntime& runtime);
    // IDA 0x4b1ea0.
    static void Render(std::uint32_t item, std::uint32_t context,
                       const std::array<float, 4>& rect,
                       float alpha, float modulation,
                       std::uint32_t clip,
                       IRichTextListItemRuntime& runtime);
    // IDA 0x4b1f90.
    [[nodiscard]] static std::uint32_t GetSize(
        std::uint32_t item, std::uint32_t output,
        IRichTextListItemRuntime& runtime);
};

}  // namespace pal4
