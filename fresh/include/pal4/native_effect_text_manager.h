#pragma once
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>

namespace pal4 {
class NativeBaseObjectMemory;
class NativeEventDocumentRuntime;

// Owns a template tree and constructs a native16-byte manager image, either
// internal or caller-owned. Borrowed services/storage must outlive it;
// properties returned by Find are borrowed until Clear/Load.
class NativeEffectTextManager final {
public:
    NativeEffectTextManager(NativeBaseObjectMemory&, NativeEventDocumentRuntime&);
    // Constructs/destructs the manager in caller-owned storage; never frees it.
    NativeEffectTextManager(NativeBaseObjectMemory&, NativeEventDocumentRuntime&,
        std::span<std::uint32_t,4> storage);
    ~NativeEffectTextManager();
    NativeEffectTextManager(const NativeEffectTextManager&) = delete;
    NativeEffectTextManager& operator=(const NativeEffectTextManager&) = delete;
    [[nodiscard]] std::uint32_t Address() const;
    [[nodiscard]] bool Load(std::uint32_t oxn_list);
    [[nodiscard]] bool Add(std::string_view name, std::uint32_t source);
    [[nodiscard]] std::uint32_t Find(std::string_view name);
    // Native cache node (+28 is the document); missing names return the head.
    [[nodiscard]] std::uint32_t FindNode(std::string_view name);
    // Transfer an owned document only on true. Unlike Add, does not clone it.
    [[nodiscard]] bool Adopt(std::string_view name,std::uint32_t document);
    void Clear();
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}  // namespace pal4
