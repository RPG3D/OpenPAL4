#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace pal4 {

struct CameraControllerState {
    std::array<std::uint32_t, 0xc4u / 4u> words{};

    [[nodiscard]] std::uint32_t& At(std::size_t offset) {
        return words[offset / 4u];
    }
    [[nodiscard]] const std::uint32_t& At(std::size_t offset) const {
        return words[offset / 4u];
    }
};
static_assert(sizeof(CameraControllerState) == 0xc4u);

class ICameraControllerRuntime {
public:
    virtual ~ICameraControllerRuntime() = default;
    virtual void SetName(std::uint32_t& name_pointer,
                         const char* name) = 0;
    virtual void DestroyNameObject(CameraControllerState& state,
                                   std::uint32_t offset) = 0;
    [[nodiscard]] virtual std::uint32_t FreeData(
        std::uint32_t data) = 0;
};

class CameraController final {
public:
    explicit CameraController(ICameraControllerRuntime& runtime)
        : runtime_(runtime) {}

    // 0x5eaf90: initialize controller fields, name and callback table.
    CameraControllerState& Construct(CameraControllerState& state);
    // 0x5eb040: destroy nested name, free data and clear controller slots.
    [[nodiscard]] std::uint32_t DestroyComponent(
        CameraControllerState& state);
    // 0x5ebc50: reset the dynamic collection end and return its old value.
    [[nodiscard]] static std::uint32_t ClearDynamicCollection(
        CameraControllerState& state);

private:
    ICameraControllerRuntime& runtime_;
};

}  // namespace pal4
