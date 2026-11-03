#pragma once
#include <windows.h>
#include <cstdint>
#include <memory>
#include <string_view>

namespace pal4 {
// Original OIRAMLOOK OLDefaultItem, with all virtual behavior supplied by DLLs.
// Lists borrow it with auto-delete disabled. Detach/reset the list before release.
class NativeCeguiDefaultListItem final {
public:
    NativeCeguiDefaultListItem(HMODULE cegui, HMODULE look, std::uint32_t list,
                               std::string_view utf8);
    ~NativeCeguiDefaultListItem();
    NativeCeguiDefaultListItem(const NativeCeguiDefaultListItem&) = delete;
    NativeCeguiDefaultListItem& operator=(const NativeCeguiDefaultListItem&) = delete;
    std::uint32_t Address() const noexcept;
    void SetId(std::uint32_t);
    // Game row key at +138; distinct from CEGUI's row ID at +134.
    void SetDataKey(std::uint32_t);
    // Original Mission code writes item+1A5 directly; preserve its value.
    void SetMissionAvailability(bool);
    void SetDisabledAppearance(bool);
private:
    HMODULE cegui_, look_;
    std::unique_ptr<std::byte[]> image_;
    bool constructed_{};
};
}
