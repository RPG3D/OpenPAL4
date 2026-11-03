#pragma once

#include "pal4/native_base_object_memory.h"
#include "pal4/native_script_player_position_runtime.h"
#include "pal4/pal_floor_attribute.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

namespace pal4 {

class NativeScenePlayerMovement;
class NativeFloorCombatAreas;

class NativeFloorAttributeOwner final : public INativeScriptFloorColor,
                                        private PalFloorAttributeRuntimeStub {
public:
    explicit NativeFloorAttributeOwner(NativeScenePlayerMovement& movement);
    NativeFloorAttributeOwner(const NativeFloorAttributeOwner&) = delete;
    NativeFloorAttributeOwner& operator=(const NativeFloorAttributeOwner&) = delete;
    ~NativeFloorAttributeOwner() override;

    void Update() override;
    [[nodiscard]] std::string ResolveCombatArea(const NativeFloorCombatAreas& areas,
        std::string_view world) const;
    void ApplyColor(std::uint32_t pixel);
    [[nodiscard]] std::uint32_t StateAddress() const noexcept {
        return static_cast<std::uint32_t>(
            reinterpret_cast<std::uintptr_t>(state_));
    }
    [[nodiscard]] std::uint32_t PixelFromRgba(std::uint32_t rgba,
                                               std::uint32_t format);
    [[nodiscard]] std::uint32_t AddColorCallback(
        std::uint32_t pixel, std::uint32_t context,
        std::uint32_t callback);
    [[nodiscard]] std::uint32_t RemoveColorCallbacks(
        std::uint32_t context, std::uint32_t callback);
    void BindTemperatureCallback(
        std::uint32_t context,
        std::function<void(std::uint32_t)> callback);
    void UnbindTemperatureCallback(std::uint32_t context) noexcept;
    void Release();
    [[nodiscard]] const PalFloorAttributeState& State() const noexcept {
        return *state_;
    }
    [[nodiscard]] std::size_t OutstandingAllocations() const noexcept {
        return memory_.OutstandingAllocations();
    }

private:
    std::int32_t FloatToLong(long double value) override;
    std::uint8_t ReadByte(std::uint32_t address) override;
    std::uint32_t ReadDword(std::uint32_t address) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    void WriteDword(std::uint32_t address, std::uint32_t value) override;
    std::uint32_t EmptyTextAddress() override;
    void SetErrorFlag(std::uint32_t record, std::uint8_t flag) override;
    bool AllocateText(std::uint32_t record, std::uint32_t size,
                      std::uint8_t mode) override;
    void DetachAndReset(std::uint32_t record) override;
    void SetTextSize(std::uint32_t record, std::uint32_t size) override;
    void CopyBytes(std::uint32_t destination, std::uint32_t source,
                   std::uint32_t size) override;
    PalFloorAttributeState* Allocate(std::uint32_t bytes) override;
    std::uint32_t Free(PalFloorAttributeState* state) override;
    std::uint32_t CreateEmptyList() override;
    std::uint32_t RgbaToPixel(std::uint32_t rgba,
                              std::uint32_t format) override;
    std::uint32_t PrepareColorValue(std::string_view path,
                                    std::uint32_t pixel) override;
    void ReleaseColorValue(std::uint32_t value) override;
    void ReleaseText(FloorTextRecord& text) override;
    void ReleaseList(PalFloorAttributeState& state) override;
    std::uint32_t MovementAdapter() override;
    std::uint32_t CallMovementVirtual(std::uint32_t object,
                                      std::uint32_t slot_bytes) override;
    std::uint32_t AllocateListNode(std::uint32_t bytes) override;
    std::uint32_t AllocateColorNode(std::uint32_t bytes) override;
    void AssignSoundPath(FloorTextRecord& destination,
                          std::uint32_t source_record) override;
    void AllocateEmptySoundPath(FloorTextRecord& destination) override;
    void InvokeCallback(std::uint32_t function, std::uint32_t context,
                         std::uint32_t pixel) override;
    std::uint32_t NextColorNode(std::uint32_t node) override;
    void DeleteColorNode(std::uint32_t node,
                          FloorColorTreeState& tree) override;
    std::uint32_t FreeAddress(std::uint32_t address) override;

    NativeScenePlayerMovement& movement_;
    NativeBaseObjectMemory memory_;
    PalFloorAttributeState* state_ = nullptr;
    std::uint8_t empty_text_[2]{0xfeu, 0u};
    std::uint32_t temperature_context_ = 0u;
    std::function<void(std::uint32_t)> temperature_callback_;
};

}  // namespace pal4
