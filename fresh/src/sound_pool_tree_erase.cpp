#include "pal4/sound_pool_tree.h"

namespace pal4 {

std::uint32_t SoundPoolTree::Erase(
    const std::uint32_t pool, const std::uint32_t node,
    ISoundPoolTreeEraseRuntime& runtime) {
    const auto read = [&runtime](const std::uint32_t address,
                                 const std::uint32_t offset) {
        return runtime.ReadWord(address, offset);
    };
    const auto write = [&runtime](const std::uint32_t address,
                                  const std::uint32_t offset,
                                  const std::uint32_t value) {
        runtime.WriteWord(address, offset, value);
    };
    const auto nil = runtime.Sentinel();
    const auto head = read(pool, 4u);
    const auto next = Next(node, runtime);

    auto replacement = node;
    auto child = nil;
    if (read(node, 0u) == nil) {
        child = read(node, 8u);
    } else if (read(node, 8u) == nil) {
        child = read(node, 0u);
    } else {
        replacement = read(node, 8u);
        while (read(replacement, 0u) != nil)
            replacement = read(replacement, 0u);
        child = read(replacement, 8u);
    }

    auto removed = replacement;
    if (replacement != node) {
        write(read(node, 0u), 4u, replacement);
        write(replacement, 0u, read(node, 0u));
        if (replacement == read(node, 8u)) {
            write(child, 4u, replacement);
        } else {
            const auto old_parent = read(replacement, 4u);
            write(child, 4u, old_parent);
            write(old_parent, 0u, child);
            write(replacement, 8u, read(node, 8u));
            write(read(node, 8u), 4u, replacement);
        }
        const auto parent = read(node, 4u);
        if (read(head, 4u) == node)
            write(head, 4u, replacement);
        else if (read(parent, 0u) == node)
            write(parent, 0u, replacement);
        else
            write(parent, 8u, replacement);
        write(replacement, 4u, parent);
        const auto color = read(replacement, 24u);
        write(replacement, 24u, read(node, 24u));
        write(node, 24u, color);
        removed = node;
    } else {
        const auto parent = read(node, 4u);
        write(child, 4u, parent);
        if (read(head, 4u) == node)
            write(head, 4u, child);
        else if (read(parent, 0u) == node)
            write(parent, 0u, child);
        else
            write(parent, 8u, child);

        if (read(head, 0u) == node) {
            if (read(node, 8u) == nil) {
                write(head, 0u, parent);
            } else {
                auto minimum = child;
                while (read(minimum, 0u) != nil)
                    minimum = read(minimum, 0u);
                write(head, 0u, minimum);
            }
        }
        if (read(head, 8u) == node) {
            if (read(node, 0u) == nil) {
                write(head, 8u, parent);
            } else {
                auto maximum = child;
                while (read(maximum, 8u) != nil)
                    maximum = read(maximum, 8u);
                write(head, 8u, maximum);
            }
        }
    }

    if (read(removed, 24u) == 1u) {
        while (child != read(head, 4u) && read(child, 24u) == 1u) {
            const auto parent = read(child, 4u);
            if (child == read(parent, 0u)) {
                auto sibling = read(parent, 8u);
                if (read(sibling, 24u) == 0u) {
                    write(sibling, 24u, 1u);
                    write(parent, 24u, 0u);
                    (void)RotateLeft(pool, parent, runtime);
                    sibling = read(read(child, 4u), 8u);
                }
                if (read(read(sibling, 0u), 24u) == 1u &&
                    read(read(sibling, 8u), 24u) == 1u) {
                    write(sibling, 24u, 0u);
                    child = read(child, 4u);
                } else {
                    if (read(read(sibling, 8u), 24u) == 1u) {
                        write(read(sibling, 0u), 24u, 1u);
                        write(sibling, 24u, 0u);
                        (void)RotateRight(pool, sibling, runtime);
                        sibling = read(read(child, 4u), 8u);
                    }
                    const auto p = read(child, 4u);
                    write(sibling, 24u, read(p, 24u));
                    write(p, 24u, 1u);
                    write(read(sibling, 8u), 24u, 1u);
                    (void)RotateLeft(pool, p, runtime);
                    break;
                }
            } else {
                auto sibling = read(parent, 0u);
                if (read(sibling, 24u) == 0u) {
                    write(sibling, 24u, 1u);
                    write(parent, 24u, 0u);
                    (void)RotateRight(pool, parent, runtime);
                    sibling = read(read(child, 4u), 0u);
                }
                if (read(read(sibling, 8u), 24u) == 1u &&
                    read(read(sibling, 0u), 24u) == 1u) {
                    write(sibling, 24u, 0u);
                    child = read(child, 4u);
                } else {
                    if (read(read(sibling, 0u), 24u) == 1u) {
                        write(read(sibling, 8u), 24u, 1u);
                        write(sibling, 24u, 0u);
                        (void)RotateLeft(pool, sibling, runtime);
                        sibling = read(read(child, 4u), 0u);
                    }
                    const auto p = read(child, 4u);
                    write(sibling, 24u, read(p, 24u));
                    write(p, 24u, 1u);
                    write(read(sibling, 0u), 24u, 1u);
                    (void)RotateRight(pool, p, runtime);
                    break;
                }
            }
        }
        write(child, 24u, 1u);
    }
    if (runtime.ReadByte(removed, 16u) != 0u &&
        read(removed, 20u) != 0u)
        runtime.CleanupEntry(removed + 12u);
    runtime.FreeNode(removed);
    write(pool, 12u, read(pool, 12u) - 1u);
    return next;
}

}  // namespace pal4
