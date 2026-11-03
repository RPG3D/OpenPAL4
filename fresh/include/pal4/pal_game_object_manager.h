#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

struct PalGameObjectManagerSingletonState {
    std::uint8_t guard = 0u;
    std::uint32_t instance = 0u;
};

// The original 32-bit STL tree allocator and shared nil nodes are supplied
// here; manager operations retain the observed container order and offsets.
class IPalGameObjectManagerRuntime {
public:
    virtual ~IPalGameObjectManagerRuntime() = default;
    [[nodiscard]] virtual std::uint32_t SingletonManager() = 0;
    [[nodiscard]] virtual std::uint32_t RegisterSingleton(
        std::uint32_t manager, std::string_view name, std::uint32_t id,
        std::uint32_t factory, std::uint32_t destructor) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
    virtual void InitializeTree(std::uint32_t tree, std::uint32_t node_size) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyTree(
        std::uint32_t tree, std::uint32_t node_size) = 0;
    [[nodiscard]] virtual bool InsertFactory(
        std::uint32_t tree, std::int32_t type,
        std::uint32_t factory) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
};

class PalGameObjectManager final {
public:
    static constexpr std::uint32_t kSize = 0x24u;
    static constexpr std::uint32_t kVtable = 0x849314u;
    static constexpr std::uint32_t kObjectTreeNodeSize = 0x24u;
    static constexpr std::uint32_t kDialogTreeNodeSize = 0x18u;

    [[nodiscard]] static std::uint32_t Get(
        PalGameObjectManagerSingletonState& state,
        IPalGameObjectManagerRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create(
        IPalGameObjectManagerRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, IPalGameObjectManagerRuntime& runtime);
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IPalGameObjectManagerRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t object, IPalGameObjectManagerRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupObjectTree(
        std::uint32_t object, IPalGameObjectManagerRuntime& runtime);
    [[nodiscard]] static bool RegisterFactory(
        std::uint32_t object, std::int32_t type, std::uint32_t factory,
        IPalGameObjectManagerRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupFactoryTree(
        std::uint32_t object, IPalGameObjectManagerRuntime& runtime);
};

}  // namespace pal4
