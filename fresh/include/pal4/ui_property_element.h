#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class IndexedPropertyGrid;

class IUiPropertyElementRuntime {
public:
    virtual ~IUiPropertyElementRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t AssignCeguiString(
        std::uint32_t, std::uint32_t, std::uint32_t,
        std::uint32_t) { return 0; }
    virtual void CopyConstructCeguiString(std::uint32_t,
                                          std::uint32_t) {}
    virtual void DestroyElementString(std::uint32_t) {}
};

struct UiFloatRect {
    float left = 0;
    float right = 0;
    float top = 0;
    float bottom = 0;
};

class IUiPropertyRenderRuntime : public IUiPropertyElementRuntime {
public:
    virtual void PrepareWhiteColour() {}
    virtual void DrawImage(std::uint32_t, const UiFloatRect&, float) {}
    [[nodiscard]] virtual float ReadFontMetric(std::uint32_t) { return 0; }
    virtual void PrepareDefaultFontColours() {}
    virtual void DrawText(std::uint32_t, std::uint32_t,
                          const UiFloatRect&, float, std::uint32_t) {}
};

class UiPropertyElement final {
public:
    // IDA 0x5aadb0: this object's last written byte is at offset 196.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t object, std::uint32_t property,
        const std::array<std::int32_t, 2>& origin,
        const std::array<std::int32_t, 2>& extent,
        const std::array<std::uint32_t, 3>& images,
        IUiPropertyElementRuntime& runtime);
    // IDA 0x5aae30/0x5aae60.
    [[nodiscard]] static std::uint32_t SetOverlayImages(
        std::uint32_t object,
        const std::array<std::uint32_t, 3>& images,
        IUiPropertyElementRuntime& runtime);
    [[nodiscard]] static std::uint32_t AssignText(
        std::uint32_t object, std::uint32_t source,
        IUiPropertyElementRuntime& runtime);
    // IDA 0x5acd20/0x5ace80: 200-byte record copy and assignment.
    static void CopyConstruct(std::uint32_t destination,
                              std::uint32_t source,
                              IUiPropertyElementRuntime& runtime);
    [[nodiscard]] static std::uint32_t Assign(
        std::uint32_t destination, std::uint32_t source,
        IUiPropertyElementRuntime& runtime);
    // IDA 0x5b4450/0x5b4480: iterate contiguous 200-byte records.
    static void DestroyRange(std::uint32_t begin, std::uint32_t end,
                             IUiPropertyElementRuntime& runtime);
    [[nodiscard]] static std::uint32_t AssignRange(
        std::uint32_t source_begin, std::uint32_t source_end,
        std::uint32_t destination, IUiPropertyElementRuntime& runtime);
    // IDA 0x5aae80/0x5aaf10.
    [[nodiscard]] static bool CanOccupy(
        std::uint32_t object,
        const std::array<std::int32_t, 2>& origin,
        const IndexedPropertyGrid& grid,
        IUiPropertyElementRuntime& runtime);
    [[nodiscard]] static std::int32_t MoveAndPaint(
        std::uint32_t object,
        const std::array<std::int32_t, 2>& origin,
        IndexedPropertyGrid& grid,
        IUiPropertyElementRuntime& runtime);
    // IDA 0x5aafb0.
    static void Render(std::uint32_t object,
                       const std::array<float, 2>& origin,
                       const std::array<float, 2>& scale,
                       float depth, std::uint32_t font, bool draw_front,
                       IUiPropertyRenderRuntime& runtime);
    // IDA 0x5ab1e0/0x5ab230/0x5ab290.
    [[nodiscard]] static bool ContainsPoint(
        std::uint32_t object,
        const std::array<std::int32_t, 2>& point,
        IUiPropertyElementRuntime& runtime);
    [[nodiscard]] static UiFloatRect ScreenRect(
        std::uint32_t object,
        const std::array<float, 2>& origin,
        const std::array<float, 2>& scale,
        IUiPropertyElementRuntime& runtime);
    [[nodiscard]] static std::array<std::int32_t, 2> AdjustPosition(
        std::uint32_t object,
        const std::array<std::int32_t, 2>& target,
        IUiPropertyElementRuntime& runtime);
};

}  // namespace pal4
