#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct ActorManagerAttachmentRequest {
    std::array<std::uint32_t, 5> text;
    std::uint32_t slot;
    std::uint32_t active;
};

class IActorManagerPlayerEquipmentRuntime {
public:
    virtual ~IActorManagerPlayerEquipmentRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t InventorySubsystem() = 0;
    [[nodiscard]] virtual std::uint32_t FindEquipment(
        std::uint32_t collection, std::uint32_t id,
        std::uint32_t type) = 0;
    // Original4095B0 returns zero for equal length-aware text, nonzero otherwise.
    [[nodiscard]] virtual bool TextDiffers(std::uint32_t first,
                                            std::uint32_t second) = 0;
    [[nodiscard]] virtual std::uint32_t LoadLeft(
        std::uint32_t animation,
        const ActorManagerAttachmentRequest& request) = 0;
    [[nodiscard]] virtual std::uint32_t LoadRight(
        std::uint32_t animation,
        const ActorManagerAttachmentRequest& request) = 0;
};

class ActorManagerPlayerEquipmentAttachments final {
public:
    static constexpr std::uint32_t kEmptyText = 0x841964u;
    static constexpr std::uint32_t kEmptyError = 0x8dc780u;
    static constexpr std::uint32_t kVersion = 0x90dec0u;
    static constexpr std::uint32_t kJqja = 0x8b62a8u;
    static constexpr std::uint32_t kJqj1 = 0x8b62b0u;
    static constexpr std::uint32_t kJq02a = 0x8b62b8u;
    static constexpr std::uint32_t kJq02 = 0x8b62c0u;
    static constexpr std::uint32_t kWta = 0x8b62a4u;
    static constexpr std::uint32_t kWl01 = 0x8b629cu;
    static constexpr std::uint32_t kWm01 = 0x8b6294u;

    [[nodiscard]] static std::uint32_t Refresh(
        std::uint32_t actor, std::uint32_t animation,
        IActorManagerPlayerEquipmentRuntime& runtime);
};

}  // namespace pal4
