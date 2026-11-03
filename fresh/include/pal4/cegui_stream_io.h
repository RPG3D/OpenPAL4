#pragma once

#include <cstdint>
#include <map>

namespace pal4 {

class ICeguiStreamIoRuntime {
public:
    virtual ~ICeguiStreamIoRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address,
                            std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyIosBase(
        std::uint32_t ios_address) = 0;
    virtual void ClearIos(std::uint32_t ios_address,
                          std::uint32_t flags, bool rethrow) = 0;
    [[nodiscard]] virtual bool ProcessCharacterInput(
        std::uint32_t stream_address, std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t ReadStreamBuffer(
        std::uint32_t streambuf_address, std::uint32_t buffer,
        std::uint32_t bytes) = 0;
    virtual void DestroyCeguiIOStream(
        std::uint32_t stream_address) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void InitializeIosBase(std::uint32_t ios_address) = 0;
    virtual void AddStandardStream(std::uint32_t ios_address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
};

class CeguiStreamIoRuntimeStub : public ICeguiStreamIoRuntime {
public:
    [[nodiscard]] std::uint32_t ReadDword(
        std::uint32_t address) override;
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override;
    [[nodiscard]] std::uint32_t DestroyIosBase(
        std::uint32_t ios_address) override;
    void ClearIos(std::uint32_t ios_address,
                  std::uint32_t flags, bool rethrow) override;
    [[nodiscard]] bool ProcessCharacterInput(
        std::uint32_t stream_address,
        std::uint32_t mode) override;
    [[nodiscard]] std::uint32_t ReadStreamBuffer(
        std::uint32_t streambuf_address, std::uint32_t buffer,
        std::uint32_t bytes) override;
    void DestroyCeguiIOStream(
        std::uint32_t stream_address) override;
    void Free(std::uint32_t address) override;
    void InitializeIosBase(std::uint32_t ios_address) override;
    void AddStandardStream(std::uint32_t ios_address) override;
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override;

private:
    std::map<std::uint32_t, std::uint32_t> words_;
    std::map<std::uint32_t, std::uint8_t> bytes_;
};

class CeguiStreamIO final {
public:
    static constexpr std::uint32_t kIosVtable = 0x8682d0u;
    static constexpr std::uint32_t kIstreamVtable = 0x8682c0u;
    static constexpr std::uint32_t kCeguiIstreamBaseVtable = 0x84216cu;

    // 0x416b80, 0x416b90, 0x416bc0, 0x416bd0.
    [[nodiscard]] static std::uint32_t DestroyIos(
        std::uint32_t ios_address, ICeguiStreamIoRuntime& runtime);
    static void SetState(std::uint32_t ios_address,
                         std::uint32_t flags, bool rethrow,
                         ICeguiStreamIoRuntime& runtime);
    // 0x4179e0.
    static void SetStateWithoutPreviousFlags(
        std::uint32_t ios_address, std::uint32_t flags,
        bool rethrow, ICeguiStreamIoRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyIstreamSubobject(
        std::uint32_t subobject_address,
        ICeguiStreamIoRuntime& runtime);
    [[nodiscard]] static std::uint32_t Read(
        std::uint32_t stream_address, std::uint32_t buffer,
        std::uint32_t bytes, ICeguiStreamIoRuntime& runtime);
    // 0x4178e0, 0x417900.
    [[nodiscard]] static std::uint32_t DestroyCeguiIstream(
        std::uint32_t stream_address, std::uint8_t flags,
        ICeguiStreamIoRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyCeguiIstreamWrapper(
        std::uint32_t stream_address, std::uint8_t flags,
        ICeguiStreamIoRuntime& runtime);
    // 0x417c30.
    [[nodiscard]] static std::uint32_t ConstructIstream(
        std::uint32_t stream_address, std::uint32_t streambuf,
        bool add_standard, bool initialize_base,
        ICeguiStreamIoRuntime& runtime);
};

}  // namespace pal4
