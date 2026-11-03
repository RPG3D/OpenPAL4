#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

struct HelpItemRect {
    float left{};
    float right{};
    float top{};
    float bottom{};
};

struct HelpImageSize {
    float width{};
    float height{};
};

class IHelpListItemRuntime {
public:
    virtual ~IHelpListItemRuntime() = default;
    virtual void ConstructBase(std::uint32_t item, std::uint32_t list,
                               std::u32string_view text) = 0;
    virtual void DestroyBase(std::uint32_t item) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ResolveImage(
        std::string_view imageset, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t TextItemVtable() = 0;
    [[nodiscard]] virtual std::uint32_t ItemVtable() = 0;
    virtual void DestroyString(std::uint32_t address) = 0;
    virtual void DrawBase(std::uint32_t item, std::uint32_t cache,
                          const HelpItemRect& rect, float alpha,
                          std::uint32_t mode, std::uint32_t clip,
                          std::uint32_t flags) = 0;
    [[nodiscard]] virtual HelpImageSize ImageSize(std::uint32_t image) = 0;
    virtual void CacheImage(std::uint32_t cache, std::uint32_t image,
                            const HelpItemRect& rect, float alpha) = 0;
};

class HelpListItem final {
public:
    static constexpr std::uint32_t kVtable = 0x843fd0u;

    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t item, std::uint32_t list,
        std::u32string_view text, std::uint8_t unread,
        IHelpListItemRuntime& runtime);
    static void Destroy(std::uint32_t item, IHelpListItemRuntime& runtime);
    static void SetImage(std::uint32_t item, std::uint8_t unread,
                         IHelpListItemRuntime& runtime);
    [[nodiscard]] static HelpItemRect UnreadImageRect(const HelpItemRect&,HelpImageSize);
    static void Draw(std::uint32_t item, std::uint32_t cache,
                     const HelpItemRect& rect, float alpha,
                     std::uint32_t mode, std::uint32_t clip,
                     std::uint32_t flags, IHelpListItemRuntime& runtime);
};

}  // namespace pal4
