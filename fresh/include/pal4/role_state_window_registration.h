#pragma once

#include "pal4/crt_atexit_stub.h"

#include <cstdint>

namespace pal4 {

class IRoleStateWindowRegistrationRuntime {
public:
    using Creator = std::uint32_t (*)(
        std::uint32_t, IRoleStateWindowRegistrationRuntime&);

    virtual ~IRoleStateWindowRegistrationRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Construct(
        std::uint32_t storage, std::uint32_t name) = 0;
    virtual void Free(std::uint32_t storage) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t ProcessFactory(
        std::uint32_t manager, std::uint32_t name, Creator creator) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void InsertOrReplace(std::uint32_t tree,
                                 std::uint32_t command,
                                 std::uint32_t& callback) = 0;
    virtual void DeleteCallback(std::uint32_t callback,
                                std::uint32_t flags) = 0;
};

class RoleStateWindowRegistration final {
public:
    static constexpr std::uint32_t kNameLiteral = 0x8b00e8u;
    static constexpr std::uint32_t kCommandLiteral = 0x8b00d8u;
    static constexpr std::uint32_t kNameCache = 0x8e7954u;
    static constexpr std::uint32_t kCommandResult = 0x8e7950u;
    static constexpr std::uint32_t kCallbackVtable = 0x843494u;

    [[nodiscard]] static int InitializeGuard(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    [[nodiscard]] static std::uint32_t RegisterName(
        std::uint32_t& name_cache) noexcept;
    [[nodiscard]] static std::uint32_t RegisterNameThunk(
        std::uint32_t& name_cache) noexcept;
    [[nodiscard]] static std::uint32_t CreateObject(
        std::uint32_t name, IRoleStateWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterFactory(
        std::uint32_t registered_name,
        IRoleStateWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterFactoryThunk(
        std::uint32_t registered_name,
        IRoleStateWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterShowCommand(
        IRoleStateWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterShowCommandThunk(
        IRoleStateWindowRegistrationRuntime& runtime);
};

}  // namespace pal4
