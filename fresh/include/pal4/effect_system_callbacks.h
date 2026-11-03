#pragma once

#include <cstdint>
#include <functional>

namespace pal4 {

struct EffectPosition {
    float x;
    float y;
    float z;
};

class IEffectProximityRuntime {
public:
    virtual ~IEffectProximityRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    [[nodiscard]] virtual EffectPosition ObjectPosition(
        std::uint32_t object_handle) = 0;
};

class IEffectSystemCallbackRuntime : public IEffectProximityRuntime {
public:
    ~IEffectSystemCallbackRuntime() override = default;
    [[nodiscard]] virtual std::uint32_t AuxiliarySingleton() = 0;
    [[nodiscard]] virtual std::uint32_t CurrentCamera() = 0;
    [[nodiscard]] virtual std::uint32_t FindElement7(
        std::uint32_t tree, std::uint32_t key) = 0;
    virtual void InvokeEffect(std::uint32_t node,
                              std::uint32_t object) = 0;
    virtual void AddEffect(std::uint32_t auxiliary,
                           std::uint32_t atomic) = 0;
    virtual void RemoveEffect(std::uint32_t auxiliary,
                              std::uint32_t atomic) = 0;
    [[nodiscard]] virtual std::uint32_t ForAllAtomics(
        std::uint32_t clump, std::uint8_t enabled,
        const std::function<std::uint32_t(std::uint32_t,
                                          std::uint8_t)>& callback) = 0;
};

class EffectSystemCallbacks final {
public:
    // IDA 0x63ce40: active proximity gate; +8 holds squared radius.
    [[nodiscard]] static bool ValidateObjectTransform(
        std::uint32_t gate, std::uint32_t object,
        EffectPosition target, IEffectProximityRuntime& runtime);
    // IDA 0x63cea0: find the object's auxiliary-tree callback and invoke it
    // when the object is within the current camera's squared effect radius.
    [[nodiscard]] static std::uint32_t ProcessEffectObject(
        std::uint32_t object, IEffectSystemCallbackRuntime& runtime);
    // IDA 0x63cf80/0x63cfe0: one-atomic and all-atomic dispatch.
    [[nodiscard]] static std::uint32_t ProcessAtomic(
        std::uint32_t atomic, std::uint8_t enabled,
        IEffectSystemCallbackRuntime& runtime);
    [[nodiscard]] static std::uint32_t ProcessAllAtomics(
        std::uint32_t clump, std::uint8_t enabled,
        IEffectSystemCallbackRuntime& runtime);
};

}  // namespace pal4
