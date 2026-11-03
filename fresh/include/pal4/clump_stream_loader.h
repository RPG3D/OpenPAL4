#pragma once

#include <cstdint>

namespace pal4 {

class IClumpStreamRuntime {
public:
    virtual ~IClumpStreamRuntime() = default;
    virtual void PrepareImage(std::uint32_t image) = 0;
    [[nodiscard]] virtual std::uint32_t CreateContext(
        std::uint32_t type, std::uint32_t mode,
        std::uint32_t source) = 0;
    [[nodiscard]] virtual bool FindChunk(std::uint32_t context,
                                         std::uint32_t chunk,
                                         std::uint32_t version,
                                         std::uint32_t length) = 0;
    [[nodiscard]] virtual std::uint32_t ReadClump(
        std::uint32_t context) = 0;
    virtual void LogLoadedClump(std::uint32_t source,
                                std::uint32_t line) = 0;
    virtual void CloseContext(std::uint32_t context,
                              std::uint32_t output) = 0;
};

class ClumpStreamRuntimeStub final : public IClumpStreamRuntime {
public:
    void PrepareImage(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t CreateContext(
        std::uint32_t, std::uint32_t, std::uint32_t) override { return 0u; }
    [[nodiscard]] bool FindChunk(std::uint32_t, std::uint32_t,
                                 std::uint32_t, std::uint32_t) override {
        return false;
    }
    [[nodiscard]] std::uint32_t ReadClump(std::uint32_t) override {
        return 0u;
    }
    void LogLoadedClump(std::uint32_t, std::uint32_t) override {}
    void CloseContext(std::uint32_t, std::uint32_t) override {}
};

class ClumpStreamLoader final {
public:
    // IDA 0x5fa030.
    [[nodiscard]] static std::uint32_t Load(
        std::uint32_t source, std::uint32_t image,
        IClumpStreamRuntime& runtime);
};

}  // namespace pal4
