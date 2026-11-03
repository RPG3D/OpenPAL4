#pragma once

#include <cstdint>

namespace pal4 {

class IPlayerManagerSlotCopyRuntime {
public:
    virtual ~IPlayerManagerSlotCopyRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address,
                            std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyTextAddress() = 0;
    virtual void EraseText(std::uint32_t record,
                           std::uint32_t start,
                           std::uint32_t count) = 0;
    virtual void SetErrorFlag(std::uint32_t record,
                              std::uint8_t flag) = 0;
    [[nodiscard]] virtual bool AllocateText(std::uint32_t record,
                                             std::uint32_t length,
                                             std::uint8_t mode) = 0;
    virtual void CopyBytes(std::uint32_t destination,
                           std::uint32_t source,
                           std::uint32_t count) = 0;
};

class PlayerManagerSlotCopyRuntimeStub : public IPlayerManagerSlotCopyRuntime {
public:
    std::uint32_t ReadDword(std::uint32_t) override;
    void WriteDword(std::uint32_t, std::uint32_t) override;
    std::uint8_t ReadByte(std::uint32_t) override;
    void WriteByte(std::uint32_t, std::uint8_t) override;
    std::uint32_t EmptyTextAddress() override;
    void EraseText(std::uint32_t, std::uint32_t,
                   std::uint32_t) override;
    void SetErrorFlag(std::uint32_t, std::uint8_t) override;
    bool AllocateText(std::uint32_t, std::uint32_t,
                      std::uint8_t) override;
    void CopyBytes(std::uint32_t, std::uint32_t,
                   std::uint32_t) override;
};

class PlayerManagerSlotDataCopier final {
public:
    static constexpr std::uint32_t kDestinationOffset = 0x9f8u;
    static constexpr std::uint32_t kSourceOffset = 0x90u;
    static constexpr std::uint32_t kRawRecordOffset = 0x658u;
    static constexpr std::uint32_t kEmptyTextAddress = 0x841964u;

    // IDA 0x43ea10: copy two text groups and the 0x90-byte source prefix.
    [[nodiscard]] static std::uint32_t Copy(
        std::uint32_t object, std::uint32_t source,
        IPlayerManagerSlotCopyRuntime& runtime);

private:
    static void CopyTextRecord(std::uint32_t destination,
                               std::uint32_t source,
                               IPlayerManagerSlotCopyRuntime& runtime);
};

}  // namespace pal4
