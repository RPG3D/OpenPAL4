#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IMoviePlayWindowRegistrationRuntime {
public:
    using Creator = std::uint32_t (*)(
        std::uint32_t, IMoviePlayWindowRegistrationRuntime&);

    virtual ~IMoviePlayWindowRegistrationRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Construct(
        std::uint32_t storage, std::uint32_t name) = 0;
    virtual void Free(std::uint32_t storage) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t RegisteredName() = 0;
    [[nodiscard]] virtual std::uint32_t RegisterFactory(
        std::uint32_t manager, std::uint32_t name, Creator creator) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void InsertOrReplace(std::uint32_t tree,
                                 std::string_view command,
                                 std::uint32_t& callback) = 0;
    virtual void DeleteCallback(std::uint32_t callback,
                                std::uint32_t flags) = 0;
};

class MoviePlayWindowRegistrationRuntimeStub :
    public IMoviePlayWindowRegistrationRuntime {
public:
    [[nodiscard]] std::uint32_t Allocate(std::uint32_t) override {
        return 0u;
    }
    [[nodiscard]] std::uint32_t Construct(
        std::uint32_t storage, std::uint32_t) override { return storage; }
    void Free(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t FrameManager() override { return 0u; }
    [[nodiscard]] std::uint32_t RegisteredName() override { return 0u; }
    [[nodiscard]] std::uint32_t RegisterFactory(
        std::uint32_t, std::uint32_t, Creator) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    void InsertOrReplace(std::uint32_t, std::string_view,
                         std::uint32_t&) override {}
    void DeleteCallback(std::uint32_t, std::uint32_t) override {}
};

class MoviePlayWindowRegistration final {
public:
    static constexpr std::uint32_t kCommandResultAddress = 0x8e43c8u;
    static constexpr std::uint32_t kCommandCallbackVtable = 0x8435dcu;
    static constexpr std::uint32_t kPlaybackHandler = 0x4498a0u;

    [[nodiscard]] static const char* RegisterName(
        const char*& destination) noexcept;
    [[nodiscard]] static const char* RegisterNameThunk(
        const char*& destination) noexcept;
    [[nodiscard]] static std::uint32_t CreateObject(
        std::uint32_t name, IMoviePlayWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterFactory(
        IMoviePlayWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterFactoryThunk(
        IMoviePlayWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterPlaybackCommand(
        IMoviePlayWindowRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterPlaybackCommandThunk(
        IMoviePlayWindowRegistrationRuntime& runtime);
};

}  // namespace pal4
