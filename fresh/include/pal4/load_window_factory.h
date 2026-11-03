#pragma once

#include <cstddef>
#include <cstdint>

namespace pal4 {

class ILoadWindowFactoryRuntime;
using LoadWindowCreator = std::uint32_t (*)(
    std::uint32_t name, ILoadWindowFactoryRuntime& runtime);

class ILoadWindowFactoryRuntime {
public:
    virtual ~ILoadWindowFactoryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::size_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Construct(
        std::uint32_t storage, std::uint32_t name) = 0;
    virtual void Free(std::uint32_t storage) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t RegisteredName() = 0;
    [[nodiscard]] virtual std::uint32_t RegisterFactory(
        std::uint32_t manager, std::uint32_t name,
        LoadWindowCreator creator) = 0;
};

class LoadWindowFactoryRuntimeStub : public ILoadWindowFactoryRuntime {
public:
    [[nodiscard]] std::uint32_t Allocate(std::size_t) override { return 0u; }
    [[nodiscard]] std::uint32_t Construct(
        std::uint32_t storage, std::uint32_t) override { return storage; }
    void Free(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t FrameManager() override { return 0u; }
    [[nodiscard]] std::uint32_t RegisteredName() override { return 0u; }
    [[nodiscard]] std::uint32_t RegisterFactory(
        std::uint32_t, std::uint32_t,
        LoadWindowCreator) override { return 0u; }
};

class LoadWindowFactory final {
public:
    [[nodiscard]] static std::uint32_t CreateObject(
        std::uint32_t name, ILoadWindowFactoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterCreateObject(
        ILoadWindowFactoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterCreateObjectThunk(
        ILoadWindowFactoryRuntime& runtime);
};

}  // namespace pal4
