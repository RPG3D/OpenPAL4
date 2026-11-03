#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "pal4/collection_lifecycle.h"
#include "pal4/collection_world_initializer.h"

namespace pal4 {

// Frame updates do not require object creation, lookup or resource loading.
class ICollectionUpdateRuntime {
public:
    virtual ~ICollectionUpdateRuntime() = default;
    [[nodiscard]] virtual int UpdateRenderParameters(CollectionState&,
                                                     float elapsed) = 0;
    virtual void RotateComponent(std::uint32_t object, double elapsed) = 0;
};

class ICollectionObjectRuntime : public ICollectionUpdateRuntime {
public:
    ~ICollectionObjectRuntime() override = default;
    [[nodiscard]] virtual std::uint32_t ActiveCameraWord() = 0;
    [[nodiscard]] virtual std::string_view NameText(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual ICollectionWorldRuntime& WorldRuntime() = 0;
    [[nodiscard]] virtual int UpdateRenderParameters(CollectionState&,
                                                     float elapsed) = 0;
    virtual void RotateComponent(std::uint32_t object, double elapsed) = 0;
    [[nodiscard]] virtual int RenderWorld(CollectionState&, int argument) = 0;
    [[nodiscard]] virtual int SortAtomicGeometries(CollectionState&,
                                                    std::size_t offset) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateGameObject(
        std::size_t size) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructComponent(
        std::uint32_t storage) = 0;
    virtual void LoadComponent(std::uint32_t object, std::uint32_t id,
                               std::uint32_t prefix, std::uint32_t name,
                               std::uint32_t variant, std::uint32_t zero) = 0;
    [[nodiscard]] virtual std::uint32_t WorldMember(CollectionState&) = 0;
    virtual void AddToWorld(std::uint32_t object, std::uint32_t world) = 0;
    virtual void ProcessEvent(CollectionState&, std::uint32_t* vector_end,
                              std::uint32_t mode,
                              std::uint32_t& object) = 0;
    [[nodiscard]] virtual std::uint32_t ObjectClump(
        std::uint32_t object) = 0;
    virtual void ProcessClumpAtomics(std::uint32_t clump,
                                     CollectionState&,
                                     std::size_t offset) = 0;
    virtual void HideHookAtomics(std::uint32_t clump) = 0;
    [[nodiscard]] virtual std::uint32_t ObjectId(
        std::uint32_t object, std::size_t offset) = 0;
    virtual void RemoveFromWorld(std::uint32_t object) = 0;
    virtual void CleanupComponent(std::uint32_t object) = 0;
    virtual void DeleteComponent(std::uint32_t object,
                                 std::uint32_t flags) = 0;
};

class CollectionObjectRuntimeStub : public ICollectionObjectRuntime {
public:
    [[nodiscard]] std::uint32_t ActiveCameraWord() override { return 0u; }
    [[nodiscard]] std::string_view NameText(std::uint32_t) override {
        return {};
    }
    [[nodiscard]] ICollectionWorldRuntime& WorldRuntime() override {
        return world_runtime_;
    }
    [[nodiscard]] int UpdateRenderParameters(CollectionState&,
                                             float) override { return 0; }
    void RotateComponent(std::uint32_t, double) override {}
    [[nodiscard]] int RenderWorld(CollectionState&, int) override { return 0; }
    [[nodiscard]] int SortAtomicGeometries(CollectionState&,
                                            std::size_t) override { return 0; }
    [[nodiscard]] std::uint32_t AllocateGameObject(
        std::size_t) override { return 0u; }
    [[nodiscard]] std::uint32_t ConstructComponent(
        std::uint32_t storage) override { return storage; }
    void LoadComponent(std::uint32_t, std::uint32_t, std::uint32_t,
                       std::uint32_t, std::uint32_t,
                       std::uint32_t) override {}
    [[nodiscard]] std::uint32_t WorldMember(CollectionState&) override {
        return 0u;
    }
    void AddToWorld(std::uint32_t, std::uint32_t) override {}
    void ProcessEvent(CollectionState&, std::uint32_t*, std::uint32_t,
                      std::uint32_t&) override {}
    [[nodiscard]] std::uint32_t ObjectClump(std::uint32_t) override {
        return 0u;
    }
    void ProcessClumpAtomics(std::uint32_t, CollectionState&,
                             std::size_t) override {}
    void HideHookAtomics(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t ObjectId(std::uint32_t,
                                          std::size_t) override { return 0u; }
    void RemoveFromWorld(std::uint32_t) override {}
    void CleanupComponent(std::uint32_t) override {}
    void DeleteComponent(std::uint32_t, std::uint32_t) override {}
private:
    CollectionWorldRuntimeStub world_runtime_{};
};

class CollectionObjectOperations final {
public:
    [[nodiscard]] static int AddElement(
        CollectionState& state, std::uint32_t object, std::uint32_t name,
        ICollectionObjectRuntime& runtime);
    static void RotateGameObjects(CollectionState& state, double elapsed,
                                  ICollectionUpdateRuntime& runtime);
    [[nodiscard]] static int UpdateRenderParameters(
        CollectionState& state, double elapsed,
        ICollectionUpdateRuntime& runtime);
    [[nodiscard]] static int RenderWorldThunk(
        CollectionState& state, int argument,
        ICollectionObjectRuntime& runtime);
    [[nodiscard]] static int GetGameObjectCount(
        CollectionState& state, int argument,
        ICollectionObjectRuntime& runtime);
    [[nodiscard]] static int GetGameObject(
        CollectionState& state, int ignored_argument,
        ICollectionObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateGameObject(
        CollectionState& state, std::uint32_t prefix, std::uint32_t name,
        std::uint32_t variant, ICollectionObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t FindGameObjectById(
        const CollectionState& state, std::uint32_t id,
        ICollectionObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t* RemoveGameObjectById(
        CollectionState& state, std::uint32_t id,
        ICollectionObjectRuntime& runtime);
};

}  // namespace pal4
