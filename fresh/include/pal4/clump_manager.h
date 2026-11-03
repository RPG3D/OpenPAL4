#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

enum class ClumpManagerCallback {
    create, destroy, free_memory, compare, set_current_clump, no_op
};

class IClumpManagerRuntime {
public:
    virtual ~IClumpManagerRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t SingletonManager() = 0;
    [[nodiscard]] virtual std::uint32_t RegisterSingleton(
        std::uint32_t manager, std::string_view name, std::uint32_t id,
        ClumpManagerCallback factory, ClumpManagerCallback destroy) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
    virtual void InitializeContainer(std::uint32_t container) = 0;
    virtual void DestroyContainer(std::uint32_t container) = 0;
    virtual void CleanupString(std::uint32_t record) = 0;
    virtual void SetContainerName(std::uint32_t container,
                                  std::string_view name) = 0;
    virtual void SetContainerFlag(std::uint32_t container,
                                  std::uint32_t value) = 0;
    virtual void SetContainerCallback(
        std::uint32_t container, ClumpManagerCallback callback) = 0;
    [[nodiscard]] virtual std::uint32_t CreateLoadCollection() = 0;
    virtual void ScanClumpFile(std::uint32_t collection,
                               std::string_view filename) = 0;
    [[nodiscard]] virtual int LoadCollectionCount(std::uint32_t collection) = 0;
    [[nodiscard]] virtual std::uint32_t LoadCollectionElement(
        std::uint32_t collection, int index) = 0;
    [[nodiscard]] virtual std::uint32_t LowercaseString(
        std::uint32_t string_record) = 0;
    virtual void CopyStringRecord(std::uint32_t destination,
                                  std::uint32_t source) = 0;
    virtual void ClearLoadCollection(std::uint32_t collection) = 0;
    virtual void DestroyLoadCollection(std::uint32_t collection) = 0;
    [[nodiscard]] virtual std::uint32_t CreateLookupString(
        std::string_view text) = 0;
    [[nodiscard]] virtual int FindStringSubstring(
        std::uint32_t query_record, std::uint32_t candidate,
        std::uint32_t start_offset) = 0;
    virtual void CleanupLookupString(std::uint32_t record) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupEmbeddedContainer(
        std::uint32_t container) = 0;
};

class ClumpManagerRuntimeStub final : public IClumpManagerRuntime {
public:
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteByte(std::uint32_t, std::uint8_t) override {}
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t SingletonManager() override { return 0u; }
    [[nodiscard]] std::uint32_t RegisterSingleton(
        std::uint32_t, std::string_view, std::uint32_t,
        ClumpManagerCallback, ClumpManagerCallback) override { return 0u; }
    [[nodiscard]] std::uint32_t Allocate(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t Free(std::uint32_t) override { return 0u; }
    void InitializeContainer(std::uint32_t) override {}
    void DestroyContainer(std::uint32_t) override {}
    void CleanupString(std::uint32_t) override {}
    void SetContainerName(std::uint32_t, std::string_view) override {}
    void SetContainerFlag(std::uint32_t, std::uint32_t) override {}
    void SetContainerCallback(std::uint32_t, ClumpManagerCallback) override {}
    [[nodiscard]] std::uint32_t CreateLoadCollection() override { return 0u; }
    void ScanClumpFile(std::uint32_t, std::string_view) override {}
    [[nodiscard]] int LoadCollectionCount(std::uint32_t) override { return 0; }
    [[nodiscard]] std::uint32_t LoadCollectionElement(std::uint32_t,
                                                      int) override { return 0u; }
    [[nodiscard]] std::uint32_t LowercaseString(std::uint32_t record) override {
        return record;
    }
    void CopyStringRecord(std::uint32_t, std::uint32_t) override {}
    void ClearLoadCollection(std::uint32_t) override {}
    void DestroyLoadCollection(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t CreateLookupString(std::string_view) override {
        return 0u;
    }
    [[nodiscard]] int FindStringSubstring(std::uint32_t, std::uint32_t,
                                          std::uint32_t) override { return -1; }
    void CleanupLookupString(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t CleanupEmbeddedContainer(
        std::uint32_t) override { return 0u; }
};

class ClumpManager final {
public:
    // IDA 0x5fa610.
    [[nodiscard]] static std::uint32_t Register(
        IClumpManagerRuntime& runtime);
    // IDA 0x5fa650.
    [[nodiscard]] static std::uint32_t Create(
        IClumpManagerRuntime& runtime);
    // IDA 0x5fa6b0.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, IClumpManagerRuntime& runtime);
    // IDA 0x5fa6d0.
    [[nodiscard]] static int Compare(std::uint32_t lhs,
                                     std::uint32_t rhs);
    // IDA 0x5fa6f0.
    [[nodiscard]] static std::uint32_t FreeRecord(
        std::uint32_t record, IClumpManagerRuntime& runtime);
    // IDA 0x5fa710.
    [[nodiscard]] static std::uint32_t SetCurrentClump(
        std::uint32_t slot, std::uint32_t clump,
        IClumpManagerRuntime& runtime);
    // IDA 0x5fa720.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t object, IClumpManagerRuntime& runtime);
    // IDA 0x5fa800.
    [[nodiscard]] static std::uint32_t Cleanup(
        std::uint32_t object, IClumpManagerRuntime& runtime);
    // IDA 0x5fa8c0.
    [[nodiscard]] static bool LoadClump(
        std::uint32_t object, std::string_view filename,
        IClumpManagerRuntime& runtime);
    // IDA 0x5fab70.
    [[nodiscard]] static std::uint32_t FindClump(
        std::uint32_t object, std::string_view name,
        IClumpManagerRuntime& runtime);
    // IDA 0x5fac20.
    [[nodiscard]] static std::uint32_t CleanupContainer(
        std::uint32_t object, IClumpManagerRuntime& runtime);
};

}  // namespace pal4
