#pragma once

#include <cstdint>

namespace pal4 {

class IPuzzleWindowRegistrationRuntime {
public:
    virtual ~IPuzzleWindowRegistrationRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Construct(
        std::uint32_t object, std::uint32_t layout_name) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t RegisterFactory(
        std::uint32_t manager, std::uint32_t name,
        std::uint32_t factory) = 0;
    virtual void InsertOrReplace(std::uint32_t tree,
                                 std::uint32_t key,
                                 std::uint32_t handler) = 0;
    virtual void DestroyHandler(std::uint32_t handler,
                                std::uint32_t flags) = 0;
};

class PuzzleWindowRegistration final {
public:
    // IDA 0x4a0a20/0x4a0a30.
    [[nodiscard]] static std::uint32_t InitializeNameThunk(
        IPuzzleWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeName(
        IPuzzleWindowRegistrationRuntime& runtime);
    // IDA 0x4a0a40.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t layout_name,
        IPuzzleWindowRegistrationRuntime& runtime);
    // IDA 0x4a0aa0/0x4a0ab0.
    [[nodiscard]] static std::uint32_t RegisterFactoryThunk(
        IPuzzleWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterFactory(
        IPuzzleWindowRegistrationRuntime& runtime);
    // IDA 0x4a0ad0/0x4a0ae0.
    [[nodiscard]] static std::uint32_t RegisterHandlerThunk(
        IPuzzleWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterHandler(
        IPuzzleWindowRegistrationRuntime& runtime);
};

}  // namespace pal4
