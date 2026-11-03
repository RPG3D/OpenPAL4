#pragma once

#include "pal4/combat_translator.h"
#include <array>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace pal4 {
struct NativeGameDbTable;
// Exact16-byte subobject at CombatSystem+380. Backend/table fields identify
// existing host database objects; they never describe a private VC6 container.
struct NativeCombatTranslatorImage final {
    std::array<std::uint32_t,4> words{};
};
static_assert(sizeof(NativeCombatTranslatorImage)==16u);

// Owns the CombatTranslation binding with host database/STL storage.
// Returned table/text views live until a successful Load or Reset.
class NativeCombatTranslator final : private ICombatTranslatorRuntime {
public:
    NativeCombatTranslator();
    // Borrowed storage must outlive this owner and remain at a fixed address.
    explicit NativeCombatTranslator(NativeCombatTranslatorImage& storage);
    ~NativeCombatTranslator();
    NativeCombatTranslator(const NativeCombatTranslator&) = delete;
    NativeCombatTranslator& operator=(const NativeCombatTranslator&) = delete;
    [[nodiscard]] bool Load(const std::filesystem::path& game_root,
                            std::string_view member = "combatDb.db");
    void Reset();
    [[nodiscard]] bool Loaded() const noexcept { return image_.words[1] != 0u; }
    [[nodiscard]] std::uint32_t Address() const noexcept;
    [[nodiscard]] const NativeGameDbTable* Table() const noexcept;
    // Exact byte lookup; text remains in the original database encoding.
    [[nodiscard]] std::optional<std::string_view> Find(std::string_view english) const;
    [[nodiscard]] const std::string& LastError() const noexcept { return error_; }

private:
    explicit NativeCombatTranslator(NativeCombatTranslatorImage* storage);
    struct Backend;
    std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    std::uint32_t Allocate(std::uint32_t bytes) override;
    std::uint32_t ConstructBackend(std::uint32_t storage) override;
    std::uint8_t LoadBackend(std::uint32_t backend, std::uint32_t input,
                             std::uint32_t adapter_vtable) override;
    std::uint32_t FindNamedBackend(std::uint32_t backend, std::string_view name) override;
    std::uint32_t ClearBackend(std::uint32_t backend) override;
    void DestroyBackend(std::uint32_t backend) override;
    std::uint32_t Free(std::uint32_t address) override;
    void CheckBackend(std::uint32_t address) const;

    NativeCombatTranslatorImage owned_image_;
    NativeCombatTranslatorImage& image_;
    std::unique_ptr<Backend> backend_;
    std::filesystem::path root_;
    std::string member_, error_;
};
}
