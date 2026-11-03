#pragma once

#include "pal4/native_numbered_list_data.h"
#include "pal4/native_numbered_list_item.h"
#include <filesystem>
#include <vector>

namespace pal4 {
class NativeCeguiUiSession;
class DialogManagerState;

// Owns the ten game rows; borrowed frame, DLL, database and UI must outlive it.
// Reset the actual list before releasing its native row wrappers.
class NativeSystemSettingSavePage final {
public:
    NativeSystemSettingSavePage(HMODULE module, NativeCeguiUiSession& ui,
        std::array<std::uint32_t, 127>& frame, const NativeGameDatabase& database,
        const std::filesystem::path& game_root);
    ~NativeSystemSettingSavePage();
    NativeSystemSettingSavePage(const NativeSystemSettingSavePage&) = delete;
    NativeSystemSettingSavePage& operator=(const NativeSystemSettingSavePage&) = delete;
    void Populate(std::int32_t page, bool load_mode, const DialogManagerState& globals);
private:
    class Runtime;
    void Clear() noexcept;
    HMODULE module_;
    NativeCeguiUiSession& ui_;
    std::array<std::uint32_t, 127>& frame_;
    std::filesystem::path game_root_;
    NativeNumberedListData data_;
    std::vector<std::unique_ptr<NativeNumberedListItem>> items_;
};
}  // namespace pal4
