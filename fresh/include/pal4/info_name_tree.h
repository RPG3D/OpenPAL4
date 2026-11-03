#pragma once

#include "pal4/npc_info_manager.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

struct InfoNameTreeInsertResult {
    std::uint32_t node = 0;
    bool inserted = false;
};

class InfoNameTree final {
public:
    // The tree argument points at the first word of the 16-byte tree holder.
    [[nodiscard]] static InfoNameTreeInsertResult Insert(
        std::uint32_t tree, std::string_view name, std::uint32_t record,
        std::uint32_t nil, INpcInfoRuntime& runtime);
    [[nodiscard]] static std::uint32_t Find(
        std::uint32_t tree, std::string_view name, std::uint32_t nil,
        INpcInfoRuntime& runtime);
    [[nodiscard]] static std::uint32_t LowerBound(
        std::uint32_t tree, std::string_view name, std::uint32_t nil,
        INpcInfoRuntime& runtime);
    [[nodiscard]] static std::uint32_t Next(
        std::uint32_t node, std::uint32_t head, std::uint32_t nil,
        INpcInfoRuntime& runtime);
    [[nodiscard]] static std::uint32_t Previous(
        std::uint32_t node, std::uint32_t head, std::uint32_t nil,
        INpcInfoRuntime& runtime);
    [[nodiscard]] static std::uint32_t RotateLeft(
        std::uint32_t tree, std::uint32_t node, std::uint32_t nil,
        INpcInfoRuntime& runtime);
    [[nodiscard]] static std::uint32_t RotateRight(
        std::uint32_t tree, std::uint32_t node, std::uint32_t nil,
        INpcInfoRuntime& runtime);
    static void Erase(std::uint32_t tree, std::uint32_t node,
                      std::uint32_t nil, INpcInfoRuntime& runtime);
    static void ClearNodes(std::uint32_t tree, std::uint32_t nil,
                           INpcInfoRuntime& runtime);
    static void CopyEntry(std::uint32_t destination, std::uint32_t source,
                          INpcInfoRuntime& runtime);
};

}  // namespace pal4
