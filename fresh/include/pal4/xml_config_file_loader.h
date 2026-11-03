#pragma once

#include <cstdint>

namespace pal4 {

class IXmlConfigFileLoadRuntime {
public:
    virtual ~IXmlConfigFileLoadRuntime() = default;
    virtual void ConstructStream() = 0;
    [[nodiscard]] virtual bool OpenInput(std::uint32_t filename,
                                         std::uint32_t mode) = 0;
    virtual void SetStreamError(std::uint32_t flags) = 0;
    [[nodiscard]] virtual bool StreamFailed() = 0;
    [[nodiscard]] virtual std::int32_t ReadVersionWord() = 0;
    [[nodiscard]] virtual std::uint8_t ReadNodeData(
        std::uint32_t parent, std::uint32_t config) = 0;
    [[nodiscard]] virtual bool CloseInput() = 0;
    virtual void ResetFileBuffer() = 0;
    virtual void DestroyStream() = 0;
};

class XmlConfigFileLoadRuntimeStub final : public IXmlConfigFileLoadRuntime {
public:
    void ConstructStream() override {}
    [[nodiscard]] bool OpenInput(std::uint32_t,
                                 std::uint32_t) override { return false; }
    void SetStreamError(std::uint32_t) override {}
    [[nodiscard]] bool StreamFailed() override { return true; }
    [[nodiscard]] std::int32_t ReadVersionWord() override { return 0; }
    [[nodiscard]] std::uint8_t ReadNodeData(std::uint32_t,
                                             std::uint32_t) override { return 0u; }
    [[nodiscard]] bool CloseInput() override { return false; }
    void ResetFileBuffer() override {}
    void DestroyStream() override {}
};

class XmlConfigFileLoader final {
public:
    // IDA 0x5f7500.
    [[nodiscard]] static std::uint8_t Load(
        std::uint32_t filename, std::uint32_t config,
        IXmlConfigFileLoadRuntime& runtime);
};

}  // namespace pal4
