#pragma once

#include <cstdint>
#include <string>

namespace pal4 {

class IRoleStaticElementsRuntime {
public:
    virtual ~IRoleStaticElementsRuntime() = default;
    [[nodiscard]] virtual std::string ReadPrefix(
        std::uint32_t prefix_data) = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        const std::string& formatted_name) = 0;
    [[nodiscard]] virtual std::uint32_t GetWindow(
        const std::string& name) = 0;
    [[nodiscard]] virtual float GetWidth(
        std::uint32_t window) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupFormatBuffer() = 0;
    [[nodiscard]] virtual std::uint32_t ReleasePrefix(
        std::uint32_t prefix_data) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteFloat(std::uint32_t address, float value) = 0;
};

class IRoleStaticCharacterRuntime {
public:
    virtual ~IRoleStaticCharacterRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetPlayer(
        std::uint32_t role_index) = 0;
    [[nodiscard]] virtual std::int32_t GetMaximum(
        std::uint32_t player, std::uint32_t stat) = 0;
    [[nodiscard]] virtual std::int32_t GetValue(
        std::uint32_t player, std::uint32_t stat) = 0;
    virtual void SetText(std::uint32_t window,
                         const std::string& text) = 0;
    virtual void SetWidth(std::uint32_t window, float width) = 0;
    virtual void SetImage(std::uint32_t window,
                          const char* imageset,
                          const char* image) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupTextBuffer() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
};

class RoleStaticElements final {
public:
    // IDA 0x4b5540. The five arguments include three unused slots;
    // prefix_data is the second and role_index is the fifth.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t object, std::uint32_t unused_first,
        std::uint32_t prefix_data, std::uint32_t unused_third,
        std::uint32_t unused_fourth, std::int32_t role_index,
        IRoleStaticElementsRuntime& runtime);
    // IDA 0x4b5c50.
    [[nodiscard]] static std::uint32_t SetCharacter(
        std::uint32_t object, std::int32_t role_index,
        IRoleStaticCharacterRuntime& runtime);
};

}  // namespace pal4
