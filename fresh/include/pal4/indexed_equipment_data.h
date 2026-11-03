#pragma once

#include <cstdint>

namespace pal4 {

class IIndexedEquipmentDataRuntime {
public:
    virtual ~IIndexedEquipmentDataRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address,
                            std::uint32_t value) = 0;
    virtual void SetErrorFlag(std::uint32_t record,
                              std::uint8_t value) = 0;
    virtual void ReallocateText(std::uint32_t record,
                                std::uint32_t length) = 0;
    virtual void CopyErrorText(std::uint32_t destination) = 0;
    virtual void RegisterCleanup() = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class IndexedEquipmentDataRuntimeStub : public IIndexedEquipmentDataRuntime {
public:
    std::uint8_t ReadByte(std::uint32_t) override;
    void WriteByte(std::uint32_t, std::uint8_t) override;
    std::uint32_t ReadDword(std::uint32_t) override;
    void WriteDword(std::uint32_t, std::uint32_t) override;
    void SetErrorFlag(std::uint32_t, std::uint8_t) override;
    void ReallocateText(std::uint32_t, std::uint32_t) override;
    void CopyErrorText(std::uint32_t) override;
    void RegisterCleanup() override;
    void Free(std::uint32_t) override;
};

class IndexedEquipmentData final {
public:
    static constexpr std::uint32_t kGuard = 0x978af8u;
    static constexpr std::uint32_t kFallback = 0x978b00u;

    // IDA 0x43f290: valid 16-byte record or process-wide ERROR fallback.
    [[nodiscard]] static std::uint32_t Get(
        std::uint32_t item, std::int32_t index,
        IIndexedEquipmentDataRuntime& runtime);
    // IDA 0x43f3b0: release the fallback string on CRT exit.
    static void Cleanup(IIndexedEquipmentDataRuntime& runtime);
};

}  // namespace pal4
