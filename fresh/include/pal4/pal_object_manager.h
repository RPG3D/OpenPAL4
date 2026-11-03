#pragma once

#include <cstdint>

namespace pal4 {

struct PalObjectManagerRegistration final {
    std::uint32_t cached = 0u;
    std::uint8_t guard = 0u;
};

class IPalObjectManagerRuntime {
public:
    virtual ~IPalObjectManagerRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetSingletonManager() = 0;
    [[nodiscard]] virtual std::uint32_t Register(
        std::uint32_t manager, const char* name, std::uint32_t id,
        std::uint32_t create_callback, std::uint32_t destroy_callback) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t object) = 0;
    virtual void InitializeBase(std::uint32_t object) = 0;
    virtual void ConstructLock(std::uint32_t address) = 0;
    virtual void SetReleaseCallback(std::uint32_t object,
                                    std::uint32_t callback) = 0;
    virtual void SetCompareCallback(std::uint32_t object,
                                    std::uint32_t callback) = 0;
    virtual void SetCopyCallback(std::uint32_t object,
                                 std::uint32_t callback) = 0;
    virtual void SetFreeCallback(std::uint32_t object,
                                 std::uint32_t callback) = 0;
    virtual void SetName(std::uint32_t object, const char* name) = 0;
    virtual void DestructorThunk(std::uint32_t object) = 0;
};

class PalObjectManagerRuntimeStub final : public IPalObjectManagerRuntime {
public:
    [[nodiscard]] std::uint32_t GetSingletonManager() override { return 0u; }
    [[nodiscard]] std::uint32_t Register(
        std::uint32_t, const char*, std::uint32_t,
        std::uint32_t, std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t Allocate(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t Free(std::uint32_t) override { return 0u; }
    void InitializeBase(std::uint32_t) override {}
    void ConstructLock(std::uint32_t) override {}
    void SetReleaseCallback(std::uint32_t, std::uint32_t) override {}
    void SetCompareCallback(std::uint32_t, std::uint32_t) override {}
    void SetCopyCallback(std::uint32_t, std::uint32_t) override {}
    void SetFreeCallback(std::uint32_t, std::uint32_t) override {}
    void SetName(std::uint32_t, const char*) override {}
    void DestructorThunk(std::uint32_t) override {}
};

class PalObjectManager final {
public:
    static constexpr std::uint32_t kSize = 40u;
    static constexpr std::uint32_t kSingletonId = 1022u;
    static constexpr std::uint32_t kCreateCallback = 0x5fc520u;
    static constexpr std::uint32_t kDestroyCallback = 0x5fc580u;
    static constexpr std::uint32_t kReleaseCallback = 0x7de5d0u;
    static constexpr std::uint32_t kCompareCallback = 0x5fc630u;
    static constexpr std::uint32_t kCopyCallback = 0x5eaf50u;
    static constexpr std::uint32_t kFreeCallback = 0x5fc6c0u;

    // IDA 0x5fc4e0.
    [[nodiscard]] static std::uint32_t RegisterSingleton(
        PalObjectManagerRegistration& registration,
        IPalObjectManagerRuntime& runtime);
    // IDA 0x5fc520.
    [[nodiscard]] static std::uint32_t Create(
        IPalObjectManagerRuntime& runtime);
    // IDA 0x5fc580.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, IPalObjectManagerRuntime& runtime);
    // IDA 0x5fc5a0.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t object, IPalObjectManagerRuntime& runtime);
    // IDA 0x5fc620.
    static void ForwardDestructor(
        std::uint32_t object, IPalObjectManagerRuntime& runtime);
    // IDA 0x5fc6c0.
    [[nodiscard]] static std::uint32_t FreeThunk(
        std::uint32_t object, IPalObjectManagerRuntime& runtime);
};

}  // namespace pal4
