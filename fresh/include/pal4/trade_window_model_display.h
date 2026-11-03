#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace pal4 {

class ITradeWindowModelRuntime {
public:
    virtual ~ITradeWindowModelRuntime() = default;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t RemoveGameObject(
        std::uint32_t collection, std::uint32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t FindGameObject(
        std::uint32_t collection, std::uint32_t id) = 0;
    [[nodiscard]] virtual std::string_view ObjectName(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t SetTexture(
        std::uint32_t object, std::uint32_t texture) = 0;
    virtual void ProcessObjectScript() = 0;
    virtual void ProcessWeaponScript() = 0;
    [[nodiscard]] virtual std::uint32_t CreateGameObject(
        std::uint32_t collection, std::string_view path,
        std::string_view name, bool flag) = 0;
    virtual void SetScaleFromMember(std::uint32_t object,
                                    std::uint32_t scale_address) = 0;
    virtual void SetObjectVector(std::uint32_t object,
                                 const std::array<float, 3>& vector,
                                 std::uint32_t flag) = 0;
    virtual void SetPosition(std::uint32_t object,
                             const std::array<float, 3>& position) = 0;
    virtual void SetRotation(std::uint32_t object, std::uint32_t axis,
                             const std::array<float, 3>& rotation) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupScripts() = 0;
};

class TradeWindowModelDisplay final {
public:
    // IDA 0x48cdd0. The original passes the frame in ECX.
    [[nodiscard]] static std::uint32_t SetGameObjectDisplay(
        std::uint32_t frame, std::optional<std::string_view> name,
        std::uint32_t texture, std::optional<std::string_view> path,
        ITradeWindowModelRuntime& runtime);
};

}  // namespace pal4
