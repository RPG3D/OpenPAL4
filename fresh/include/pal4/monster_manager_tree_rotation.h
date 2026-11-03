#pragma once

#include <cstdint>

namespace pal4 {

// The IDA symbols use the opposite names from conventional tree rotations.
class MonsterManagerTreeRotation final {
public:
    // IDA 0x438d50: promotes the right child.
    template <class Memory>
    [[nodiscard]] static std::uint32_t RotateRight(
        std::uint32_t tree, std::uint32_t node, Memory& memory) {
        const auto read = [&](std::uint32_t address) {
            return memory.ReadDword(address);
        };
        const auto write = [&](std::uint32_t address, std::uint32_t value) {
            memory.WriteDword(address, value);
        };
        const auto promoted = read(node + 8u);
        const auto moved = read(promoted);
        write(node + 8u, moved);
        if (moved != memory.SharedNil()) write(moved + 4u, node);
        const auto parent = read(node + 4u);
        write(promoted + 4u, parent);
        const auto header = read(tree + 4u);
        if (node == read(header + 4u)) write(header + 4u, promoted);
        else if (node == read(parent)) write(parent, promoted);
        else write(parent + 8u, promoted);
        write(promoted, node);
        write(node + 4u, promoted);
        return promoted;
    }

    // IDA 0x438db0: promotes the left child.
    template <class Memory>
    [[nodiscard]] static std::uint32_t RotateLeft(
        std::uint32_t tree, std::uint32_t node, Memory& memory) {
        const auto read = [&](std::uint32_t address) {
            return memory.ReadDword(address);
        };
        const auto write = [&](std::uint32_t address, std::uint32_t value) {
            memory.WriteDword(address, value);
        };
        const auto promoted = read(node);
        const auto moved = read(promoted + 8u);
        write(node, moved);
        if (moved != memory.SharedNil()) write(moved + 4u, node);
        const auto parent = read(node + 4u);
        write(promoted + 4u, parent);
        const auto header = read(tree + 4u);
        if (node == read(header + 4u)) write(header + 4u, promoted);
        else if (node == read(parent + 8u)) write(parent + 8u, promoted);
        else write(parent, promoted);
        write(promoted + 8u, node);
        write(node + 4u, promoted);
        return promoted;
    }
};

}  // namespace pal4
