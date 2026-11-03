#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <utility>
#include <vector>
namespace pal4 {
struct NativeCombatSystemImage;class NativeCombatMonsterManager;class NativeCombatGameEventHandler;
class INativeSkillSystemServices;
//212-byte game fields plus native maps/vectors. Tree handles are host storage
//identities, not private VC6 tree nodes. Only typed native services consume them.
//Finished reports are immutable; clones own all container/record data.
class NativeCombatReport final {
public:
    ~NativeCombatReport();
    NativeCombatReport(const NativeCombatReport&)=delete;
    NativeCombatReport& operator=(const NativeCombatReport&)=delete;
    std::uint32_t Address() const noexcept;
    std::uint32_t Word(std::size_t offset) const;
    std::uint8_t Byte(std::size_t offset) const;
    std::vector<std::pair<std::uint32_t,std::uint32_t>> Items(bool bonus=false) const;
    std::span<const std::uint32_t> Learned(std::size_t player) const;
    std::unique_ptr<NativeCombatReport> Clone() const;
private:
    friend class NativeCombatReportPreparer;
    friend class NativeCombatRewards;
    friend class NativeCombatBattleEnd;
    friend class NativeCombatEndingWindow;
    friend class NativeCombatEndingPresentation;
    class Impl;std::unique_ptr<Impl> impl_;
    NativeCombatReport();
    void SetResult(std::uint32_t value);
};
//Original571240, reusing actual416 stats, first Monster roster, property52(5),
//SkillSystem RNG and the existing shared report algorithm. Borrowers outlive
//each synchronous Prepare. This does not apply rewards or construct ending UI.
class NativeCombatReportPreparer final {
public:
    NativeCombatReportPreparer(NativeCombatSystemImage&,NativeCombatGameEventHandler&,
        NativeCombatMonsterManager&,INativeSkillSystemServices&);
    ~NativeCombatReportPreparer();
    NativeCombatReportPreparer(const NativeCombatReportPreparer&)=delete;
    NativeCombatReportPreparer& operator=(const NativeCombatReportPreparer&)=delete;
    std::unique_ptr<NativeCombatReport> Prepare();
    void Close();
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
