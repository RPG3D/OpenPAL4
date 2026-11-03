#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "pal4/collection_lifecycle.h"

namespace pal4 {

class ICollectionWorldRuntime {
public:
    virtual ~ICollectionWorldRuntime() = default;
    virtual void PrepareGeometry(CollectionState& state,
                                 std::size_t data_offset) = 0;
    [[nodiscard]] virtual int CompareName(CollectionWorldState& world,
                                          std::string_view name) = 0;
    [[nodiscard]] virtual bool AllocateName(CollectionWorldState& world,
                                             std::size_t length,
                                             std::uint8_t mode) = 0;
    virtual void CopyName(CollectionWorldState& world,
                          std::string_view name) = 0;
    virtual void ProcessScripts() = 0;
    [[nodiscard]] virtual std::uint32_t GameInstance() = 0;
    virtual void InitializeWorld(CollectionState& state,
                                 std::uint32_t object,
                                 std::string_view label,
                                 std::string_view name,
                                 std::uint32_t owner) = 0;
    virtual void SetCamera(CollectionState& state,
                           std::uint32_t camera) = 0;
    virtual void AddLight(CollectionState& state,
                          const std::array<std::uint32_t, 4>& color) = 0;
    [[nodiscard]] virtual std::uint32_t WorldClump(
        CollectionState& state) = 0;
    virtual void AddClump(CollectionState& state,
                          std::uint32_t clump) = 0;
    virtual void ProcessClumpAtomics(CollectionState& state,
                                     std::uint32_t clump,
                                     std::size_t data_offset) = 0;
    [[nodiscard]] virtual int CleanupScripts() = 0;
};

class CollectionWorldRuntimeStub : public ICollectionWorldRuntime {
public:
    void PrepareGeometry(CollectionState&, std::size_t) override {}
    [[nodiscard]] int CompareName(CollectionWorldState&,
                                   std::string_view) override { return 0; }
    [[nodiscard]] bool AllocateName(CollectionWorldState&, std::size_t,
                                     std::uint8_t) override { return false; }
    void CopyName(CollectionWorldState&, std::string_view) override {}
    void ProcessScripts() override {}
    [[nodiscard]] std::uint32_t GameInstance() override { return 0u; }
    void InitializeWorld(CollectionState&, std::uint32_t,
                         std::string_view, std::string_view,
                         std::uint32_t) override {}
    void SetCamera(CollectionState&, std::uint32_t) override {}
    void AddLight(CollectionState&,
                  const std::array<std::uint32_t, 4>&) override {}
    [[nodiscard]] std::uint32_t WorldClump(CollectionState&) override {
        return 0u;
    }
    void AddClump(CollectionState&, std::uint32_t) override {}
    void ProcessClumpAtomics(CollectionState&, std::uint32_t,
                             std::size_t) override {}
    [[nodiscard]] int CleanupScripts() override { return 0; }
};

class CollectionWorldInitializer final {
public:
    [[nodiscard]] static int AddElementWithDetails(
        CollectionState& state, std::uint32_t object,
        std::string_view name, std::uint32_t camera,
        ICollectionWorldRuntime& runtime);
};

}  // namespace pal4
