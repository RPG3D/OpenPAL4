#pragma once

#include "pal4/mode_string_buffer.h"

#include <cstdint>

namespace pal4 {

class IMonsterManagerFindRuntime {
public:
    virtual ~IMonsterManagerFindRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CStringLength(
        std::uint32_t address) = 0;
    virtual void SetErrorFlag(ModeStringBufferRecord& text,
                              std::uint8_t flag) = 0;
    [[nodiscard]] virtual bool AllocateText(
        ModeStringBufferRecord& text, std::uint32_t length,
        std::uint8_t mode) = 0;
    virtual void CopyBytes(std::uint32_t destination,
                           std::uint32_t source,
                           std::uint32_t length) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void FreeTextHeader(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t SearchNodeAlternative(
        std::uint32_t tree, const ModeStringBufferRecord& name) = 0;
    [[nodiscard]] virtual std::int32_t CompareName(
        const ModeStringBufferRecord& name,
        std::uint32_t node_text) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
};

class MonsterManagerFindRuntimeStub : public IMonsterManagerFindRuntime {
public:
    std::uint32_t CStringLength(std::uint32_t) override;
    void SetErrorFlag(ModeStringBufferRecord&, std::uint8_t) override;
    bool AllocateText(ModeStringBufferRecord&, std::uint32_t,
                      std::uint8_t) override;
    void CopyBytes(std::uint32_t, std::uint32_t,
                   std::uint32_t) override;
    void WriteByte(std::uint32_t, std::uint8_t) override;
    std::uint8_t ReadByte(std::uint32_t) override;
    void FreeTextHeader(std::uint32_t) override;
    std::uint32_t SearchNodeAlternative(
        std::uint32_t, const ModeStringBufferRecord&) override;
    std::int32_t CompareName(
        const ModeStringBufferRecord&,
        std::uint32_t) override;
    std::uint32_t ReadDword(std::uint32_t) override;
};

class MonsterManagerFinder final {
public:
    [[nodiscard]] static std::uint32_t FindByName(
        std::uint32_t manager, std::uint32_t name_address,
        IMonsterManagerFindRuntime& runtime);
};

}  // namespace pal4
