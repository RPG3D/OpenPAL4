#pragma once

#include <cstdint>

namespace pal4 {

class IJigsawWindowRegistrationRuntime {
public:
    virtual ~IJigsawWindowRegistrationRuntime() = default;
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

class JigsawWindowRegistration final {
public:
    // IDA 0x49f730/0x49f740.
    [[nodiscard]] static std::uint32_t InitializeNameThunk(
        IJigsawWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeName(
        IJigsawWindowRegistrationRuntime& runtime);
    // IDA 0x49f750.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t layout_name,
        IJigsawWindowRegistrationRuntime& runtime);
    // IDA 0x49f7b0/0x49f7c0.
    [[nodiscard]] static std::uint32_t RegisterFactoryThunk(
        IJigsawWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterFactory(
        IJigsawWindowRegistrationRuntime& runtime);
    // IDA 0x49f7e0/0x49f7f0.
    [[nodiscard]] static std::uint32_t RegisterHandlerThunk(
        IJigsawWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterHandler(
        IJigsawWindowRegistrationRuntime& runtime);
};

}  // namespace pal4
