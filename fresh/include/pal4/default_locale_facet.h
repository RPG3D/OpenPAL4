#pragma once

#include <cstdint>

namespace pal4 {

class IDefaultLocaleFacetRuntime {
public:
    virtual ~IDefaultLocaleFacetRuntime() = default;
    virtual void Write(std::uint32_t address, std::uint32_t value) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t RegisterExitHandler(
        std::uint32_t function_address) = 0;
};

class DefaultLocaleFacet final {
public:
    static constexpr std::uint32_t kGlobalAddress = 0x8dec88u;
    static constexpr std::uint32_t kVtableAddress = 0x841938u;
    static constexpr std::uint32_t kResetEntry = 0x403a40u;

    static void Set(IDefaultLocaleFacetRuntime& runtime);
    static void Reset(IDefaultLocaleFacetRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeAndRegisterExit(
        IDefaultLocaleFacetRuntime& runtime);
};

class LocaleFacetLifecycle final {
public:
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, std::uint32_t flags,
        IDefaultLocaleFacetRuntime& runtime);
};

class ActorMessageLocaleFacet final {
public:
    static constexpr std::uint32_t kVtableAddress = 0x841e80u;

    static void SetVtable(std::uint32_t object,
                          IDefaultLocaleFacetRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, std::uint32_t flags,
        IDefaultLocaleFacetRuntime& runtime);
};

class SystemEventLocaleFacet final {
public:
    static constexpr std::uint32_t kBaseVtable = 0x84491cu;
    static constexpr std::uint32_t kDataVtable = 0x844928u;

    static void ResetBase(std::uint32_t object,
                          IDefaultLocaleFacetRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyBase(
        std::uint32_t object, std::uint32_t flags,
        IDefaultLocaleFacetRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeData(
        std::uint32_t object, IDefaultLocaleFacetRuntime& runtime);
    static void CleanupData(std::uint32_t object,
                            IDefaultLocaleFacetRuntime& runtime);
    static void ResetData(std::uint32_t object,
                          IDefaultLocaleFacetRuntime& runtime);
};

class ActorManagerLocaleFacet final {
public:
    static constexpr std::uint32_t kVtableAddress = 0x844cccu;

    static void Reset(std::uint32_t object,
                      IDefaultLocaleFacetRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, std::uint32_t flags,
        IDefaultLocaleFacetRuntime& runtime);
};

}  // namespace pal4
