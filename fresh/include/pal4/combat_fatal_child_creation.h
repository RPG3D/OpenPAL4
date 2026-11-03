#pragma once

#include <cstdint>

namespace pal4 {

class IFatalChildCreationRuntime {
public:
    virtual ~IFatalChildCreationRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t object) = 0;
    virtual void ConstructBase(std::uint32_t object) = 0;
    virtual void DestroyBase(std::uint32_t object) = 0;
    virtual void ConstructNameNode(std::uint32_t node) = 0;
    virtual void ResetNameNode(std::uint32_t node) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
};

class FatalEventChildTask final {
public:
    static constexpr std::uint32_t kSize = 72u;
    static constexpr std::uint32_t kVtable = 0x845df4u;
    // IDA 0x56bac0.
    [[nodiscard]] static std::uint32_t Create(
        IFatalChildCreationRuntime& runtime);
};

class FatalFirstFinalChildTask final {
public:
    static constexpr std::uint32_t kSize = 64u;
    static constexpr std::uint32_t kVtable = 0x845e10u;
    // IDA 0x56bb40.
    [[nodiscard]] static std::uint32_t Create(
        IFatalChildCreationRuntime& runtime);
};

class FatalFinishChildTask final {
public:
    static constexpr std::uint32_t kSize = 80u;
    static constexpr std::uint32_t kVtable = 0x845e2cu;
    // IDA 0x56bbb0.
    [[nodiscard]] static std::uint32_t Create(
        IFatalChildCreationRuntime& runtime);
};

class FatalEndChildTask final {
public:
    static constexpr std::uint32_t kSize = 68u;
    static constexpr std::uint32_t kVtable = 0x845e48u;
    // IDA 0x56bc20.
    [[nodiscard]] static std::uint32_t Create(
        IFatalChildCreationRuntime& runtime);
};

class FatalMessageChildTask final {
public:
    static constexpr std::uint32_t kSize = 100u;
    static constexpr std::uint32_t kVtable = 0x845788u;
    // IDA 0x56bc70.
    [[nodiscard]] static std::uint32_t Create(
        IFatalChildCreationRuntime& runtime);
};

}  // namespace pal4
