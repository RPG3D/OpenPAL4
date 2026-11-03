#pragma once

#include <cstdint>

namespace pal4 {

struct MonsterManagerTreeStep {
    std::uint32_t next{};
    std::uint32_t result{};
};

// IDA 0x4389e0. The returned EAX value differs from the updated cursor.
class MonsterManagerTreeCursor final {
public:
    template <class Reader>
    [[nodiscard]] static MonsterManagerTreeStep Successor(
        std::uint32_t node, std::uint32_t nil, Reader&& read) {
        const auto right = read(node + 8u);
        if (right != nil) {
            auto next = right;
            auto left = read(next);
            while (left != nil) {
                next = left;
                left = read(next);
            }
            return {next, left};
        }

        auto parent = read(node + 4u);
        while (node == read(parent + 8u)) {
            node = parent;
            parent = read(parent + 4u);
        }
        if (read(node + 8u) != parent) node = parent;
        return {node, parent};
    }

    // IDA 0x438e30. The red-header case advances to its rightmost child.
    template <class Reader>
    [[nodiscard]] static MonsterManagerTreeStep Predecessor(
        std::uint32_t node, std::uint32_t nil, Reader&& read,
        std::uint32_t color_offset = 0x20u) {
        const auto parent = read(node + 4u);
        if (read(node + color_offset) == 0u &&
            read(parent + 4u) == node) {
            const auto rightmost = read(node + 8u);
            return {rightmost, rightmost};
        }
        auto previous = read(node);
        if (previous != nil) {
            auto right = read(previous + 8u);
            while (right != nil) {
                previous = right;
                right = read(previous + 8u);
            }
            return {previous, right};
        }
        auto ancestor = parent;
        while (node == read(ancestor)) {
            node = ancestor;
            ancestor = read(ancestor + 4u);
        }
        return {ancestor, ancestor};
    }
};

}  // namespace pal4
