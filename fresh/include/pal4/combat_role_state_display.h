#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
namespace pal4 {
class ICombatRoleStateDisplayRuntime {
public:
    virtual ~ICombatRoleStateDisplayRuntime() = default;
    virtual std::uint32_t ReadWord(std::uint32_t) = 0;
    virtual void WriteWord(std::uint32_t,std::uint32_t) = 0;
    virtual std::uint8_t ReadByte(std::uint32_t) = 0;
    virtual void WriteByte(std::uint32_t,std::uint8_t) = 0;
    virtual std::string ConvertToUtf8(const std::string&) = 0;
    virtual std::uint32_t CleanupTemporaryString() = 0;
    virtual void SetImage(std::uint32_t control, std::uint32_t image) = 0;
    virtual void SetWidth(std::uint32_t control, float width) = 0;
    virtual void SetText(std::uint32_t control,
                         std::string_view text) = 0;
    virtual void SetControlVisible(std::uint32_t control, bool visible) = 0;
    virtual void SetControlEnabled(std::uint32_t control, bool enabled) = 0;
    [[nodiscard]] virtual bool IsCommandOrTargetSelection() = 0;
    [[nodiscard]] virtual std::uint32_t OpenCandidateIterator() = 0;
    [[nodiscard]] virtual std::uint32_t OpenActiveRoleIterator() = 0;
    [[nodiscard]] virtual bool IteratorAtEnd(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t CurrentActor(
        std::uint32_t iterator) = 0;
    virtual void AdvanceIterator(std::uint32_t iterator) = 0;
    virtual void ReleaseIterator(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t ReleaseActiveRoleIterator(
        std::uint32_t iterator) = 0;
    [[nodiscard]] virtual bool ActorCheckState3(std::uint32_t actor) = 0;
    [[nodiscard]] virtual bool ActorIsStateZero(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::int32_t ActorActionCount(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::int32_t ActorStat(
        std::uint32_t actor, std::uint32_t stat) = 0;
    [[nodiscard]] virtual std::int32_t ActorStatMaximum(
        std::uint32_t actor, std::uint32_t stat) = 0;
    [[nodiscard]] virtual std::uint32_t GetIconImage(
        std::uint32_t portrait, std::uint32_t category,
        std::uint32_t variant) = 0;
    virtual void SetBackgroundImage(std::uint32_t control,
                                    std::uint32_t image) = 0;
    [[nodiscard]] virtual std::uint32_t OpenStatusIterator(
        std::uint32_t actor, bool secondary) = 0;
    [[nodiscard]] virtual bool StatusIteratorAtEnd(
        std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::int32_t StatusValue(
        std::uint32_t iterator) = 0;
    virtual void AdvanceStatusIterator(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint8_t ReleaseStatusIterator(
        std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t GetDialogDataFromParam(
        std::uint32_t parameter) = 0;
    [[nodiscard]] virtual std::int32_t PlayerRosterCount() = 0;
    [[nodiscard]] virtual std::uint32_t ActorAtRoleSlot(
        std::uint32_t slot) = 0;
    [[nodiscard]] virtual std::int32_t ActorStatusKind(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::int32_t ActorElement(
        std::uint32_t actor) = 0;
    virtual void SetImageColours(
        std::uint32_t control,
        const std::array<std::uint32_t, 4>& rgba_bits) = 0;
};
class ICombatRoleStateVisibilityRuntime : public ICombatRoleStateDisplayRuntime {
public:
    virtual void SetParent(std::uint32_t,bool,std::uint32_t) = 0;
    virtual std::uint8_t StartColourTimer(std::uint32_t) = 0;
    virtual std::uint8_t StopColourTimer(std::uint32_t) = 0;
};
}
