#include "pal4/sound_pool_tree.h"

namespace pal4 {

SoundPoolInsertResult SoundPoolTree::Insert(
    const std::uint32_t pool, const std::uint32_t entry,
    ISoundPoolTreeRuntime& runtime) {
    const auto read = [&runtime](const std::uint32_t address,
                                 const std::uint32_t offset) {
        return runtime.ReadWord(address, offset);
    };
    const auto key = read(entry, 0u);
    const auto head = read(pool, 4u);
    const auto nil = runtime.Sentinel();
    auto parent = head;
    auto child = read(head, 4u);
    bool went_left = true;
    while (child != nil) {
        parent = child;
        went_left = key < read(child, 12u);
        child = read(child, went_left ? 0u : 8u);
    }

    if (runtime.ReadByte(pool, 8u) == 0u) {
        auto previous = parent;
        if (went_left) {
            if (parent == read(head, 0u))
                return {InsertNodeAndRebalance(
                            pool, child, parent, entry, runtime), true};
            previous = Previous(parent, runtime);
        }
        if (read(previous, 12u) >= key)
            return {previous, false};
        return {InsertNodeAndRebalance(
                    pool, child, parent, entry, runtime), true};
    }

    return {InsertNodeAndRebalance(
                pool, child, parent, entry, runtime), true};
}

std::uint32_t SoundPoolTree::InsertNodeAndRebalance(
    const std::uint32_t pool, const std::uint32_t child,
    const std::uint32_t parent, const std::uint32_t entry,
    ISoundPoolTreeRuntime& runtime) {
    const auto read = [&runtime](const std::uint32_t address,
                                 const std::uint32_t offset) {
        return runtime.ReadWord(address, offset);
    };
    const auto write = [&runtime](const std::uint32_t address,
                                  const std::uint32_t offset,
                                  const std::uint32_t value) {
        runtime.WriteWord(address, offset, value);
    };
    const auto key = read(entry, 0u);
    const auto head = read(pool, 4u);
    const auto nil = runtime.Sentinel();

    auto node = runtime.CreateNode(parent);
    write(node, 0u, nil);
    write(node, 8u, nil);
    (void)CopyEntry(node + 12u, entry, runtime);
    write(pool, 12u, read(pool, 12u) + 1u);
    if (parent == head || child != nil || key < read(parent, 12u)) {
        write(parent, 0u, node);
        if (parent == head) {
            write(head, 4u, node);
            write(head, 8u, node);
        } else if (parent == read(head, 0u)) {
            write(head, 0u, node);
        }
    } else {
        write(parent, 8u, node);
        if (parent == read(head, 8u))
            write(head, 8u, node);
    }

    auto current = node;
    while (current != read(head, 4u)) {
        auto p = read(current, 4u);
        if (read(p, 24u) != 0u)
            break;
        auto grand = read(p, 4u);
        if (p == read(grand, 0u)) {
            const auto uncle = read(grand, 8u);
            if (read(uncle, 24u) != 0u) {
                if (current == read(p, 8u)) {
                    current = p;
                    (void)RotateLeft(pool, p, runtime);
                }
                write(read(current, 4u), 24u, 1u);
                grand = read(read(current, 4u), 4u);
                write(grand, 24u, 0u);
                (void)RotateRight(pool, grand, runtime);
            } else {
                write(p, 24u, 1u);
                write(uncle, 24u, 1u);
                write(grand, 24u, 0u);
                current = grand;
            }
        } else {
            const auto uncle = read(grand, 0u);
            if (read(uncle, 24u) != 0u) {
                if (current == read(p, 0u)) {
                    current = p;
                    (void)RotateRight(pool, p, runtime);
                }
                write(read(current, 4u), 24u, 1u);
                grand = read(read(current, 4u), 4u);
                write(grand, 24u, 0u);
                (void)RotateLeft(pool, grand, runtime);
            } else {
                write(p, 24u, 1u);
                write(uncle, 24u, 1u);
                write(grand, 24u, 0u);
                current = grand;
            }
        }
    }
    write(read(head, 4u), 24u, 1u);
    return node;
}

}  // namespace pal4
