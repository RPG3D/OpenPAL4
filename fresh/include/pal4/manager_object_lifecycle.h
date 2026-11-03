#pragma once

#include <cstdint>

namespace pal4 {

class IManagerObjectLifecycleRuntime {
public:
    virtual ~IManagerObjectLifecycleRuntime() = default;
    virtual void WriteVtable(std::uint32_t object,
                             std::uint32_t vtable) = 0;
    [[nodiscard]] virtual std::uint8_t Cleanup(std::uint32_t object) = 0;
    virtual void CleanupEffect(std::uint32_t object) = 0;
    virtual void CleanupExtended(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint8_t CleanupSecondary(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address, std::uint32_t value) = 0;
    virtual void Free(std::uint32_t object) = 0;
};

class ManagerObjectLifecycleRuntimeStub final
    : public IManagerObjectLifecycleRuntime {
public:
    void WriteVtable(std::uint32_t object,
                     std::uint32_t vtable) override;
    [[nodiscard]] std::uint8_t Cleanup(std::uint32_t object) override;
    void CleanupEffect(std::uint32_t object) override;
    void CleanupExtended(std::uint32_t object) override;
    [[nodiscard]] std::uint8_t CleanupSecondary(
        std::uint32_t object) override;
    [[nodiscard]] std::uint32_t ReadDword(std::uint32_t address) override;
    void WriteDword(std::uint32_t address, std::uint32_t value) override;
    void Free(std::uint32_t object) override;
};

class ManagerObjectLifecycle final {
public:
    [[nodiscard]] static std::uint32_t DeleteBase(
        std::uint32_t object, std::uint8_t flags,
        IManagerObjectLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint8_t DestroyFirst(
        std::uint32_t object, IManagerObjectLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t DeleteFirst(
        std::uint32_t object, std::uint8_t flags,
        IManagerObjectLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint8_t DestroyCommon(
        std::uint32_t object, std::uint32_t manager_key,
        IManagerObjectLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t DeleteCommon(
        std::uint32_t object, std::uint32_t manager_key,
        std::uint8_t flags, IManagerObjectLifecycleRuntime& runtime);
    static void DestroyEffect(std::uint32_t object,
                              IManagerObjectLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t DeleteEffect(
        std::uint32_t object, std::uint8_t flags,
        IManagerObjectLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyExtended(
        std::uint32_t object, IManagerObjectLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t DeleteExtended(
        std::uint32_t object, std::uint8_t flags,
        IManagerObjectLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint8_t DestroySecondary(
        std::uint32_t object, IManagerObjectLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t DeleteSecondary(
        std::uint32_t object, std::uint8_t flags,
        IManagerObjectLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t ConstantTrue();
};

}  // namespace pal4
