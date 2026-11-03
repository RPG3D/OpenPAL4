#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IMagicWindowRegistrationRuntime {
public:
    using Creator = std::uint32_t (*)(std::uint32_t,
                                      IMagicWindowRegistrationRuntime&);
    virtual ~IMagicWindowRegistrationRuntime() = default;
    virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual std::uint32_t Construct(std::uint32_t storage,
                                    std::uint32_t name) = 0;
    virtual void Free(std::uint32_t storage) = 0;
    virtual std::uint32_t FrameManager() = 0;
    virtual std::uint32_t CachedName() = 0;
    virtual std::uint32_t LiteralName() = 0;
    virtual std::uint32_t RegisterFactory(std::uint32_t manager,
                                           std::uint32_t name,
                                           Creator creator) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void InsertOrReplace(std::uint32_t tree,
                                 std::string_view command,
                                 std::uint32_t& callback) = 0;
    virtual void DeleteCallback(std::uint32_t callback,
                                std::uint32_t flags) = 0;
};

class MagicWindowRegistrationRuntimeStub : public IMagicWindowRegistrationRuntime {
public:
    std::uint32_t Allocate(std::uint32_t) override { return 0; }
    std::uint32_t Construct(std::uint32_t storage, std::uint32_t) override { return storage; }
    void Free(std::uint32_t) override {}
    std::uint32_t FrameManager() override { return 0; }
    std::uint32_t CachedName() override { return 0; }
    std::uint32_t LiteralName() override { return 0; }
    std::uint32_t RegisterFactory(std::uint32_t, std::uint32_t, Creator) override { return 0; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    void InsertOrReplace(std::uint32_t, std::string_view, std::uint32_t&) override {}
    void DeleteCallback(std::uint32_t, std::uint32_t) override {}
};

class MagicWindowRegistration final {
public:
    static constexpr std::uint32_t kNameCacheAddress = 0x8e454cu;
    static constexpr std::uint32_t kCommandResultAddress = 0x8e4548u;
    static constexpr std::uint32_t kCommandCallbackVtable = 0x843494u;

    static const char* RegisterName(const char*& destination) noexcept;
    static const char* RegisterNameThunk(const char*& destination) noexcept;
    static std::uint32_t CreateObject(std::uint32_t name,
                                      IMagicWindowRegistrationRuntime& runtime);
    static std::uint32_t RegisterFactory(IMagicWindowRegistrationRuntime& runtime);
    static std::uint32_t RegisterFactoryThunk(IMagicWindowRegistrationRuntime& runtime);
    static std::uint32_t RegisterShowCommand(IMagicWindowRegistrationRuntime& runtime);
    static std::uint32_t RegisterShowCommandThunk(IMagicWindowRegistrationRuntime& runtime);
};

}  // namespace pal4
