#pragma once

#include "pal4/native_base_object_memory.h"
#include "pal4/native_gob_name_tree.h"
#include "pal4/pal_game_object_manager.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string_view>

namespace pal4 {

class NativeGobFactoryTree;

// Owns the original 0x24-byte GOB manager and its two embedded trees.
class NativePalGameObjectManagerOwner final
    : private IPalGameObjectManagerRuntime {
public:
    explicit NativePalGameObjectManagerOwner(NativeBaseObjectMemory& memory);
    ~NativePalGameObjectManagerOwner() override;
    NativePalGameObjectManagerOwner(const NativePalGameObjectManagerOwner&) =
        delete;
    NativePalGameObjectManagerOwner& operator=(
        const NativePalGameObjectManagerOwner&) = delete;

    void RegisterOriginalFactories();
    [[nodiscard]] bool InsertObject(std::string_view name,
                                    std::uint32_t object);
    [[nodiscard]] std::uint32_t FindObject(std::string_view name);
    [[nodiscard]] std::uint32_t FactoryForType(std::int32_t type) const;
    [[nodiscard]] std::size_t ObjectCount() const;
    void VisitObjects(const std::function<bool(std::uint32_t)>& visitor);
    std::size_t RenderEnabled(std::uint32_t camera,const std::function<void(std::uint32_t,std::uint32_t)>&);
    // Original 5A7170: virtual+44, deleting destructor mode1, then tree clear.
    // Callbacks must not mutate this manager; retirement is not transactional.
    void CleanupScene(const std::function<void(std::uint32_t)>& release,
                      const std::function<void(std::uint32_t, std::uint32_t)>& destroy);
    [[nodiscard]] std::size_t FactoryCount() const;
    [[nodiscard]] std::uint32_t Address() const noexcept { return object_; }

private:
    [[nodiscard]] std::uint32_t SingletonManager() override;
    [[nodiscard]] std::uint32_t RegisterSingleton(
        std::uint32_t manager, std::string_view name, std::uint32_t id,
        std::uint32_t factory, std::uint32_t destructor) override;
    [[nodiscard]] std::uint32_t Allocate(std::uint32_t size) override;
    [[nodiscard]] std::uint32_t Free(std::uint32_t address) override;
    void InitializeTree(std::uint32_t tree, std::uint32_t node_size) override;
    [[nodiscard]] std::uint32_t DestroyTree(
        std::uint32_t tree, std::uint32_t node_size) override;
    [[nodiscard]] bool InsertFactory(
        std::uint32_t tree, std::int32_t type,
        std::uint32_t factory) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;

    NativeBaseObjectMemory& memory_;
    std::unique_ptr<NativeGobNameTree> objects_;
    std::unique_ptr<NativeGobFactoryTree> factories_;
    std::uint32_t object_ = 0u;
    bool cleaning_ = false;
    unsigned visiting_{};
};

}  // namespace pal4
