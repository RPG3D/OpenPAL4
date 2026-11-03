#pragma once

#include "pal4/crt_noop_atexit_registration.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class IPropertyWindowRegistrationRuntime {
public:
    using Creator = std::uint32_t (*)(
        std::uint32_t, IPropertyWindowRegistrationRuntime&);

    virtual ~IPropertyWindowRegistrationRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Construct(
        std::uint32_t storage, std::uint32_t name) = 0;
    virtual void Free(std::uint32_t storage) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t RegisteredName() = 0;
    [[nodiscard]] virtual std::uint32_t LiteralName() = 0;
    [[nodiscard]] virtual std::uint32_t RegisterFactory(
        std::uint32_t manager, std::uint32_t name, Creator creator) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void InsertOrReplace(std::uint32_t tree,
                                 std::string_view command,
                                 std::uint32_t& callback) = 0;
    virtual void DeleteCallback(std::uint32_t callback,
                                std::uint32_t flags) = 0;
};

class PropertyWindowRegistrationRuntimeStub :
    public IPropertyWindowRegistrationRuntime {
public:
    [[nodiscard]] std::uint32_t Allocate(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t Construct(
        std::uint32_t storage, std::uint32_t) override { return storage; }
    void Free(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t FrameManager() override { return 0u; }
    [[nodiscard]] std::uint32_t RegisteredName() override { return 0u; }
    [[nodiscard]] std::uint32_t LiteralName() override { return 0u; }
    [[nodiscard]] std::uint32_t RegisterFactory(
        std::uint32_t, std::uint32_t, Creator) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    void InsertOrReplace(std::uint32_t, std::string_view,
                         std::uint32_t&) override {}
    void DeleteCallback(std::uint32_t, std::uint32_t) override {}
};

class PropertyWindowRegistration final {
public:
    static constexpr std::uint32_t kNameCacheAddress = 0x8e781cu;
    static constexpr std::uint32_t kCommandResultAddress = 0x8e7818u;
    static constexpr std::uint32_t kCommandCallbackVtable = 0x843494u;

    [[nodiscard]] static int InitializeStatic(
        std::uint8_t& flag, ICrtNoopAtExitRuntime& runtime);
    [[nodiscard]] static const char* RegisterName(
        const char*& destination) noexcept;
    [[nodiscard]] static const char* RegisterNameThunk(
        const char*& destination) noexcept;
    [[nodiscard]] static std::uint32_t CreateObject(
        std::uint32_t name, IPropertyWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterFactory(
        IPropertyWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterFactoryThunk(
        IPropertyWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterShowCommand(
        IPropertyWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterShowCommandThunk(
        IPropertyWindowRegistrationRuntime& runtime);
};

}  // namespace pal4
