#pragma once

#include "pal4/pal_actor_control_blend_tail_config.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct ActorControlXmlConfigDocument {
    std::array<std::uint32_t, 15> words{};
};
static_assert(sizeof(ActorControlXmlConfigDocument) == 60u);

using ActorControlAudioPoint = std::array<float, 3>;

class IActorControlConfigInitRuntime {
public:
    virtual ~IActorControlConfigInitRuntime() = default;
    virtual void InitializeDocument(ActorControlXmlConfigDocument& document,
                                    std::string_view root, std::uint32_t flag);
    virtual void InitializeFacet(ActorControlXmlWriteFacet& facet,
                                 std::uint32_t argument);
    [[nodiscard]] virtual bool CheckVersionAndType(
        ActorControlXmlWriteFacet& facet,
        ActorControlXmlConfigDocument& document);
    [[nodiscard]] virtual std::uint32_t FindAttribute(
        ActorControlXmlConfigDocument& document, std::string_view name);
    [[nodiscard]] virtual std::uint32_t AttributeToInt(
        std::uint32_t attribute, const std::uint32_t* default_value);
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address);
    [[nodiscard]] virtual std::uint32_t MinimapManager();
    virtual void AddIcon(std::uint32_t manager, std::string_view name,
                         std::uint32_t index, std::string_view texture,
                         std::uint32_t first_flag, std::uint32_t second_flag);
    virtual void SetActiveIcon(std::uint32_t manager,
                               std::string_view name, std::uint32_t index);
    virtual void UpdateGrid(std::uint32_t manager,
                            std::uint32_t position,
                            std::uint32_t grid_state);
    [[nodiscard]] virtual long double ActorScale(std::uint32_t actor);
    [[nodiscard]] virtual std::uint32_t MovementAdapter();
    virtual void UpdateMovement(std::uint32_t adapter,
                                std::uint32_t position,
                                float first, float second,
                                std::uint32_t flag);
    [[nodiscard]] virtual std::uint32_t FloorAttributeManager();
    virtual void UpdateFloorAttributes(std::uint32_t manager);
    [[nodiscard]] virtual std::uint32_t CameraSubsystem();
    [[nodiscard]] virtual std::uint32_t ActiveCamera(
        std::uint32_t subsystem);
    [[nodiscard]] virtual ActorControlAudioPoint ReadPoint(
        std::uint32_t address);
    [[nodiscard]] virtual std::uint32_t AudioManager();
    [[nodiscard]] virtual std::uint32_t AudioDriver(
        std::uint32_t manager);
    virtual void SetAudioPosition(std::uint32_t driver,
                                  const ActorControlAudioPoint& point);
    virtual void SetAudioOrientation(
        std::uint32_t driver, const ActorControlAudioPoint& direction,
        const ActorControlAudioPoint& up);
    virtual void ProcessInput(PalActorControlState& state);
    virtual void ProcessInputAndClearVector(PalActorControlState& state);
    [[nodiscard]] virtual std::uint32_t ControllerAccess();
    virtual void ResetAndSync(std::uint32_t access, std::uint32_t flag);
    virtual void ResetFacetVtable(ActorControlXmlWriteFacet& facet);
    virtual void DestroyDocument(ActorControlXmlConfigDocument& document);
};

class PalActorControlConfigInitializer final {
public:
    // IDA 0x427160. Returns the low byte of the success gate.
    [[nodiscard]] static std::uint8_t Initialize(
        PalActorControlState& state, std::uint32_t argument,
        IActorControlConfigInitRuntime& runtime);
};

}  // namespace pal4
