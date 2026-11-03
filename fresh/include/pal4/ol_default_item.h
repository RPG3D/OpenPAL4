#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

using OlRect = std::array<float, 4>;

class IOlDefaultItemRuntime {
public:
    virtual ~IOlDefaultItemRuntime() = default;
    virtual void ConstructBase(std::uint32_t item, std::uint32_t list,
                               std::uint32_t text) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t FindImage(
        std::string_view imageset, std::string_view image) = 0;
    [[nodiscard]] virtual std::uint32_t BaseTextVtable() = 0;
    [[nodiscard]] virtual std::uint32_t BaseItemVtable() = 0;
    virtual void DestroyString(std::uint32_t address) = 0;
    virtual void DrawBase(std::uint32_t item, std::uint32_t cache,
                          const OlRect& rect, float alpha, float modulation,
                          std::uint32_t clip, std::uint32_t format) = 0;
    virtual void CacheImage(std::uint32_t cache, std::uint32_t image,
                            const OlRect& rect, float alpha,
                            std::uint32_t clip,
                            const std::array<float, 4>& colour,
                            bool first_flag, bool second_flag) = 0;
};

class OlDefaultItem final {
public:
    static constexpr std::uint32_t kVtable = 0x844204u;

    // IDA 0x4b31c0.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t item, std::uint32_t list, std::uint32_t text,
        bool equipped, IOlDefaultItemRuntime& runtime);
    // IDA 0x4b3330.
    static void Destruct(std::uint32_t item,
                         IOlDefaultItemRuntime& runtime);
    // IDA 0x4b33a0.
    static void Render(std::uint32_t item, std::uint32_t cache,
                       const OlRect& rect, float alpha, float modulation,
                       std::uint32_t clip, std::uint32_t format,
                       IOlDefaultItemRuntime& runtime);
};

}  // namespace pal4
