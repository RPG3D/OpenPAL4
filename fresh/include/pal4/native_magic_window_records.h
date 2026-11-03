#pragma once
#include "pal4/magic_window_skill_catalog.h"
#include "pal4/magic_window_stunt_image_updater.h"
#include "pal4/magic_window_details.h"
#include <memory>
#include <optional>
#include <span>
namespace pal4 {
class NativeGameDatabase;
// Original152-byte Magic and208-byte Stunt records, with owned legacy text.
// Both schemas come from the existing game-data field registries.
class NativeMagicWindowRecords final {
public:
    explicit NativeMagicWindowRecords(const NativeGameDatabase&);
    ~NativeMagicWindowRecords();
    NativeMagicWindowRecords(const NativeMagicWindowRecords&)=delete;
    NativeMagicWindowRecords& operator=(const NativeMagicWindowRecords&)=delete;
    std::span<const MagicDbSkillRecord> Magic()const noexcept;
    using StuntWords=std::array<std::uint32_t,208u/4u>;
    std::span<const StuntWords> Stunts()const noexcept;
    const MagicDbSkillRecord* FindMagic(std::uint32_t)const noexcept;
    std::optional<MagicDbStuntRecord> FindStunt(std::uint32_t)const;
    std::uint32_t StuntAddress(std::uint32_t)const noexcept;
    std::optional<MagicWindowDetailText> MagicDetails(std::uint32_t)const;
    std::optional<MagicWindowDetailText> StuntDetails(std::uint32_t)const;
    std::string_view Text(std::uint32_t address)const;
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
