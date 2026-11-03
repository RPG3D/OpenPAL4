#pragma once

#include <cstdint>

namespace pal4 {

class MainWindowRegistration final {
public:
    [[nodiscard]] static const char* RegisterName(
        const char*& destination) noexcept;
    [[nodiscard]] static const char* RegisterNameThunk(
        const char*& destination) noexcept;
};

class IMainWindowFactoryRuntime {
public:
    using Creator = std::uint32_t (*)(
        std::uint32_t, IMainWindowFactoryRuntime&);

    virtual ~IMainWindowFactoryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Construct(
        std::uint32_t storage, std::uint32_t name) = 0;
    virtual void Free(std::uint32_t storage) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t RegisteredName() = 0;
    [[nodiscard]] virtual std::uint32_t RegisterFactory(
        std::uint32_t manager, std::uint32_t name,
        Creator creator) = 0;
};

class MainWindowFactoryRuntimeStub : public IMainWindowFactoryRuntime {
public:
    [[nodiscard]] std::uint32_t Allocate(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t Construct(
        std::uint32_t storage, std::uint32_t) override {
        return storage;
    }
    void Free(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t FrameManager() override { return 0u; }
    [[nodiscard]] std::uint32_t RegisteredName() override {
        return 0u;
    }
    [[nodiscard]] std::uint32_t RegisterFactory(
        std::uint32_t, std::uint32_t, Creator) override {
        return 0u;
    }
};

class MainWindowFactory final {
public:
    [[nodiscard]] static std::uint32_t CreateObject(
        std::uint32_t name, IMainWindowFactoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterCreateObject(
        IMainWindowFactoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterCreateObjectThunk(
        IMainWindowFactoryRuntime& runtime);
};

}  // namespace pal4
