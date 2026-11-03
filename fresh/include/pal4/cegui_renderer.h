#pragma once

#include <array>
#include <cstdint>
#include <map>

namespace pal4 {

class CeguiRendererState final {
public:
    static constexpr std::uint32_t kObservedBytes = 0x12cu;
    static constexpr std::uint32_t kVtable = 0x841ec8u;
    static constexpr std::uint32_t kScaledVtable = 0x841f40u;
    static constexpr std::uint32_t kVariant3Vtable = 0x841fb0u;
    static constexpr std::uint32_t kVariant4Vtable = 0x842020u;
    static constexpr std::uint32_t kWideVtable = 0x842090u;

    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t offset) const noexcept;
    [[nodiscard]] std::uint32_t ReadDword(std::uint32_t offset) const noexcept;
    [[nodiscard]] float ReadFloat(std::uint32_t offset) const noexcept;
    void WriteByte(std::uint32_t offset, std::uint8_t value) noexcept;
    void WriteDword(std::uint32_t offset, std::uint32_t value) noexcept;
    void WriteFloat(std::uint32_t offset, float value) noexcept;

private:
    std::array<std::uint8_t, kObservedBytes> bytes_{};
};

class ICeguiRendererRuntime {
public:
    virtual ~ICeguiRendererRuntime() = default;
    virtual void ConstructBase(CeguiRendererState& state) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint16_t value) = 0;
    virtual void CopyElement60(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void RangeNoOp(std::uint32_t begin, std::uint32_t end) = 0;
    [[nodiscard]] virtual std::uint32_t EffectCount(
        const CeguiRendererState& state) = 0;
    virtual void InsertElements28(CeguiRendererState& state,
                                  std::uint32_t position,
                                  std::uint32_t count,
                                  const std::array<std::uint8_t, 28>& value) = 0;
    [[nodiscard]] virtual std::uint32_t ElementCount28(
        const CeguiRendererState& state) = 0;
    virtual void MoveElements28(CeguiRendererState& state,
                                std::uint32_t destination,
                                std::uint32_t source_end) = 0;
    [[nodiscard]] virtual std::uint32_t WideSize(
        const CeguiRendererState& state) = 0;
    virtual void InsertWide(CeguiRendererState& state,
                            std::uint32_t position,
                            std::uint32_t count, std::uint16_t fill) = 0;
    virtual void CopyWide(CeguiRendererState& state,
                          std::uint32_t destination,
                          std::uint32_t source_end) = 0;
    virtual void AssignRect(CeguiRendererState& state,
                            std::uint32_t destination_offset,
                            std::uint32_t source_offset) = 0;
    virtual void ProcessCollectionUntilEmpty(
        CeguiRendererState& state) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void RemoveListElement(CeguiRendererState& state,
                                   std::uint32_t node) = 0;
    virtual void DestroyBase(CeguiRendererState& state) = 0;
};

class CeguiRendererStub final : public ICeguiRendererRuntime {
public:
    void ConstructBase(CeguiRendererState& state) override;
    [[nodiscard]] std::uint32_t Allocate(std::uint32_t bytes) override;
    void Free(std::uint32_t address) override;
    void WriteDword(std::uint32_t address, std::uint32_t value) override;
    void WriteWord(std::uint32_t address, std::uint16_t value) override;
    void CopyElement60(std::uint32_t destination,
                       std::uint32_t source) override;
    void RangeNoOp(std::uint32_t begin, std::uint32_t end) override;
    [[nodiscard]] std::uint32_t EffectCount(
        const CeguiRendererState& state) override;
    void InsertElements28(CeguiRendererState& state,
                          std::uint32_t position, std::uint32_t count,
                          const std::array<std::uint8_t, 28>& value) override;
    [[nodiscard]] std::uint32_t ElementCount28(
        const CeguiRendererState& state) override;
    void MoveElements28(CeguiRendererState& state,
                        std::uint32_t destination,
                        std::uint32_t source_end) override;
    [[nodiscard]] std::uint32_t WideSize(
        const CeguiRendererState& state) override;
    void InsertWide(CeguiRendererState& state,
                    std::uint32_t position, std::uint32_t count,
                    std::uint16_t fill) override;
    void CopyWide(CeguiRendererState& state,
                  std::uint32_t destination,
                  std::uint32_t source_end) override;
    void AssignRect(CeguiRendererState& state,
                    std::uint32_t destination_offset,
                    std::uint32_t source_offset) override;
    void ProcessCollectionUntilEmpty(CeguiRendererState& state) override;
    [[nodiscard]] std::uint32_t ReadDword(
        std::uint32_t address) override;
    void RemoveListElement(CeguiRendererState& state,
                           std::uint32_t node) override;
    void DestroyBase(CeguiRendererState& state) override;

private:
    std::uint32_t next_address_ = 0x10000u;
    std::map<std::uint32_t, std::uint32_t> words_;
    std::map<std::uint32_t, std::array<std::uint8_t, 28>> vertex_records_;
};

class CeguiRenderer final {
public:
    [[nodiscard]] static CeguiRendererState& Construct(
        CeguiRendererState& state, std::int32_t config_width,
        std::int32_t config_height, std::uint8_t incoming_stack_byte,
        const std::array<std::uint8_t, 28>& stack_element,
        ICeguiRendererRuntime& runtime);

    static void InitializeBuffers(
        CeguiRendererState& state,
        const std::array<std::uint8_t, 28>& stack_element,
        ICeguiRendererRuntime& runtime);

    [[nodiscard]] static CeguiRendererState& ConstructScaled(
        CeguiRendererState& state, std::int32_t config_width,
        std::int32_t config_height, std::uint8_t incoming_stack_byte,
        const std::array<std::uint8_t, 28>& stack_element,
        ICeguiRendererRuntime& runtime);
    [[nodiscard]] static CeguiRendererState& ConstructVariant3(
        CeguiRendererState& state, std::int32_t config_width,
        std::int32_t config_height, std::uint8_t incoming_stack_byte,
        const std::array<std::uint8_t, 28>& stack_element,
        ICeguiRendererRuntime& runtime);
    [[nodiscard]] static CeguiRendererState& ConstructVariant4(
        CeguiRendererState& state, std::int32_t config_width,
        std::int32_t config_height, std::uint8_t incoming_stack_byte,
        const std::array<std::uint8_t, 28>& stack_element,
        ICeguiRendererRuntime& runtime);
    [[nodiscard]] static CeguiRendererState& ConstructWide(
        CeguiRendererState& state, std::int32_t config_width,
        std::int32_t config_height, std::uint8_t incoming_stack_byte,
        const std::array<std::uint8_t, 28>& stack_element,
        ICeguiRendererRuntime& runtime);
    static void Destroy(CeguiRendererState& state,
                        ICeguiRendererRuntime& runtime);
};

}  // namespace pal4
