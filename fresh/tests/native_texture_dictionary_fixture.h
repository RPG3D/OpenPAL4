#pragma once

#include "pal4/native_pal_game_iv_texture_storage.h"

#include <windows.h>

#include <cstring>
#include <new>
#include <stdexcept>
#include <string_view>

namespace pal4::test {

// Seed one real native heap node to exercise the production removal adapter.
// This is test setup, not a production cache insertion implementation.
inline void SeedSingleTextureDictionary(NativePalGameIVTextureStorage& game,
                                        std::string_view name,
                                        std::uint32_t dictionary,
                                        std::uint32_t references) {
    auto& tree = const_cast<RwTexDictionaryTreeState&>(game.Dictionary());
    if (tree.count != 0u || dictionary == 0u || references == 0u)
        throw std::invalid_argument("native texture cache fixture precondition");
    auto* node = static_cast<std::uint32_t*>(
        HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 40u));
    if (!node) throw std::bad_alloc();
    auto* text = static_cast<char*>(
        HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, name.size() + 2u));
    if (!text) {
        (void)HeapFree(GetProcessHeap(), 0u, node);
        throw std::bad_alloc();
    }
    std::memcpy(text + 1u, name.data(), name.size());
    const auto address = reinterpret_cast<std::uint32_t>(node);
    auto* head = reinterpret_cast<std::uint32_t*>(tree.head);
    node[0u] = node[2u] = head[1u];  // shared nil
    node[1u] = tree.head;
    node[4u] = reinterpret_cast<std::uint32_t>(text + 1u);
    node[5u] = node[6u] = static_cast<std::uint32_t>(name.size());
    node[7u] = dictionary;
    node[8u] = references;
    node[9u] = 1u;  // black root
    head[0u] = head[1u] = head[2u] = address;
    tree.count = 1u;
}
}  // namespace pal4::test
