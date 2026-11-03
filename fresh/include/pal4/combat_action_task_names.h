#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

enum class CombatActionTaskName : std::uint8_t {
    ChangeAction,
    PlayAction,
    PlayAndWaitAnimation,
    WaitActionFinish,
    ProcessMessage,
    DataOperation,
    FireEvent,
    Protect,
    NormalAttack,
    AttackAction,
    CheckJointAttack,
    Defend,
    Flee,
    Dead,
    ShowWounded,
    FendOff,
    CastMagic,
    MagicAction,
    Run,
    TranslateActor,
    Face,
    ChangeToPrepare,
    FadeOff,
    DeadEffect,
    FinalHit,
    Stunt,
    StuntAction,
    UseProperty,
    PropertyAction,
    ThrowProperty,
    ThrowPropertyAction,
    CastPropertyAction,
};

struct CombatActionTaskNameBinding {
    CombatActionTaskName task;
    std::uint32_t buffer;
    std::uint32_t guard;
    std::uint32_t destructor;
    std::string_view text;
    std::uint8_t guard_mask = 1u;
};

class ICombatActionTaskNameRuntime {
public:
    virtual ~ICombatActionTaskNameRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void SetErrorFlag(std::uint32_t buffer,
                              std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReallocateString(
        std::uint32_t buffer, std::uint32_t length) = 0;
    virtual void CopyBytes(std::uint32_t destination,
                           std::string_view text) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual int RegisterAtExit(std::uint32_t callback) = 0;
};

class CombatActionTaskNames final {
public:
    static constexpr std::array<CombatActionTaskNameBinding, 32> kBindings{{
        {CombatActionTaskName::ChangeAction, 0x8f961cu, 0x8f9618u,
         0x556270u, "ChangeActionTask"},
        {CombatActionTaskName::PlayAction, 0x8f9608u, 0x8f9604u,
         0x5563f0u, "PlayActionTask"},
        {CombatActionTaskName::PlayAndWaitAnimation, 0x8f95f4u,
         0x8f95f0u, 0x556620u, "PlayAndWaitAniMsgTask"},
        {CombatActionTaskName::WaitActionFinish, 0x8f95e0u, 0x8f95dcu,
         0x556780u, "WaitActionFinishTask"},
        {CombatActionTaskName::ProcessMessage, 0x8f95ccu, 0x8f95c8u,
         0x5569c0u, "cbProcMsgTask"},
        {CombatActionTaskName::DataOperation, 0x8f9658u, 0x8f9654u,
         0x557440u, "DataOperateTask"},
        {CombatActionTaskName::FireEvent, 0x8f9644u, 0x8f9640u,
         0x5575c0u, "FireEventTask"},
        {CombatActionTaskName::Protect, 0x8f9630u, 0x8f962cu,
         0x557720u, "ProtectTask"},
        {CombatActionTaskName::NormalAttack, 0x8f9690u, 0x8f968cu,
         0x5578d0u, "NormalAttackTask"},
        {CombatActionTaskName::AttackAction, 0x8f967cu, 0x8f9678u,
         0x5580d0u, "AttackActionTask"},
        {CombatActionTaskName::CheckJointAttack, 0x8f9668u, 0x8f9678u,
         0x558230u, "checkJointAtk", 2u},
        {CombatActionTaskName::Defend, 0x8f96f4u, 0x8f96f0u,
         0x558610u, "DefendTask"},
        {CombatActionTaskName::Flee, 0x8f96e0u, 0x8f96dcu,
         0x558790u, "FleeTask"},
        {CombatActionTaskName::Dead, 0x8f96ccu, 0x8f96c8u,
         0x558940u, "DeadTask"},
        {CombatActionTaskName::ShowWounded, 0x8f96b8u, 0x8f96b4u,
         0x558ac0u, "ShowWoundedTask"},
        {CombatActionTaskName::FendOff, 0x8f96a4u, 0x8f96a0u,
         0x558c20u, "FendOffTask"},
        {CombatActionTaskName::CastMagic, 0x8f971cu, 0x8f9718u,
         0x558dd0u, "CastMagicTask"},
        {CombatActionTaskName::MagicAction, 0x8f9708u, 0x8f9704u,
         0x559380u, "MagicActionTask"},
        {CombatActionTaskName::Run, 0x8f97a8u, 0x8f97a4u,
         0x55a450u, "RunTask"},
        {CombatActionTaskName::TranslateActor, 0x8f9794u, 0x8f9790u,
         0x55a740u, "TranslateActorTask"},
        {CombatActionTaskName::Face, 0x8f9780u, 0x8f977cu,
         0x55a8c0u, "FaceTask"},
        {CombatActionTaskName::ChangeToPrepare, 0x8f976cu, 0x8f9768u,
         0x55acf0u, "ChangeToPrepareActionTask"},
        {CombatActionTaskName::FadeOff, 0x8f9758u, 0x8f9754u,
         0x55ae70u, "FadeOffTask"},
        {CombatActionTaskName::DeadEffect, 0x8f9744u, 0x8f9740u,
         0x55b090u, "DeadEffectTask"},
        {CombatActionTaskName::FinalHit, 0x8f9730u, 0x8f972cu,
         0x55b390u, "cbFinalHitTask"},
        {CombatActionTaskName::Stunt, 0x8f97d0u, 0x8f97ccu,
         0x55b920u, "StuntTask"},
        {CombatActionTaskName::StuntAction, 0x8f97bcu, 0x8f97b8u,
         0x55bdb0u, "StuntActionTask"},
        {CombatActionTaskName::UseProperty, 0x8f9834u, 0x8f9830u,
         0x55cf80u, "UsePropertyTask"},
        {CombatActionTaskName::PropertyAction, 0x8f9820u, 0x8f981cu,
         0x55d570u, "PropertyActionTask"},
        {CombatActionTaskName::ThrowProperty, 0x8f980cu, 0x8f9808u,
         0x55dd60u, "ThrowPropertyTask"},
        {CombatActionTaskName::ThrowPropertyAction, 0x8f97f8u,
         0x8f97f4u, 0x55e360u, "ThrowPropertyActionTask"},
        {CombatActionTaskName::CastPropertyAction, 0x8f97e4u,
         0x8f97e0u, 0x55eb40u, "CastPropertyActionTask"},
    }};

    // IDA 0x556170-0x5569c0 and 0x557340-0x55eb40.
    [[nodiscard]] static std::uint8_t Create(
        CombatActionTaskName task, std::uint8_t incoming_header,
        ICombatActionTaskNameRuntime& runtime);
    [[nodiscard]] static int InitializeAndRegister(
        CombatActionTaskName task, std::uint8_t incoming_header,
        ICombatActionTaskNameRuntime& runtime);
    static void Cleanup(CombatActionTaskName task,
                        ICombatActionTaskNameRuntime& runtime);
};

}  // namespace pal4
