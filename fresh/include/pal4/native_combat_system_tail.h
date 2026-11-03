#pragma once
#include <cstdint>
#include "pal4/script_combat_config.h"
#include <memory>
#include <span>
#include <string_view>
namespace pal4 {
// Owns the three host strings behind CombatSystem+432..503. The borrowed
// 72-byte range is stable and must outlive this owner. Record pointers are
// read-only native views, never inputs to VC6 reference-count/destructor code.
class NativeCombatSystemTail final : public ICombatConfigurationState {
public:
    explicit NativeCombatSystemTail(std::span<std::uint32_t,18> storage);
    ~NativeCombatSystemTail();
    NativeCombatSystemTail(const NativeCombatSystemTail&)=delete;
    NativeCombatSystemTail& operator=(const NativeCombatSystemTail&)=delete;
    std::uint32_t Address() const;
    std::string_view Text(std::uint32_t record) const;
    void SetText(std::uint32_t record,std::string_view text);
    CombatConfigSnapshot Snapshot()const override;
    std::uint32_t CombatSystem() override;
    std::uint32_t ReadWord(std::uint32_t address) override;
    CombatConfigText ReadTextProperty(std::uint32_t address) override;
    void SetText(CombatConfigTextSlot,std::string_view) override;
    void ApplyConfiguration(const CombatConfigSnapshot&) override;
    std::uint32_t ReleaseText(CombatConfigText& text,std::uint32_t) noexcept override;
    void Reset();
    void Close();
    bool Closed() const;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
