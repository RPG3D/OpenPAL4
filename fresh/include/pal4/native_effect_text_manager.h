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
// Managers in the same native allocation domain share the original tree nil.
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
    // Borrow an initialized native VC6 key without creating a lookup string.
    [[nodiscard]] std::uint32_t FindNodeByKeyRecord(std::uint32_t key);
    // Transfer an owned document only on true. Unlike Add, does not clone it.
    [[nodiscard]] bool Adopt(std::string_view name,std::uint32_t document);
    // Key storage uses the same native allocator; insertion retains a COW
    // reference. The key remains caller-owned and the document transfers on true.
    [[nodiscard]] bool AdoptKeyRecord(std::uint32_t key,std::uint32_t document);
    void Clear();
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}  // namespace pal4
