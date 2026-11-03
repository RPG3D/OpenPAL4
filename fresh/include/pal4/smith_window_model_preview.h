#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace pal4 {

class ISmithWindowModelPreviewRuntime {
public:
    virtual ~ISmithWindowModelPreviewRuntime() = default;
    [[nodiscard]] virtual std::uint32_t FrameCollection() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t RemoveGameObject(
        std::uint32_t collection, std::uint32_t id) = 0;
    virtual void LoadFlowerScript() = 0;
    virtual void LoadObjectScript() = 0;
    virtual void LoadWeaponScript() = 0;
    [[nodiscard]] virtual std::uint32_t CleanupScripts() = 0;
    [[nodiscard]] virtual std::uint32_t CreateGameObject(
        std::uint32_t collection, std::string_view path,
        std::string_view name, bool active) = 0;
    [[nodiscard]] virtual std::uint32_t FindGameObject(
        std::uint32_t collection, std::uint32_t id) = 0;
    [[nodiscard]] virtual std::string_view ObjectName(
        std::uint32_t object) = 0;
    virtual void SetScale(std::uint32_t object,
                          const std::array<float, 3>& scale) = 0;
    virtual void SetPosition(std::uint32_t object,
                             const std::array<float, 3>& position) = 0;
    virtual void SetTexture(std::uint32_t object,
                            std::optional<std::string_view> texture) = 0;
    virtual void SetObjectVector(std::uint32_t object,
                                 const std::array<float, 3>& value,
                                 std::uint32_t flag) = 0;
    virtual void SetRotation(std::uint32_t object, std::uint32_t axis,
                             const std::array<float, 3>& rotation) = 0;
};

class SmithWindowModelPreview final {
public:
    // IDA 0x49eaa0.
    [[nodiscard]] static std::uint32_t Update(
        std::uint32_t object,
        std::optional<std::string_view> model_name,
        std::optional<std::string_view> texture,
        std::optional<std::string_view> class_path,
        ISmithWindowModelPreviewRuntime& runtime);
};

}  // namespace pal4
