#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct VasExceptionState final {
    std::array<std::uint32_t, 7u> words{};
};

static_assert(sizeof(VasExceptionState) == 28);

class IVasExceptionRuntime {
public:
    virtual ~IVasExceptionRuntime() = default;
    virtual void CopyBase(VasExceptionState& destination,
                          const VasExceptionState& source) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyTextAddress() = 0;
    [[nodiscard]] virtual std::uint8_t ReferenceByte(
        std::uint32_t buffer) = 0;
    virtual void SetSharedTextFlag(VasExceptionState& destination) = 0;
    virtual void IncrementReferenceByte(std::uint32_t buffer) = 0;
    virtual void DecrementReferenceByte(std::uint32_t buffer) = 0;
    virtual void FreeText(std::uint32_t header) = 0;
    [[nodiscard]] virtual bool AllocateText(
        VasExceptionState& destination, std::uint32_t size,
        std::uint32_t mode) = 0;
    virtual void CopyText(std::uint32_t destination,
                          std::uint32_t source,
                          std::uint32_t size) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void DetachAndReset(VasExceptionState& destination) = 0;
    virtual void SetTextSize(VasExceptionState& destination,
                             std::uint32_t size) = 0;
    virtual void DestroyBase(VasExceptionState& destination) = 0;
};

class VasExceptionRuntimeStub final : public IVasExceptionRuntime {
public:
    void CopyBase(VasExceptionState& destination,
                  const VasExceptionState& source) override {
        for (std::uint32_t index = 0u; index < 3u; ++index) {
            destination.words[index] = source.words[index];
        }
    }
    [[nodiscard]] std::uint32_t EmptyTextAddress() override { return 0u; }
    [[nodiscard]] std::uint8_t ReferenceByte(
        std::uint32_t) override { return 0xffu; }
    void SetSharedTextFlag(VasExceptionState&) override {}
    void IncrementReferenceByte(std::uint32_t) override {}
    void DecrementReferenceByte(std::uint32_t) override {}
    void FreeText(std::uint32_t) override {}
    [[nodiscard]] bool AllocateText(
        VasExceptionState&, std::uint32_t,
        std::uint32_t) override { return false; }
    void CopyText(std::uint32_t, std::uint32_t,
                  std::uint32_t) override {}
    void WriteByte(std::uint32_t, std::uint8_t) override {}
    void DetachAndReset(VasExceptionState&) override {}
    void SetTextSize(VasExceptionState&, std::uint32_t) override {}
    void DestroyBase(VasExceptionState&) override {}
};

class VasException final {
public:
    static constexpr std::uint32_t kVtable = 0x84a048u;

    // IDA 0x600740.
    [[nodiscard]] static VasExceptionState& CopyConstruct(
        VasExceptionState& destination,
        const VasExceptionState& source,
        IVasExceptionRuntime& runtime);
    // IDA 0x6016f0.
    static void Destroy(VasExceptionState& state,
                        IVasExceptionRuntime& runtime);
};

}  // namespace pal4
