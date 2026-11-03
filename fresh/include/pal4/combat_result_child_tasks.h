#pragma once

#include <cstdint>

namespace pal4 {

class IFleeCandidateChildDestructionRuntime;

class ICombatResultChildRuntime {
public:
    virtual ~ICombatResultChildRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t object) = 0;
    virtual void ConstructBase(std::uint32_t object) = 0;
    virtual void DestroyBase(std::uint32_t object) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
};

class FendOffImpactChildTask final {
public:
    static constexpr std::uint32_t kSize = 68u;
    static constexpr std::uint32_t kVtable = 0x845d18u;

    // IDA 0x569330: factory target of the 0x845d10 slot.
    [[nodiscard]] static std::uint32_t Create(
        ICombatResultChildRuntime& runtime);
};

class MissMessageChildTask final {
public:
    static constexpr std::uint32_t kSize = 80u;
    static constexpr std::uint32_t kVtable = 0x845a24u;

    // IDA 0x569380: factory target of the 0x845d14 slot.
    [[nodiscard]] static std::uint32_t Create(
        ICombatResultChildRuntime& runtime);
};

class FleeCandidateChildTask final {
public:
    static constexpr std::uint32_t kSize = 84u;
    static constexpr std::uint32_t kVtable = 0x845d3cu;

    // IDA 0x569880: factory target of the 0x845d34 slot.
    [[nodiscard]] static std::uint32_t Create(
        ICombatResultChildRuntime& runtime);
    // IDA 0x569900: release text at +68, then the generic base.
    [[nodiscard]] static std::uint32_t DestroyBase(
        std::uint32_t object,
        IFleeCandidateChildDestructionRuntime& runtime);
};

class IFleeCandidateChildDestructionRuntime {
public:
    virtual ~IFleeCandidateChildDestructionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void FreeText(std::uint32_t header) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyGenericBase(
        std::uint32_t object) = 0;
};

class FleeFinishChildTask final {
public:
    static constexpr std::uint32_t kSize = 68u;
    static constexpr std::uint32_t kVtable = 0x845d58u;

    // IDA 0x569950: factory target of the 0x845d38 slot.
    [[nodiscard]] static std::uint32_t Create(
        ICombatResultChildRuntime& runtime);
};

}  // namespace pal4
