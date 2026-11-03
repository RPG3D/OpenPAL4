#pragma once

#include <cstddef>
#include <cstdint>

namespace pal4 {

class ICollectionUiObjectRuntime;
using CollectionUiObjectCreator = std::uint32_t (*)(
    std::uint32_t name, ICollectionUiObjectRuntime& runtime);

class ICollectionUiObjectRuntime {
public:
    virtual ~ICollectionUiObjectRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::size_t size) = 0;
    [[nodiscard]] virtual std::uint32_t Construct(
        std::uint32_t storage, std::uint32_t name) = 0;
    virtual void Free(std::uint32_t storage) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual const char* CastWindowName() = 0;
    [[nodiscard]] virtual std::uint32_t ProcessFrameEvent(
        std::uint32_t manager, const char* name,
        CollectionUiObjectCreator creator) = 0;
};

class CollectionUiObjectRuntimeStub : public ICollectionUiObjectRuntime {
public:
    [[nodiscard]] std::uint32_t Allocate(std::size_t) override { return 0u; }
    [[nodiscard]] std::uint32_t Construct(std::uint32_t storage,
                                           std::uint32_t) override {
        return storage;
    }
    void Free(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t FrameManager() override { return 0u; }
    [[nodiscard]] const char* CastWindowName() override { return nullptr; }
    [[nodiscard]] std::uint32_t ProcessFrameEvent(
        std::uint32_t, const char*, CollectionUiObjectCreator) override {
        return 0u;
    }
};

class CollectionUiObjectFactory final {
public:
    [[nodiscard]] static std::uint32_t CreateObject(
        std::uint32_t name, ICollectionUiObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterCreateObject(
        ICollectionUiObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterCreateObjectThunk(
        ICollectionUiObjectRuntime& runtime);
};

}  // namespace pal4
