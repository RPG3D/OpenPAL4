#pragma once

#include "pal4/pal_actor_control.h"

#include <array>
#include <cstdint>

namespace pal4 {

struct ActorControlVector3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

// A RenderWare matrix is 64 raw bytes. Its padding/flags are retained.
struct ActorControlMatrix {
    std::array<std::uint32_t, 16> words{};
};
static_assert(sizeof(ActorControlMatrix) == 64u);

struct ActorControlRotation {
    ActorControlVector3 axis{};
    float angle = 0.0f;
};

class IActorControlTransformRuntime {
public:
    virtual ~IActorControlTransformRuntime() = default;
    [[nodiscard]] virtual ActorControlMatrix CameraMatrix(
        std::uint32_t camera) = 0;
    [[nodiscard]] virtual ActorControlMatrix ActorTransform(
        std::uint32_t actor) = 0;
    virtual void SetupTransformMatrix(ActorControlMatrix& destination,
                                      const ActorControlMatrix& source) = 0;
    virtual void NormalizeBasis(ActorControlMatrix& matrix,
                                std::uint32_t byte_offset) = 0;
    [[nodiscard]] virtual ActorControlMatrix Multiply(
        const ActorControlMatrix& camera,
        const ActorControlMatrix& actor) = 0;
    [[nodiscard]] virtual ActorControlRotation QueryRotation(
        const ActorControlMatrix& matrix) = 0;
    [[nodiscard]] virtual ActorControlVector3 CameraVector(
        std::uint32_t camera, std::uint32_t byte_offset) = 0;
};

class ActorControlTransformRuntimeStub final
    : public IActorControlTransformRuntime {
public:
    ActorControlMatrix CameraMatrix(std::uint32_t) override;
    ActorControlMatrix ActorTransform(std::uint32_t) override;
    void SetupTransformMatrix(ActorControlMatrix&,
                              const ActorControlMatrix&) override;
    void NormalizeBasis(ActorControlMatrix&, std::uint32_t) override;
    ActorControlMatrix Multiply(const ActorControlMatrix&,
                                const ActorControlMatrix&) override;
    ActorControlRotation QueryRotation(const ActorControlMatrix&) override;
    ActorControlVector3 CameraVector(std::uint32_t,
                                    std::uint32_t) override;
};

class PalActorControlTransform final {
public:
    // IDA 0x424150. The optional output receives the queried axis Y.
    [[nodiscard]] static float Compute(
        PalActorControlState& state, std::uint32_t camera,
        std::uint32_t actor, float* axis_y,
        IActorControlTransformRuntime& runtime);
};

}  // namespace pal4
