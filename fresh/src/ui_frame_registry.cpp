#include "pal4/ui_frame_registry.h"
#include "pal4/ui_red_black_tree_rotation.h"
#include "pal4/ui_tree_pair_data.h"

#include <string_view>

namespace pal4 {
namespace {

void EnsureFactorySingleton(
    std::uint8_t& guard, std::uint32_t& cached,
    IUiFrameRegistryRuntime& runtime) {
    if ((guard & 1u) != 0u) return;
    guard |= 1u;
    cached = runtime.RegisterFactorySingleton(
        runtime.SingletonManager(),
        "fsObjectFactory < template-arg4 >", 1019u,
        0x4c1c10u, 0x4c2210u);
}

}  // namespace

std::uint32_t UiFrameRegistry::Find(
    const std::uint32_t manager, const char* name,
    IUiFrameRegistryRuntime& runtime) {
    const auto begin = runtime.ReadWord(manager + 0x18cu);
    if (begin == 0u) return 0u;
    const auto end = runtime.ReadWord(manager + 0x190u);
    const std::string_view wanted(name);
    for (std::uint32_t slot = begin; slot != end; slot += 4u) {
        const auto frame = runtime.ReadWord(slot);
        if (runtime.FrameName(frame) == wanted) return frame;
    }
    return 0u;
}

std::uint32_t UiFrameRegistry::Register(
    const std::uint32_t manager, const char* name,
    std::uint8_t& factory_guard, std::uint32_t& factory_singleton,
    IUiFrameRegistryRuntime& runtime) {
    const auto existing = Find(manager, name, runtime);
    if (existing != 0u) return existing;

    EnsureFactorySingleton(factory_guard, factory_singleton, runtime);
    const auto node = runtime.FindFactoryNode(factory_singleton, name);
    if (node == runtime.FactoryEndNode(factory_singleton)) return 0u;
    const auto frame = runtime.CreateFrame(node, name);
    if (frame == 0u) return 0u;

    const auto begin = runtime.ReadWord(manager + 0x18cu);
    const auto end = runtime.ReadWord(manager + 0x190u);
    const auto capacity_end = runtime.ReadWord(manager + 0x194u);
    if (capacity_end - end >= 4u) {
        runtime.WriteWord(end, frame);
        runtime.WriteWord(manager + 0x190u, end + 4u);
        return end;
    }

    const auto old_size = begin == 0u ? 0u : (end - begin) / 4u;
    const auto growth = old_size > 1u ? old_size : 1u;
    const auto new_capacity = old_size + growth;
    const auto storage = runtime.Allocate(new_capacity * 4u);
    for (std::uint32_t i = 0; i < old_size; ++i) {
        runtime.WriteWord(storage + i * 4u,
                          runtime.ReadWord(begin + i * 4u));
    }
    runtime.WriteWord(storage + old_size * 4u, frame);
    runtime.Free(begin);
    runtime.WriteWord(manager + 0x194u, storage + new_capacity * 4u);
    runtime.WriteWord(manager + 0x190u, storage + (old_size + 1u) * 4u);
    runtime.WriteWord(manager + 0x18cu, storage);
    return old_size;
}

std::uint32_t UiFrameFactoryTree::InsertRegistration(
    const std::uint32_t key, const std::uint32_t creator,
    std::uint8_t& factory_guard, std::uint32_t& factory_singleton,
    IUiFrameFactoryTreeRuntime& runtime) {
    EnsureFactorySingleton(factory_guard, factory_singleton, runtime);

    const auto tree = factory_singleton + 4u;
    const auto head = runtime.ReadWord(factory_singleton + 8u);
    const auto nil = runtime.SharedNil();
    auto parent = head;
    auto node = runtime.ReadWord(head + 4u);
    bool last_left = true;
    while (node != nil) {
        parent = node;
        last_left = key < runtime.ReadWord(node + 12u);
        node = runtime.ReadWord(node + (last_left ? 0u : 8u));
    }

    if (runtime.ReadByte(factory_singleton + 12u) != 0u)
        return runtime.InsertFactoryNode(tree, nil, parent, key, creator);

    auto candidate = parent;
    if (last_left) {
        if (parent == runtime.ReadWord(head))
            return runtime.InsertFactoryNode(tree, nil, parent,
                                             key, creator);
        candidate = runtime.Predecessor(parent);
    }
    if (runtime.ReadWord(candidate + 12u) < key)
        return runtime.InsertFactoryNode(tree, nil, parent, key, creator);
    return key;
}

std::uint32_t UiFrameFactoryTree::FindNode(
    const std::uint32_t tree, const std::uint32_t output,
    const std::uint32_t key, IUiFrameFactoryTreeRuntime& runtime) {
    const auto head = runtime.ReadWord(tree + 4u);
    const auto nil = runtime.SharedNil();
    auto candidate = head;
    auto node = runtime.ReadWord(head + 4u);
    while (node != nil) {
        if (runtime.ReadWord(node + 12u) >= key) {
            candidate = node;
            node = runtime.ReadWord(node);
        } else {
            node = runtime.ReadWord(node + 8u);
        }
    }
    const auto found = candidate != head &&
                       key >= runtime.ReadWord(candidate + 12u);
    runtime.WriteWord(output, found ? candidate : head);
    return output;
}

std::uint32_t UiFrameFactoryTree::InsertNode(
    const std::uint32_t tree, const std::uint32_t output,
    const std::uint32_t child, const std::uint32_t parent,
    const std::uint32_t value, IUiFrameFactoryTreeRuntime& runtime) {
    const auto node = runtime.Allocate(0x18u);
    const auto nil = runtime.SharedNil();
    runtime.WriteWord(node + 4u, parent);
    runtime.WriteWord(node + 20u, 0u);
    runtime.WriteWord(node, nil);
    runtime.WriteWord(node + 8u, nil);
    (void)UiTreePairData::Copy(node + 12u, value, runtime);

    const auto head = runtime.ReadWord(tree + 4u);
    runtime.WriteWord(tree + 12u, runtime.ReadWord(tree + 12u) + 1u);
    if (parent == head || child != nil ||
        runtime.ReadWord(value) < runtime.ReadWord(parent + 12u)) {
        runtime.WriteWord(parent, node);
        if (parent == head) {
            runtime.WriteWord(head + 4u, node);
            runtime.WriteWord(head + 8u, node);
        } else if (parent == runtime.ReadWord(head)) {
            runtime.WriteWord(head, node);
        }
    } else {
        runtime.WriteWord(parent + 8u, node);
        if (parent == runtime.ReadWord(head + 8u))
            runtime.WriteWord(head + 8u, node);
    }

    struct Links final : IUiRedBlackTreeLinkRuntime {
        explicit Links(IUiFrameFactoryTreeRuntime& source) : source(source) {}
        std::uint32_t ReadWord(std::uint32_t address) override {
            return source.ReadWord(address);
        }
        void WriteWord(std::uint32_t address, std::uint32_t data) override {
            source.WriteWord(address, data);
        }
        std::uint32_t SharedNil() override { return source.SharedNil(); }
        IUiFrameFactoryTreeRuntime& source;
    } links(runtime);

    auto current = node;
    while (current != runtime.ReadWord(head + 4u) &&
           runtime.ReadWord(runtime.ReadWord(current + 4u) + 20u) == 0u) {
        const auto parent_node = runtime.ReadWord(current + 4u);
        const auto grandparent = runtime.ReadWord(parent_node + 4u);
        if (parent_node == runtime.ReadWord(grandparent)) {
            const auto uncle = runtime.ReadWord(grandparent + 8u);
            if (runtime.ReadWord(uncle + 20u) == 0u) {
                runtime.WriteWord(parent_node + 20u, 1u);
                runtime.WriteWord(uncle + 20u, 1u);
                runtime.WriteWord(grandparent + 20u, 0u);
                current = grandparent;
            } else {
                if (current == runtime.ReadWord(parent_node + 8u)) {
                    current = parent_node;
                    (void)UiRedBlackTreeRotation::RotateLeft(tree, current,
                                                               links);
                }
                const auto upper = runtime.ReadWord(current + 4u);
                const auto root = runtime.ReadWord(upper + 4u);
                runtime.WriteWord(upper + 20u, 1u);
                runtime.WriteWord(root + 20u, 0u);
                (void)UiRedBlackTreeRotation::RotateRight(tree, root, links);
            }
        } else {
            const auto uncle = runtime.ReadWord(grandparent);
            if (runtime.ReadWord(uncle + 20u) == 0u) {
                runtime.WriteWord(parent_node + 20u, 1u);
                runtime.WriteWord(uncle + 20u, 1u);
                runtime.WriteWord(grandparent + 20u, 0u);
                current = grandparent;
            } else {
                if (current == runtime.ReadWord(parent_node)) {
                    current = parent_node;
                    (void)UiRedBlackTreeRotation::RotateRight(tree, current,
                                                                links);
                }
                const auto upper = runtime.ReadWord(current + 4u);
                const auto root = runtime.ReadWord(upper + 4u);
                runtime.WriteWord(upper + 20u, 1u);
                runtime.WriteWord(root + 20u, 0u);
                (void)UiRedBlackTreeRotation::RotateLeft(tree, root, links);
            }
        }
    }
    runtime.WriteWord(runtime.ReadWord(head + 4u) + 20u, 1u);
    runtime.WriteWord(output, node);
    return output;
}

std::uint32_t UiFrameFactoryTreeStorage::Construct(
    const std::uint8_t seed, IUiFrameFactoryTreeStorageRuntime& runtime) {
    const auto tree = runtime.Allocate(0x14u);
    if (tree == 0u) return 0u;
    runtime.WriteByte(tree + 4u, seed);
    runtime.WriteByte(tree + 5u, seed);
    runtime.WriteByte(tree + 12u, 0u);

    const auto nil_candidate = runtime.Allocate(0x18u);
    runtime.WriteWord(nil_candidate + 4u, 0u);
    runtime.WriteWord(nil_candidate + 20u, 1u);
    if (runtime.SharedNil() == 0u) {
        runtime.SetSharedNil(nil_candidate);
        runtime.WriteWord(nil_candidate, 0u);
        runtime.WriteWord(nil_candidate + 8u, 0u);
    }
    runtime.SetSharedNilUsers(runtime.SharedNilUsers() + 1u);
    if (runtime.SharedNil() != nil_candidate)
        runtime.Free(nil_candidate);

    const auto head = runtime.Allocate(0x18u);
    runtime.WriteWord(head + 4u, runtime.SharedNil());
    runtime.WriteWord(head + 20u, 0u);
    runtime.WriteWord(tree + 8u, head);
    runtime.WriteWord(tree + 16u, 0u);
    runtime.WriteWord(head, head);
    runtime.WriteWord(head + 8u, head);
    runtime.WriteWord(tree, 0x84444cu);
    return tree;
}

void UiFrameFactoryTreeStorage::Destroy(
    const std::uint32_t tree, IUiFrameFactoryTreeStorageRuntime& runtime) {
    runtime.WriteWord(tree, 0x84444cu);
    const auto embedded_tree = tree + 4u;
    const auto head = runtime.ReadWord(tree + 8u);
    auto first = runtime.ReadWord(head);
    if (runtime.ReadWord(tree + 16u) != 0u) {
        (void)FreeNodes(runtime.ReadWord(head + 4u), runtime);
        runtime.WriteWord(head + 4u, runtime.SharedNil());
        runtime.WriteWord(tree + 16u, 0u);
        runtime.WriteWord(head, head);
        runtime.WriteWord(head + 8u, head);
        runtime.GetFirstElement(embedded_tree);
    } else {
        while (first != head) {
            const auto current = first;
            first = runtime.NextNode(first);
            runtime.DeleteNode(embedded_tree, current);
        }
    }
    runtime.Free(head);
    runtime.WriteWord(tree + 8u, 0u);
    runtime.WriteWord(tree + 16u, 0u);
    const auto users = runtime.SharedNilUsers() - 1u;
    runtime.SetSharedNilUsers(users);
    if (users == 0u) {
        const auto nil = runtime.SharedNil();
        runtime.SetSharedNil(0u);
        if (nil != 0u) runtime.Free(nil);
    }
}

std::uint32_t UiFrameFactoryTreeStorage::FreeNodes(
    std::uint32_t node, IUiFrameFactoryTreeStorageRuntime& runtime) {
    while (node != runtime.SharedNil()) {
        (void)FreeNodes(runtime.ReadWord(node + 8u), runtime);
        const auto left = runtime.ReadWord(node);
        runtime.Free(node);
        node = left;
    }
    return runtime.SharedNil();
}

std::optional<std::uint32_t> UiFrameFactoryTreeStorage::DeleteIfPresent(
    const std::uint32_t object,
    IUiFrameFactoryTreeStorageRuntime& runtime) {
    if (object == 0u) return std::nullopt;
    return runtime.InvokeDeletingDestructor(object, 1u);
}

}  // namespace pal4
