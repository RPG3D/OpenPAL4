#pragma once

#include "pal4/animation_event_queue.h"
#include "pal4/combat_camera_access.h"
#include "pal4/ui_text_image_overlay.h"

#include <array>
#include <cstdint>

namespace pal4 {

class ICombatConsoleVisualRuntime : public IAnimationEventQueueRuntime,
                                  public ICombatCameraRuntime,
                                  public IUiTextImageOverlayRuntime {
public:
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) override = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) override = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) override = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) override = 0;
    // Original GetInstance result is discarded; preserve its required side
    // effect without inventing a native CombatSystem identity.
    virtual void EnsureCombatSystem() = 0;
    // Ownership transfers only on successful return. Native actor owners must
    // use their own queue allocator/guards, not this activity allocator.
    virtual std::uint32_t EnqueueActorAnimation(std::uint32_t actor,
        std::uint32_t animation,bool staggered);
    [[nodiscard]] virtual std::uint32_t ActorScreenPosition(
        std::uint32_t actor, std::uint32_t camera,
        std::array<std::uint32_t, 2>& position) = 0;
    [[nodiscard]] virtual std::uint32_t DialogData(
        std::uint32_t argument) = 0;
};

class CombatConsoleImageEvents final {
public:
    static constexpr std::uint32_t kElementSize = 72u;
    static constexpr std::uint32_t kActorQueueOffset = 0x2d8u;

    // IDA 0x540020. event+4 is an actor pointer.
    [[nodiscard]] static std::uint32_t CreateMissImage(
        std::uint32_t event, ICombatConsoleVisualRuntime& runtime);
    // IDA 0x540270. The dialog actor is looked up again for queue insertion.
    [[nodiscard]] static std::uint32_t CreateImmuneImage(
        std::uint32_t argument, ICombatConsoleVisualRuntime& runtime);
    // IDA 0x5408b0.
    [[nodiscard]] static std::uint32_t CreateCounterImage(
        std::uint32_t argument, ICombatConsoleVisualRuntime& runtime);

private:
    [[nodiscard]] static std::uint32_t CreateImage(
        std::uint32_t argument, bool from_dialog,
        const char* image_name, ICombatConsoleVisualRuntime& runtime);
};

}  // namespace pal4
