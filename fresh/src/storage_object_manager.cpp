#include "pal4/storage_object_manager.h"

#include "pal4/ui_red_black_tree_rotation.h"

#include <new>

namespace pal4 {
namespace {

std::uint32_t Read(std::uint32_t address, IStorageObjectRuntime& runtime) {
    return runtime.ReadWord(address);
}

void Write(std::uint32_t address, std::uint32_t value,
           IStorageObjectRuntime& runtime) {
    runtime.WriteWord(address, value);
}

void RotateLeft(std::uint32_t tree, std::uint32_t node,
                IStorageObjectRuntime& runtime) {
    (void)UiRedBlackTreeRotation::RotateLeft(tree, node, runtime);
}

void RotateRight(std::uint32_t tree, std::uint32_t node,
                 IStorageObjectRuntime& runtime) {
    (void)UiRedBlackTreeRotation::RotateRight(tree, node, runtime);
}

void RepairInsert(std::uint32_t tree, std::uint32_t node,
                  IStorageObjectRuntime& runtime) {
    const auto head = Read(tree + 4u, runtime);
    while (node != Read(head + 4u, runtime) &&
           Read(Read(node + 4u, runtime) + 20u, runtime) == 0u) {
        const auto parent = Read(node + 4u, runtime);
        const auto grandparent = Read(parent + 4u, runtime);
        if (parent == Read(grandparent, runtime)) {
            const auto uncle = Read(grandparent + 8u, runtime);
            if (Read(uncle + 20u, runtime) == 0u) {
                Write(parent + 20u, 1u, runtime);
                Write(uncle + 20u, 1u, runtime);
                Write(grandparent + 20u, 0u, runtime);
                node = grandparent;
            } else {
                if (node == Read(parent + 8u, runtime)) {
                    node = parent;
                    RotateLeft(tree, node, runtime);
                }
                const auto p = Read(node + 4u, runtime);
                const auto g = Read(p + 4u, runtime);
                Write(p + 20u, 1u, runtime);
                Write(g + 20u, 0u, runtime);
                RotateRight(tree, g, runtime);
            }
        } else {
            const auto uncle = Read(grandparent, runtime);
            if (Read(uncle + 20u, runtime) == 0u) {
                Write(parent + 20u, 1u, runtime);
                Write(uncle + 20u, 1u, runtime);
                Write(grandparent + 20u, 0u, runtime);
                node = grandparent;
            } else {
                if (node == Read(parent, runtime)) {
                    node = parent;
                    RotateRight(tree, node, runtime);
                }
                const auto p = Read(node + 4u, runtime);
                const auto g = Read(p + 4u, runtime);
                Write(p + 20u, 1u, runtime);
                Write(g + 20u, 0u, runtime);
                RotateLeft(tree, g, runtime);
            }
        }
    }
    Write(Read(head + 4u, runtime) + 20u, 1u, runtime);
}

}  // namespace

std::uint32_t StorageObjectManager::Register(
    StorageObjectRegistration& registration, IStorageObjectRuntime& runtime) {
    if ((registration.guard & 1u) == 0u) {
        registration.guard |= 1u;
        const auto manager = runtime.SingletonManager();
        registration.instance = runtime.RegisterSingleton(
            manager, "storageObjectMgr", 1050u, 0x5b4f30u, 0x5b4f90u);
    }
    return registration.instance;
}

std::uint32_t StorageObjectManager::Create(IStorageObjectRuntime& runtime) {
    const auto object = runtime.Allocate(20u);
    if (object == 0u) return 0u;
    const auto stack_byte = runtime.ConstructorStackByte(0u);
    try {
        return Construct(object, stack_byte, stack_byte, runtime);
    } catch (...) {
        runtime.FreeMemory(object);
        throw;
    }
}

void StorageObjectManager::Destroy(std::uint32_t object,
                                   IStorageObjectRuntime& runtime) {
    if (object == 0u) return;
    Destruct(object, runtime);
    runtime.FreeMemory(object);
}

std::uint32_t StorageObjectManager::Construct(
    std::uint32_t object, std::uint8_t byte4, std::uint8_t byte5,
    IStorageObjectRuntime& runtime) {
    runtime.WriteByte(object + 4u, byte4);
    runtime.WriteByte(object + 5u, byte5);
    runtime.WriteByte(object + 12u, 0u);
    const auto candidate = runtime.Allocate(24u);
    if (candidate == 0u) throw std::bad_alloc();
    Write(candidate + 4u, 0u, runtime);
    Write(candidate + 20u, 1u, runtime);
    if (runtime.SharedNil() == 0u) {
        runtime.SetSharedNil(candidate);
        Write(candidate, 0u, runtime);
        Write(candidate + 8u, 0u, runtime);
    } else {
        runtime.FreeMemory(candidate);
    }
    runtime.SetNilUsers(runtime.NilUsers() + 1u);

    const auto head = runtime.Allocate(24u);
    if (head == 0u) {
        const auto users = runtime.NilUsers() - 1u;
        runtime.SetNilUsers(users);
        if (users == 0u) {
            runtime.FreeMemory(runtime.SharedNil());
            runtime.SetSharedNil(0u);
        }
        throw std::bad_alloc();
    }
    Write(head + 4u, runtime.SharedNil(), runtime);
    Write(head + 20u, 0u, runtime);
    Write(object + 8u, head, runtime);
    Write(object + 16u, 0u, runtime);
    Write(head, head, runtime);
    Write(head + 8u, head, runtime);
    Write(object, 0x84941cu, runtime);
    return object;
}

void StorageObjectManager::Destruct(std::uint32_t object,
                                    IStorageObjectRuntime& runtime) {
    Write(object, 0x84941cu, runtime);
    const auto tree = object + 4u;
    const auto head = Read(object + 8u, runtime);
    if (Read(object + 16u, runtime) != 0u) {
        (void)ClearSubtree(Read(head + 4u, runtime), runtime);
        Write(head + 4u, runtime.SharedNil(), runtime);
        Write(object + 16u, 0u, runtime);
        Write(head, head, runtime);
        Write(head + 8u, head, runtime);
    } else {
        auto node = Read(head, runtime);
        while (node != head) {
            (void)Erase(tree, object, node, runtime);
            node = Read(object, runtime);
        }
        Write(object, 0x84941cu, runtime);
    }
    runtime.FreeMemory(head);
    Write(object + 8u, 0u, runtime);
    Write(object + 16u, 0u, runtime);
    const auto users = runtime.NilUsers() - 1u;
    runtime.SetNilUsers(users);
    if (users == 0u) {
        const auto nil = runtime.SharedNil();
        runtime.SetSharedNil(0u);
        if (nil != 0u) runtime.FreeMemory(nil);
    }
}

void StorageObjectManager::DestructAlias(std::uint32_t object,
                                         IStorageObjectRuntime& runtime) {
    Destruct(object, runtime);
}

std::uint32_t StorageObjectManager::DestroyWithFlags(
    std::uint32_t object, std::uint8_t flags,
    IStorageObjectRuntime& runtime) {
    DestructAlias(object, runtime);
    if ((flags & 1u) != 0u) runtime.FreeMemory(object);
    return object;
}

bool StorageObjectManager::Insert(std::uint32_t object, std::int32_t key,
                                  std::uint32_t callback,
                                  IStorageObjectRuntime& runtime) {
    const auto tree = object + 4u;
    const auto head = Read(object + 8u, runtime);
    const auto nil = runtime.SharedNil();
    auto cursor = Read(head + 4u, runtime);
    auto parent = head;
    bool goes_left = true;
    while (cursor != nil) {
        parent = cursor;
        goes_left = key < static_cast<std::int32_t>(Read(cursor + 12u, runtime));
        cursor = Read(cursor + (goes_left ? 0u : 8u), runtime);
    }
    if (runtime.ReadByte(object + 12u) != 0u) {
        (void)InsertAt(tree, 0u, cursor, parent, key, callback, runtime);
        return true;
    }
    auto previous = parent;
    if (goes_left) {
        if (parent == Read(head, runtime)) {
            (void)InsertAt(tree, 0u, cursor, parent, key, callback, runtime);
            return true;
        }
        previous = Predecessor(parent, runtime);
    }
    if (static_cast<std::int32_t>(Read(previous + 12u, runtime)) >= key)
        return false;
    (void)InsertAt(tree, 0u, cursor, parent, key, callback, runtime);
    return true;
}

std::uint32_t StorageObjectManager::FindAndInvoke(
    std::uint32_t object, std::int32_t key, IStorageObjectRuntime& runtime) {
    const auto tree = object + 4u;
    const auto head = Read(object + 8u, runtime);
    const auto node = LowerBound(tree, key, runtime);
    if (node == head || key < static_cast<std::int32_t>(Read(node + 12u, runtime)))
        return 0u;
    return runtime.InvokeCallback(Read(node + 16u, runtime), head);
}

std::uint32_t StorageObjectManager::Erase(
    std::uint32_t tree, std::uint32_t output, std::uint32_t node,
    IStorageObjectRuntime& runtime) {
    return UiRedBlackTreeRemove::RemoveFactory(tree, output, node, runtime);
}

std::uint32_t StorageObjectManager::ClearSubtree(
    std::uint32_t root, IStorageObjectRuntime& runtime) {
    const auto nil = runtime.SharedNil();
    while (root != nil) {
        (void)ClearSubtree(Read(root + 8u, runtime), runtime);
        const auto left = Read(root, runtime);
        runtime.FreeMemory(root);
        root = left;
    }
    return nil;
}

std::uint32_t StorageObjectManager::LowerBound(
    std::uint32_t tree, std::int32_t key, IStorageObjectRuntime& runtime) {
    const auto head = Read(tree + 4u, runtime);
    auto candidate = head;
    auto cursor = Read(head + 4u, runtime);
    const auto nil = runtime.SharedNil();
    while (cursor != nil) {
        if (static_cast<std::int32_t>(Read(cursor + 12u, runtime)) >= key) {
            candidate = cursor;
            cursor = Read(cursor, runtime);
        } else {
            cursor = Read(cursor + 8u, runtime);
        }
    }
    return candidate;
}

std::uint32_t StorageObjectManager::Successor(
    std::uint32_t node, IStorageObjectRuntime& runtime) {
    const auto nil = runtime.SharedNil();
    auto right = Read(node + 8u, runtime);
    if (right != nil) {
        while (Read(right, runtime) != nil) right = Read(right, runtime);
        return right;
    }
    auto parent = Read(node + 4u, runtime);
    while (node == Read(parent + 8u, runtime)) {
        node = parent;
        parent = Read(parent + 4u, runtime);
    }
    return Read(node + 8u, runtime) == parent ? node : parent;
}

std::uint32_t StorageObjectManager::InsertAt(
    std::uint32_t tree, std::uint32_t output, std::uint32_t child,
    std::uint32_t parent, std::int32_t key, std::uint32_t callback,
    IStorageObjectRuntime& runtime) {
    const auto node = runtime.Allocate(24u);
    if (node == 0u) throw std::bad_alloc();
    const auto nil = runtime.SharedNil();
    const auto head = Read(tree + 4u, runtime);
    Write(node + 4u, parent, runtime);
    Write(node + 20u, 0u, runtime);
    Write(node, nil, runtime);
    Write(node + 8u, nil, runtime);
    Write(node + 12u, static_cast<std::uint32_t>(key), runtime);
    Write(node + 16u, callback, runtime);
    Write(tree + 12u, Read(tree + 12u, runtime) + 1u, runtime);
    if (parent == head || child != nil ||
        key < static_cast<std::int32_t>(Read(parent + 12u, runtime))) {
        Write(parent, node, runtime);
        if (parent == head) {
            Write(head + 4u, node, runtime);
            Write(head + 8u, node, runtime);
        } else if (parent == Read(head, runtime)) {
            Write(head, node, runtime);
        }
    } else {
        Write(parent + 8u, node, runtime);
        if (parent == Read(head + 8u, runtime))
            Write(head + 8u, node, runtime);
    }
    RepairInsert(tree, node, runtime);
    if (output != 0u) Write(output, node, runtime);
    return node;
}

std::uint32_t StorageObjectManager::Predecessor(
    std::uint32_t node, IStorageObjectRuntime& runtime) {
    const auto parent = Read(node + 4u, runtime);
    if (Read(node + 20u, runtime) == 0u &&
        Read(parent + 4u, runtime) == node)
        return Read(node + 8u, runtime);
    const auto nil = runtime.SharedNil();
    auto left = Read(node, runtime);
    if (left != nil) {
        while (Read(left + 8u, runtime) != nil)
            left = Read(left + 8u, runtime);
        return left;
    }
    auto ancestor = parent;
    while (node == Read(ancestor, runtime)) {
        node = ancestor;
        ancestor = Read(ancestor + 4u, runtime);
    }
    return ancestor;
}

}  // namespace pal4
