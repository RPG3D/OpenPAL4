#pragma once

#include "pal4/ui_red_black_tree_remove.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

struct StorageObjectRegistration {
    std::uint8_t guard = 0;
    std::uint32_t instance = 0;
};

class IStorageObjectRuntime : public IUiRedBlackTreeRemoveRuntime {
public:
    virtual ~IStorageObjectRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) override = 0;
    [[nodiscard]] virtual std::uint32_t NilUsers() = 0;
    virtual void SetSharedNil(std::uint32_t value) = 0;
    virtual void SetNilUsers(std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t SingletonManager() = 0;
    [[nodiscard]] virtual std::uint32_t RegisterSingleton(
        std::uint32_t manager, std::string_view name, std::uint32_t id,
        std::uint32_t factory, std::uint32_t destructor) = 0;
    [[nodiscard]] virtual std::uint8_t ConstructorStackByte(
        std::uint32_t offset) = 0;
    [[nodiscard]] virtual std::uint32_t InvokeCallback(
        std::uint32_t callback, std::uint32_t head) = 0;
};

class StorageObjectManager final {
public:
    [[nodiscard]] static std::uint32_t Register(
        StorageObjectRegistration& registration, IStorageObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create(IStorageObjectRuntime& runtime);
    static void Destroy(std::uint32_t object, IStorageObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, std::uint8_t byte4, std::uint8_t byte5,
        IStorageObjectRuntime& runtime);
    static void Destruct(std::uint32_t object, IStorageObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyWithFlags(
        std::uint32_t object, std::uint8_t flags,
        IStorageObjectRuntime& runtime);
    static void DestructAlias(std::uint32_t object,
                              IStorageObjectRuntime& runtime);
    [[nodiscard]] static bool Insert(std::uint32_t object, std::int32_t key,
                                     std::uint32_t callback,
                                     IStorageObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t FindAndInvoke(
        std::uint32_t object, std::int32_t key,
        IStorageObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t Erase(
        std::uint32_t tree, std::uint32_t output, std::uint32_t node,
        IStorageObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t ClearSubtree(
        std::uint32_t root, IStorageObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t LowerBound(
        std::uint32_t tree, std::int32_t key,
        IStorageObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t Successor(
        std::uint32_t node, IStorageObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t InsertAt(
        std::uint32_t tree, std::uint32_t output, std::uint32_t child,
        std::uint32_t parent, std::int32_t key, std::uint32_t callback,
        IStorageObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t Predecessor(
        std::uint32_t node, IStorageObjectRuntime& runtime);
};

}  // namespace pal4
