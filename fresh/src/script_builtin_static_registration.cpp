#include "pal4/script_builtin_static_registration.h"

#include <array>
#include <cstddef>
#include <stdexcept>

namespace pal4 {
namespace {

struct RegistrationSpec {
    std::uint32_t descriptor;
    std::uint32_t cleanup_callback;
};

constexpr std::array<RegistrationSpec,
                     static_cast<std::size_t>(StaticBuiltin::Count)> kSpecs{{
    {0x8eac30u, 0x486e50u},
    {0x8eabf8u, 0x486e90u},
    {0x8eabc0u, 0x486ed0u},
    {0x8eab88u, 0x486f10u},
    {0x8eab50u, 0x486f50u},
    {0x8eab18u, 0x486f90u},
    {0x8eaae0u, 0x486fd0u},
    {0x8eaaa8u, 0x487010u},
    {0x8eaa70u, 0x487050u},
    {0x8eaa38u, 0x487090u},
    {0x8eaa00u, 0x4870d0u},
    {0x8ea9c8u, 0x487110u},
    {0x8ea990u, 0x487150u},
    {0x8ea958u, 0x487190u},
    {0x8ea920u, 0x4871d0u},
    {0x8ea8e8u, 0x487210u},
    {0x8ea8b0u, 0x487250u},
    {0x8ea878u, 0x487290u},
    {0x8ea840u, 0x4872d0u},
    {0x8ea808u, 0x487310u},
    {0x8ea7d0u, 0x487350u},
    {0x8ea798u, 0x487390u},
    {0x8ea760u, 0x4873d0u},
    {0x8ea728u, 0x487410u},
    {0x8ea6f0u, 0x487450u},
    {0x8ea6b8u, 0x487490u},
    {0x8ea680u, 0x4874d0u},
    {0x8ea648u, 0x487510u},
    {0x8ea610u, 0x487550u},
    {0x8ea5d8u, 0x487590u},
    {0x8ea5a0u, 0x4875d0u},
    {0x8ea568u, 0x487610u},
    {0x8ea530u, 0x487650u},
    {0x8ea4f8u, 0x487690u},
    {0x8ea4c0u, 0x4876d0u},
    {0x8ea488u, 0x487710u},
    {0x8ea450u, 0x487750u},
    {0x8ea418u, 0x487790u},
    {0x8ea3e0u, 0x4877d0u},
    {0x8ea3a8u, 0x487810u},
    {0x8ea370u, 0x487850u},
    {0x8ea338u, 0x487890u},
    {0x8ea300u, 0x4878d0u},
    {0x8ea2c8u, 0x487910u},
    {0x8ea290u, 0x487950u},
    {0x8ea258u, 0x487990u},
    {0x8ea220u, 0x4879d0u},
    {0x8ea1e8u, 0x487a10u},
    {0x8ea1b0u, 0x487a50u},
    {0x8ea178u, 0x487a90u},
    {0x8ea140u, 0x487ad0u},
    {0x8ea108u, 0x487b10u},
    {0x8ea0d0u, 0x487b50u},
    {0x8ea098u, 0x487b90u},
    {0x8ea060u, 0x487bd0u},
    {0x8ea028u, 0x487c10u},
    {0x8e9ff0u, 0x487c50u},
    {0x8e9fb8u, 0x487c90u},
    {0x8e9f80u, 0x487cd0u},
    {0x8e9f48u, 0x487d10u},
    {0x8e9f10u, 0x487d50u},
    {0x8e9ed8u, 0x487d90u},
    {0x8e9ea0u, 0x487dd0u},
    {0x8e9e68u, 0x487e10u},
    {0x8e9e30u, 0x487e50u},
    {0x8e9df8u, 0x487e90u},
    {0x8e9dc0u, 0x487ed0u},
    {0x8e9d88u, 0x487f10u},
    {0x8e9d50u, 0x487f50u},
    {0x8e9d18u, 0x487f90u},
    {0x8e9ce0u, 0x487fd0u},
    {0x8e9ca8u, 0x488010u},
    {0x8e9c70u, 0x488050u},
    {0x8e9c38u, 0x488090u},
    {0x8e9c00u, 0x4880d0u},
    {0x8e9bc8u, 0x488110u},
    {0x8e9b90u, 0x488150u},
    {0x8e9b58u, 0x488190u},
    {0x8e9b20u, 0x4881d0u},
    {0x8e9ae8u, 0x488210u},
    {0x8e9ab0u, 0x488250u},
    {0x8e9a78u, 0x488290u},
    {0x8e9a40u, 0x4882d0u},
    {0x8e9a08u, 0x488310u},
    {0x8e99d0u, 0x488350u},
    {0x8e9998u, 0x488390u},
    {0x8e9960u, 0x4883d0u},
    {0x8e9928u, 0x488410u},
    {0x8e98f0u, 0x488450u},
    {0x8e98b8u, 0x488490u},
    {0x8e9880u, 0x4884d0u},
    {0x8e9848u, 0x488510u},
    {0x8e9810u, 0x488550u},
    {0x8e97d8u, 0x488590u},
    {0x8e97a0u, 0x4885d0u},
    {0x8e9768u, 0x488610u},
    {0x8e9730u, 0x488650u},
    {0x8e96f8u, 0x488690u},
    {0x8e96c0u, 0x4886d0u},
    {0x8e9688u, 0x488710u},
    {0x8e9650u, 0x488750u},
    {0x8e9618u, 0x488790u},
    {0x8e95e0u, 0x4887d0u},
    {0x8e95a8u, 0x488810u},
    {0x8e9570u, 0x488850u},
    {0x8e9538u, 0x488890u},
    {0x8e9500u, 0x4888d0u},
    {0x8e94c8u, 0x488910u},
    {0x8e9490u, 0x488950u},
    {0x8e9458u, 0x488990u},
    {0x8e9420u, 0x4889d0u},
    {0x8e93e8u, 0x488a10u},
    {0x8e93b0u, 0x488a50u},
    {0x8e9378u, 0x488a90u},
    {0x8e9340u, 0x488ad0u},
    {0x8e9308u, 0x488b10u},
    {0x8e92d0u, 0x488b50u},
    {0x8e9298u, 0x488b90u},
    {0x8e9260u, 0x488bd0u},
    {0x8e9228u, 0x488c10u},
    {0x8e91f0u, 0x488c50u},
    {0x8e91b8u, 0x488c90u},
    {0x8e9180u, 0x488cd0u},
    {0x8e9148u, 0x488d10u},
    {0x8e9110u, 0x488d50u},
    {0x8e90d8u, 0x488d90u},
    {0x8e90a0u, 0x488dd0u},
    {0x8e9068u, 0x488e10u},
    {0x8e9030u, 0x488e50u},
    {0x8e8ff8u, 0x488e90u},
    {0x8e8fc0u, 0x488ed0u},
    {0x8e8f88u, 0x488f10u},
    {0x8e8f50u, 0x488f50u},
    {0x8e8f18u, 0x488f90u},
    {0x8e8ee0u, 0x488fd0u},
    {0x8e8ea8u, 0x489010u},
    {0x8e8e70u, 0x489050u},
    {0x8e8e38u, 0x489090u},
    {0x8e8e00u, 0x4890d0u},
    {0x8e8dc8u, 0x489110u},
    {0x8e8d90u, 0x489150u},
    {0x8e8d58u, 0x489190u},
    {0x8e8d20u, 0x4891d0u},
    {0x8e8ce8u, 0x489210u},
    {0x8e8cb0u, 0x489250u},
    {0x8e8c78u, 0x489290u},
    {0x8e8c40u, 0x4892d0u},
    {0x8e8c08u, 0x489310u},
    {0x8e8bd0u, 0x489350u},
    {0x8e8b98u, 0x489390u},
    {0x8e8b60u, 0x4893d0u},
    {0x8e8b28u, 0x489410u},
    {0x8e8af0u, 0x489450u},
    {0x8e8ab8u, 0x489490u},
    {0x8e8a80u, 0x4894d0u},
    {0x8e8a48u, 0x489510u},
    {0x8e8a10u, 0x489550u},
    {0x8e89d8u, 0x489590u},
    {0x8e89a0u, 0x4895d0u},
    {0x8e8968u, 0x489610u},
    {0x8e8930u, 0x489650u},
    {0x8e88f8u, 0x489690u},
    {0x8e88c0u, 0x4896d0u},
    {0x8e8888u, 0x489710u},
    {0x8e8850u, 0x489750u},
    {0x8e8818u, 0x489790u},
    {0x8e87e0u, 0x4897d0u},
    {0x8e87a8u, 0x489810u},
    {0x8e8770u, 0x489850u},
    {0x8e8738u, 0x489890u},
    {0x8e8700u, 0x4898d0u},
    {0x8e86c8u, 0x489910u},
    {0x8e8690u, 0x489950u},
    {0x8e8658u, 0x489990u},
    {0x8e8620u, 0x4899d0u},
    {0x8e85e8u, 0x489a10u},
    {0x8e85b0u, 0x489a50u},
    {0x8e8578u, 0x489a90u},
    {0x8e8540u, 0x489ad0u},
    {0x8e8508u, 0x489b10u},
    {0x8e84d0u, 0x489b50u},
    {0x8e8498u, 0x489b90u},
    {0x8e8460u, 0x489bd0u},
    {0x8e8428u, 0x489c10u},
    {0x8e83f0u, 0x489c50u},
    {0x8e83b8u, 0x489c90u},
    {0x8e8380u, 0x489cd0u},
    {0x8e8348u, 0x489d10u},
    {0x8e8310u, 0x489d50u},
    {0x8e82d8u, 0x489d90u},
    {0x8e82a0u, 0x489dd0u},
    {0x8e8268u, 0x489e10u},
    {0x8e8230u, 0x489e50u},
    {0x8e81f8u, 0x489e90u},
    {0x8e81c0u, 0x489ed0u},
    {0x8e8188u, 0x489f10u},
    {0x8e8150u, 0x489f50u},
    {0x8e8118u, 0x489f90u},
    {0x8e80e0u, 0x489fd0u},
    {0x8e80a8u, 0x48a010u},
    {0x8e8070u, 0x48a050u},
    {0x8e8038u, 0x48a090u},
    {0x8e8000u, 0x48a0d0u},
    {0x8e7fc8u, 0x48a110u},
    {0x8e7f90u, 0x48a150u},
    {0x8e7f58u, 0x48a190u},
    {0x8e7f20u, 0x48a1d0u},
    {0x8e7ee8u, 0x48a210u},
    {0x8e7eb0u, 0x48a250u},
    {0x8e7e78u, 0x48a290u},
    {0x8e7e40u, 0x48a2d0u},
    {0x8e7e08u, 0x48a310u},
    {0x8e7dd0u, 0x48a350u},
    {0x8e7d98u, 0x48a390u},
    {0x8e7d60u, 0x48a3d0u},
    {0x8e7d28u, 0x48a410u},
    {0x8e7cf0u, 0x48a450u},
    {0x8e7cb8u, 0x48a490u},
    {0x8e7c80u, 0x48a4d0u},
    {0x8e7c48u, 0x48a510u},
    {0x8e7c10u, 0x48a550u},
    {0x8e7bd8u, 0x48a590u},
    {0x8e7ba0u, 0x48a5d0u},
    {0x8e7b68u, 0x48a610u},
    {0x8e7b30u, 0x48a650u},
    {0x8e7af8u, 0x48a690u},
    {0x8e7ac0u, 0x48a6d0u},
    {0x8e7a88u, 0x48a710u},
    {0x8e7a50u, 0x48a750u},
    {0x8e7a18u, 0x48a790u},
    {0x8e79e0u, 0x48a7d0u},
    {0x8e79a8u, 0x48a810u},
}};

const RegistrationSpec& Spec(const StaticBuiltin builtin) {
    return kSpecs[static_cast<std::size_t>(builtin)];
}

void Register(const StaticBuiltin builtin,
              IScriptBuiltinStaticRuntime& runtime) {
    runtime.RegisterBuiltin(builtin, Spec(builtin).descriptor);
}

int Initialize(const StaticBuiltin builtin,
               IScriptBuiltinStaticRuntime& runtime) {
    Register(builtin, runtime);
    return runtime.RegisterAtExit(Spec(builtin).cleanup_callback);
}

void Cleanup(const StaticBuiltin builtin,
             IScriptBuiltinStaticRuntime& runtime) {
    runtime.DeallocateDescriptor(Spec(builtin).descriptor);
}

}  // namespace

int ScriptBuiltinStaticRegistration::InitializeImmBegin(
    IScriptBuiltinStaticRuntime& runtime) {
    return Initialize(StaticBuiltin::ImmBegin, runtime);
}
void ScriptBuiltinStaticRegistration::RegisterImmBegin(
    IScriptBuiltinStaticRuntime& runtime) {
    Register(StaticBuiltin::ImmBegin, runtime);
}
void ScriptBuiltinStaticRegistration::CleanupImmBegin(
    IScriptBuiltinStaticRuntime& runtime) {
    Cleanup(StaticBuiltin::ImmBegin, runtime);
}

int ScriptBuiltinStaticRegistration::InitializeImmEnd(
    IScriptBuiltinStaticRuntime& runtime) {
    return Initialize(StaticBuiltin::ImmEnd, runtime);
}
void ScriptBuiltinStaticRegistration::RegisterImmEnd(
    IScriptBuiltinStaticRuntime& runtime) {
    Register(StaticBuiltin::ImmEnd, runtime);
}
void ScriptBuiltinStaticRegistration::CleanupImmEnd(
    IScriptBuiltinStaticRuntime& runtime) {
    Cleanup(StaticBuiltin::ImmEnd, runtime);
}

int ScriptBuiltinStaticRegistration::InitializeNewGame(
    IScriptBuiltinStaticRuntime& runtime) {
    return Initialize(StaticBuiltin::NewGame, runtime);
}
void ScriptBuiltinStaticRegistration::RegisterNewGame(
    IScriptBuiltinStaticRuntime& runtime) {
    Register(StaticBuiltin::NewGame, runtime);
}
void ScriptBuiltinStaticRegistration::CleanupNewGame(
    IScriptBuiltinStaticRuntime& runtime) {
    Cleanup(StaticBuiltin::NewGame, runtime);
}

int ScriptBuiltinStaticRegistration::InitializeCameraYpr(
    IScriptBuiltinStaticRuntime& runtime) {
    return Initialize(StaticBuiltin::CameraYpr, runtime);
}
void ScriptBuiltinStaticRegistration::RegisterCameraYpr(
    IScriptBuiltinStaticRuntime& runtime) {
    Register(StaticBuiltin::CameraYpr, runtime);
}
void ScriptBuiltinStaticRegistration::CleanupCameraYpr(
    IScriptBuiltinStaticRuntime& runtime) {
    Cleanup(StaticBuiltin::CameraYpr, runtime);
}

int ScriptBuiltinStaticRegistration::InitializeCameraCtrlDist(
    IScriptBuiltinStaticRuntime& runtime) {
    return Initialize(StaticBuiltin::CameraCtrlDist, runtime);
}
void ScriptBuiltinStaticRegistration::RegisterCameraCtrlDist(
    IScriptBuiltinStaticRuntime& runtime) {
    Register(StaticBuiltin::CameraCtrlDist, runtime);
}
void ScriptBuiltinStaticRegistration::CleanupCameraCtrlDist(
    IScriptBuiltinStaticRuntime& runtime) {
    Cleanup(StaticBuiltin::CameraCtrlDist, runtime);
}

int ScriptBuiltinStaticRegistration::InitializeCameraCtrlYprd(
    IScriptBuiltinStaticRuntime& runtime) {
    return Initialize(StaticBuiltin::CameraCtrlYprd, runtime);
}
void ScriptBuiltinStaticRegistration::RegisterCameraCtrlYprd(
    IScriptBuiltinStaticRuntime& runtime) {
    Register(StaticBuiltin::CameraCtrlYprd, runtime);
}
void ScriptBuiltinStaticRegistration::CleanupCameraCtrlYprd(
    IScriptBuiltinStaticRuntime& runtime) {
    Cleanup(StaticBuiltin::CameraCtrlYprd, runtime);
}

int ScriptBuiltinStaticRegistration::InitializeCameraGetDist(
    IScriptBuiltinStaticRuntime& runtime) {
    return Initialize(StaticBuiltin::CameraGetDist, runtime);
}
void ScriptBuiltinStaticRegistration::RegisterCameraGetDist(
    IScriptBuiltinStaticRuntime& runtime) {
    Register(StaticBuiltin::CameraGetDist, runtime);
}
void ScriptBuiltinStaticRegistration::CleanupCameraGetDist(
    IScriptBuiltinStaticRuntime& runtime) {
    Cleanup(StaticBuiltin::CameraGetDist, runtime);
}

int ScriptBuiltinStaticRegistration::InitializeCameraGetYaw(
    IScriptBuiltinStaticRuntime& runtime) {
    return Initialize(StaticBuiltin::CameraGetYaw, runtime);
}
void ScriptBuiltinStaticRegistration::RegisterCameraGetYaw(
    IScriptBuiltinStaticRuntime& runtime) {
    Register(StaticBuiltin::CameraGetYaw, runtime);
}
void ScriptBuiltinStaticRegistration::CleanupCameraGetYaw(
    IScriptBuiltinStaticRuntime& runtime) {
    Cleanup(StaticBuiltin::CameraGetYaw, runtime);
}

int ScriptBuiltinStaticRegistration::InitializeCameraGetPitch(
    IScriptBuiltinStaticRuntime& runtime) {
    return Initialize(StaticBuiltin::CameraGetPitch, runtime);
}
void ScriptBuiltinStaticRegistration::RegisterCameraGetPitch(
    IScriptBuiltinStaticRuntime& runtime) {
    Register(StaticBuiltin::CameraGetPitch, runtime);
}
void ScriptBuiltinStaticRegistration::CleanupCameraGetPitch(
    IScriptBuiltinStaticRuntime& runtime) {
    Cleanup(StaticBuiltin::CameraGetPitch, runtime);
}

int ScriptBuiltinStaticRegistration::InitializeCameraGetRoll(
    IScriptBuiltinStaticRuntime& runtime) {
    return Initialize(StaticBuiltin::CameraGetRoll, runtime);
}
void ScriptBuiltinStaticRegistration::RegisterCameraGetRoll(
    IScriptBuiltinStaticRuntime& runtime) {
    Register(StaticBuiltin::CameraGetRoll, runtime);
}
void ScriptBuiltinStaticRegistration::CleanupCameraGetRoll(
    IScriptBuiltinStaticRuntime& runtime) {
    Cleanup(StaticBuiltin::CameraGetRoll, runtime);
}

int ScriptBuiltinStaticRegistration::InitializeArenaLoad(
    IScriptBuiltinStaticRuntime& runtime) {
    return Initialize(StaticBuiltin::ArenaLoad, runtime);
}
void ScriptBuiltinStaticRegistration::RegisterArenaLoad(
    IScriptBuiltinStaticRuntime& runtime) {
    Register(StaticBuiltin::ArenaLoad, runtime);
}
void ScriptBuiltinStaticRegistration::CleanupArenaLoad(
    IScriptBuiltinStaticRuntime& runtime) {
    Cleanup(StaticBuiltin::ArenaLoad, runtime);
}

int ScriptBuiltinStaticRegistration::InitializeArenaReady(
    IScriptBuiltinStaticRuntime& runtime) {
    return Initialize(StaticBuiltin::ArenaReady, runtime);
}
void ScriptBuiltinStaticRegistration::RegisterArenaReady(
    IScriptBuiltinStaticRuntime& runtime) {
    Register(StaticBuiltin::ArenaReady, runtime);
}
void ScriptBuiltinStaticRegistration::CleanupArenaReady(
    IScriptBuiltinStaticRuntime& runtime) {
    Cleanup(StaticBuiltin::ArenaReady, runtime);
}

#define PAL4_STATIC_BUILTIN_METHODS(Name)                                     \
    int ScriptBuiltinStaticRegistration::Initialize##Name(                   \
        IScriptBuiltinStaticRuntime& runtime) {                               \
        return Initialize(StaticBuiltin::Name, runtime);                      \
    }                                                                         \
    void ScriptBuiltinStaticRegistration::Register##Name(                    \
        IScriptBuiltinStaticRuntime& runtime) {                               \
        Register(StaticBuiltin::Name, runtime);                                \
    }                                                                         \
    void ScriptBuiltinStaticRegistration::Cleanup##Name(                     \
        IScriptBuiltinStaticRuntime& runtime) {                               \
        Cleanup(StaticBuiltin::Name, runtime);                                 \
    }

PAL4_STATIC_BUILTIN_METHODS(ArenaReadyRestore)
PAL4_STATIC_BUILTIN_METHODS(ArenaHint)
PAL4_STATIC_BUILTIN_METHODS(ArenaComeFromHere)
PAL4_STATIC_BUILTIN_METHODS(PlayerSetLeader)
PAL4_STATIC_BUILTIN_METHODS(PlayerSetVisible)
PAL4_STATIC_BUILTIN_METHODS(PlayerAttachCollision)
PAL4_STATIC_BUILTIN_METHODS(PlayerDetachCollision)
PAL4_STATIC_BUILTIN_METHODS(PlayerLock)
PAL4_STATIC_BUILTIN_METHODS(PlayerUnlock)
PAL4_STATIC_BUILTIN_METHODS(SetNpcVisible)
PAL4_STATIC_BUILTIN_METHODS(NpcCreate)
PAL4_STATIC_BUILTIN_METHODS(NpcDelete)
PAL4_STATIC_BUILTIN_METHODS(SystemExchange)
PAL4_STATIC_BUILTIN_METHODS(MonsterStopPursuit)
PAL4_STATIC_BUILTIN_METHODS(GlobalVariables)
PAL4_STATIC_BUILTIN_METHODS(MazeVariables)
PAL4_STATIC_BUILTIN_METHODS(MazeVar2)
PAL4_STATIC_BUILTIN_METHODS(MazeVar3)
PAL4_STATIC_BUILTIN_METHODS(MazeVar4)
PAL4_STATIC_BUILTIN_METHODS(MazeVar5)
PAL4_STATIC_BUILTIN_METHODS(MazeVar6)
PAL4_STATIC_BUILTIN_METHODS(MazeVar7)
PAL4_STATIC_BUILTIN_METHODS(MazeVar8)
PAL4_STATIC_BUILTIN_METHODS(MazeVar9)
PAL4_STATIC_BUILTIN_METHODS(MazeVar10)
PAL4_STATIC_BUILTIN_METHODS(VarShu)
PAL4_STATIC_BUILTIN_METHODS(VarXia)
PAL4_STATIC_BUILTIN_METHODS(VarWu)
PAL4_STATIC_BUILTIN_METHODS(VarFeng)
PAL4_STATIC_BUILTIN_METHODS(VarZi)
PAL4_STATIC_BUILTIN_METHODS(VarMo)
PAL4_STATIC_BUILTIN_METHODS(VarMing)
PAL4_STATIC_BUILTIN_METHODS(VarYan)
PAL4_STATIC_BUILTIN_METHODS(VarCha)
PAL4_STATIC_BUILTIN_METHODS(VarStar)
PAL4_STATIC_BUILTIN_METHODS(NpcA)
PAL4_STATIC_BUILTIN_METHODS(NpcB)
PAL4_STATIC_BUILTIN_METHODS(NpcC)
PAL4_STATIC_BUILTIN_METHODS(NpcD)
PAL4_STATIC_BUILTIN_METHODS(NpcE)
PAL4_STATIC_BUILTIN_METHODS(NpcF)
PAL4_STATIC_BUILTIN_METHODS(NpcG)
PAL4_STATIC_BUILTIN_METHODS(VarZhu)
PAL4_STATIC_BUILTIN_METHODS(VarShop031)
PAL4_STATIC_BUILTIN_METHODS(VarShop010)
PAL4_STATIC_BUILTIN_METHODS(VarShop032)
PAL4_STATIC_BUILTIN_METHODS(VarShop021)
PAL4_STATIC_BUILTIN_METHODS(scriptRegisterIntVarShop012)
PAL4_STATIC_BUILTIN_METHODS(scriptRegisterIntVarShop034)
PAL4_STATIC_BUILTIN_METHODS(scriptRegisterIntVarShop037)
PAL4_STATIC_BUILTIN_METHODS(scriptRegisterIntVarShop004)
PAL4_STATIC_BUILTIN_METHODS(scriptRegisterIntVarShop024)
PAL4_STATIC_BUILTIN_METHODS(scriptRegisterIntVarShop025)
PAL4_STATIC_BUILTIN_METHODS(scriptRegisterIntVarShop005)
PAL4_STATIC_BUILTIN_METHODS(scriptRegisterIntVarShop035)
PAL4_STATIC_BUILTIN_METHODS(scriptRegisterIntVarShop026)
PAL4_STATIC_BUILTIN_METHODS(scriptRegisterIntVarShop006)
PAL4_STATIC_BUILTIN_METHODS(scriptRegisterIntVarShop027)
PAL4_STATIC_BUILTIN_METHODS(scriptRegisterIntVarShop007)
PAL4_STATIC_BUILTIN_METHODS(scriptRegisterIntVarShop029)
PAL4_STATIC_BUILTIN_METHODS(scriptRegisterIntVarShop009)
PAL4_STATIC_BUILTIN_METHODS(scriptRegisterIntVarShop030)
PAL4_STATIC_BUILTIN_METHODS(CameraSetMode)
PAL4_STATIC_BUILTIN_METHODS(GetGoodsOpenCondition)
PAL4_STATIC_BUILTIN_METHODS(SetGoodsOpenCondition)
PAL4_STATIC_BUILTIN_METHODS(NpcPauseBeh)
PAL4_STATIC_BUILTIN_METHODS(NpcResumeBeh)
PAL4_STATIC_BUILTIN_METHODS(OpenWeather)
PAL4_STATIC_BUILTIN_METHODS(CloseWeather)
PAL4_STATIC_BUILTIN_METHODS(SetMinimapExpmode)
PAL4_STATIC_BUILTIN_METHODS(GetRandnum)
PAL4_STATIC_BUILTIN_METHODS(SetTempGameState)
PAL4_STATIC_BUILTIN_METHODS(FlushTailYAngle)
PAL4_STATIC_BUILTIN_METHODS(AddCombatMonster)
PAL4_STATIC_BUILTIN_METHODS(ConfigCombatParam)
PAL4_STATIC_BUILTIN_METHODS(ConfigCombatBgm)
PAL4_STATIC_BUILTIN_METHODS(ConfigCombatCamera)
PAL4_STATIC_BUILTIN_METHODS(StartCombat)
PAL4_STATIC_BUILTIN_METHODS(SetObjectVisible)
PAL4_STATIC_BUILTIN_METHODS(AddProperty)
PAL4_STATIC_BUILTIN_METHODS(DelProperty)
PAL4_STATIC_BUILTIN_METHODS(PlayerInTeam)
PAL4_STATIC_BUILTIN_METHODS(PlayerOutTeam)
PAL4_STATIC_BUILTIN_METHODS(GOMTouch)
PAL4_STATIC_BUILTIN_METHODS(CameraSetCollide)
PAL4_STATIC_BUILTIN_METHODS(CameraSeekToPlayer)
PAL4_STATIC_BUILTIN_METHODS(CameraAutoSeek)
PAL4_STATIC_BUILTIN_METHODS(PlayerSetAttr)
PAL4_STATIC_BUILTIN_METHODS(PlayerGetLeader)
PAL4_STATIC_BUILTIN_METHODS(SetPlayerLevel)
PAL4_STATIC_BUILTIN_METHODS(AddPlayerEquip)
PAL4_STATIC_BUILTIN_METHODS(OpenMovieFlag)
PAL4_STATIC_BUILTIN_METHODS(AddQuestComplatePercentage)
PAL4_STATIC_BUILTIN_METHODS(AddEquipment)
PAL4_STATIC_BUILTIN_METHODS(PlayerCurrentSetVisible)
PAL4_STATIC_BUILTIN_METHODS(SetFullHP)
PAL4_STATIC_BUILTIN_METHODS(SetFullMP)
PAL4_STATIC_BUILTIN_METHODS(GetPlayerLevel)
PAL4_STATIC_BUILTIN_METHODS(GotoLogo)
PAL4_STATIC_BUILTIN_METHODS(PlayerUnHoldAct)
PAL4_STATIC_BUILTIN_METHODS(NpcUnHoldAct)
PAL4_STATIC_BUILTIN_METHODS(CameraSetDistOptEnable)
PAL4_STATIC_BUILTIN_METHODS(MonsterSetHide)
PAL4_STATIC_BUILTIN_METHODS(GameObjectSetResearch)
PAL4_STATIC_BUILTIN_METHODS(SetPortrait)
PAL4_STATIC_BUILTIN_METHODS(BGMConfigSetMusic)
PAL4_STATIC_BUILTIN_METHODS(BGMConfigIsInArea)
PAL4_STATIC_BUILTIN_METHODS(ScriptMusicMute)
PAL4_STATIC_BUILTIN_METHODS(ScriptMusicPlay)
PAL4_STATIC_BUILTIN_METHODS(ScriptMusicStop)
PAL4_STATIC_BUILTIN_METHODS(ArenaMusicStop)
PAL4_STATIC_BUILTIN_METHODS(WorldMapSetState)
PAL4_STATIC_BUILTIN_METHODS(GetPuzzleGameResult)
PAL4_STATIC_BUILTIN_METHODS(AlwaysJump)
PAL4_STATIC_BUILTIN_METHODS(Sound2DPlay)
PAL4_STATIC_BUILTIN_METHODS(Sound2DStop)
PAL4_STATIC_BUILTIN_METHODS(Sound2DStopID)
PAL4_STATIC_BUILTIN_METHODS(CGEffPlay)
PAL4_STATIC_BUILTIN_METHODS(CGEffStop)
PAL4_STATIC_BUILTIN_METHODS(EffectPlay)
PAL4_STATIC_BUILTIN_METHODS(EffectPlayWithPlayer)
PAL4_STATIC_BUILTIN_METHODS(EffectPlayWithCurrentPlayer)
PAL4_STATIC_BUILTIN_METHODS(EffectPlayWithNPC)
PAL4_STATIC_BUILTIN_METHODS(EffectPlayWithOBJ)
PAL4_STATIC_BUILTIN_METHODS(EffectStopWithOBJ)
PAL4_STATIC_BUILTIN_METHODS(ShowHint)
PAL4_STATIC_BUILTIN_METHODS(PlayerAddSkill)
PAL4_STATIC_BUILTIN_METHODS(MonsterSetVisible)
PAL4_STATIC_BUILTIN_METHODS(PlayerRandomPosition)
PAL4_STATIC_BUILTIN_METHODS(PlayerCurrentRandomPosition)
PAL4_STATIC_BUILTIN_METHODS(EventVolumeVisible)
PAL4_STATIC_BUILTIN_METHODS(AllPlayerGarb2)
PAL4_STATIC_BUILTIN_METHODS(PlayerGarb2)
PAL4_STATIC_BUILTIN_METHODS(AllPlayerGarb1)
PAL4_STATIC_BUILTIN_METHODS(PlayerGarb1)
PAL4_STATIC_BUILTIN_METHODS(GetVisibleObject)
PAL4_STATIC_BUILTIN_METHODS(GetVisibleMonster)
PAL4_STATIC_BUILTIN_METHODS(CheckPackProperty)
PAL4_STATIC_BUILTIN_METHODS(GrantSystemUi)
PAL4_STATIC_BUILTIN_METHODS(OpenSystemUi)
PAL4_STATIC_BUILTIN_METHODS(GrantSmithSystem)
PAL4_STATIC_BUILTIN_METHODS(GrantMagicSystem)
PAL4_STATIC_BUILTIN_METHODS(CheckMagicMastered)
PAL4_STATIC_BUILTIN_METHODS(SelectDialogAddItem)
PAL4_STATIC_BUILTIN_METHODS(SelectDialogGetLastSelect)
PAL4_STATIC_BUILTIN_METHODS(GOBAttachToPlayerEntry)
PAL4_STATIC_BUILTIN_METHODS(GOBAttachToCurrentPlayer)
PAL4_STATIC_BUILTIN_METHODS(GOBDetachFromPlayerEntry)
PAL4_STATIC_BUILTIN_METHODS(GOBDetachFromCurrentPlayer)
PAL4_STATIC_BUILTIN_METHODS(EffectAttachToPlayer)
PAL4_STATIC_BUILTIN_METHODS(EffectAttachToCurrentPlayer)
PAL4_STATIC_BUILTIN_METHODS(EffectDetachFromPlayer)
PAL4_STATIC_BUILTIN_METHODS(EffectDetachFromCurrentPlayer)
PAL4_STATIC_BUILTIN_METHODS(EffectAttachToNpc)
PAL4_STATIC_BUILTIN_METHODS(EffectDetachFromNpc)
PAL4_STATIC_BUILTIN_METHODS(GOBAttachToNpcEntry)
PAL4_STATIC_BUILTIN_METHODS(GOBDetachFromNPCEntry)
PAL4_STATIC_BUILTIN_METHODS(GOBSetPosition)
PAL4_STATIC_BUILTIN_METHODS(ScriptClearCTXButCurrent)
PAL4_STATIC_BUILTIN_METHODS(AddMoney)
PAL4_STATIC_BUILTIN_METHODS(PayMoney)
PAL4_STATIC_BUILTIN_METHODS(HideGASkillObject)
PAL4_STATIC_BUILTIN_METHODS(ShowSignpost)
PAL4_STATIC_BUILTIN_METHODS(PlayerSetEmotion)
PAL4_STATIC_BUILTIN_METHODS(PlayerResetEmotion)
PAL4_STATIC_BUILTIN_METHODS(PlayerCurrentSetEmotion)
PAL4_STATIC_BUILTIN_METHODS(PlayerCurrentResetEmotion)
PAL4_STATIC_BUILTIN_METHODS(LINGSHALegsInjured)
PAL4_STATIC_BUILTIN_METHODS(LINGSHALegsHealing)
PAL4_STATIC_BUILTIN_METHODS(NpcSetEmotion)
PAL4_STATIC_BUILTIN_METHODS(NpcResetEmotion)
PAL4_STATIC_BUILTIN_METHODS(GetPropertyNumb)
PAL4_STATIC_BUILTIN_METHODS(PlayerCurrentGetPosX)
PAL4_STATIC_BUILTIN_METHODS(PlayerCurrentGetPosY)
PAL4_STATIC_BUILTIN_METHODS(PlayerCurrentGetPosZ)
PAL4_STATIC_BUILTIN_METHODS(ArenaGetName)
PAL4_STATIC_BUILTIN_METHODS(ArenaGetArea)
PAL4_STATIC_BUILTIN_METHODS(ArenaSkillEnable)
PAL4_STATIC_BUILTIN_METHODS(ShowInnDialog)
PAL4_STATIC_BUILTIN_METHODS(GetInnDialogResult)
PAL4_STATIC_BUILTIN_METHODS(PlayerTakeARest)
PAL4_STATIC_BUILTIN_METHODS(IsNightTime)
PAL4_STATIC_BUILTIN_METHODS(PlayerSetPosRot)
PAL4_STATIC_BUILTIN_METHODS(PlayerSetPosRotByName)
PAL4_STATIC_BUILTIN_METHODS(StartUiTimer)
PAL4_STATIC_BUILTIN_METHODS(PlayerForbidenSkill)
PAL4_STATIC_BUILTIN_METHODS(GetMoney)
PAL4_STATIC_BUILTIN_METHODS(SelectDialogSetDefaultSelect)
PAL4_STATIC_BUILTIN_METHODS(ShowQuestDialog)
PAL4_STATIC_BUILTIN_METHODS(GetQuestDialogResult)
PAL4_STATIC_BUILTIN_METHODS(ResetPlayerToJumpStart)
PAL4_STATIC_BUILTIN_METHODS(GOBResetEntry)
PAL4_STATIC_BUILTIN_METHODS(CheckEquipInInventory)
PAL4_STATIC_BUILTIN_METHODS(RemoveEquipment)
PAL4_STATIC_BUILTIN_METHODS(AddPrescription)
PAL4_STATIC_BUILTIN_METHODS(EnableShadow)
PAL4_STATIC_BUILTIN_METHODS(AddRoundTimes)
PAL4_STATIC_BUILTIN_METHODS(TimeScript)
PAL4_STATIC_BUILTIN_METHODS(TimeScriptTerminate)
PAL4_STATIC_BUILTIN_METHODS(ConfigCombatGroundCamera)
PAL4_STATIC_BUILTIN_METHODS(AddPlayerFavor)
PAL4_STATIC_BUILTIN_METHODS(GetPalTestResult)
PAL4_STATIC_BUILTIN_METHODS(SetMinimapLevel)
PAL4_STATIC_BUILTIN_METHODS(PetShow)
PAL4_STATIC_BUILTIN_METHODS(NpcAttachEffect)
PAL4_STATIC_BUILTIN_METHODS(NpcDetachEffect)
PAL4_STATIC_BUILTIN_METHODS(MstAttachEffect)
PAL4_STATIC_BUILTIN_METHODS(MstDetachEffect)
PAL4_STATIC_BUILTIN_METHODS(PlayerHookEffect)
PAL4_STATIC_BUILTIN_METHODS(PlayerDetachEffect)
PAL4_STATIC_BUILTIN_METHODS(CommonDialogGetLastSelect)
PAL4_STATIC_BUILTIN_METHODS(ConfigCombatVipMonster)
PAL4_STATIC_BUILTIN_METHODS(EnableSTS)
PAL4_STATIC_BUILTIN_METHODS(ClearUiTimer)
PAL4_STATIC_BUILTIN_METHODS(PauseUiTimer)
PAL4_STATIC_BUILTIN_METHODS(ResumeUiTimer)
PAL4_STATIC_BUILTIN_METHODS(SetFullRage)
PAL4_STATIC_BUILTIN_METHODS(UiTimerGetSaveData)
PAL4_STATIC_BUILTIN_METHODS(ScriptMusicPause)
PAL4_STATIC_BUILTIN_METHODS(ScriptMusicResume)

#undef PAL4_STATIC_BUILTIN_METHODS

namespace {
constexpr std::array<std::uint32_t,
                     static_cast<std::size_t>(StaticBuiltin::Count)>
    kActorManagerNativeEntries{{
#include "pal4/detail/actor_manager_script_builtin_targets.inc"
}};
static_assert(kActorManagerNativeEntries.size() == 232u);
}  // namespace

ActorManagerScriptBuiltinSpec ActorManagerScriptBuiltinRegistration::SpecFor(
    const StaticBuiltin builtin) {
    const auto index = static_cast<std::size_t>(builtin);
    if (index >= kActorManagerNativeEntries.size())
        throw std::out_of_range("actor-manager script built-in index");
    const auto initializer = 0x533f40u + static_cast<std::uint32_t>(index) * 0x40u;
    return {0x8f9468u - static_cast<std::uint32_t>(index) * 0x38u,
            kActorManagerNativeEntries[index], initializer,
            initializer + 0x10u, initializer + 0x30u};
}

int ActorManagerScriptBuiltinRegistration::Initialize(
    const StaticBuiltin builtin, IScriptBuiltinStaticRuntime& runtime) {
    Register(builtin, runtime);
    return runtime.RegisterAtExit(SpecFor(builtin).cleanup);
}

void ActorManagerScriptBuiltinRegistration::Register(
    const StaticBuiltin builtin, IScriptBuiltinStaticRuntime& runtime) {
    const auto spec = SpecFor(builtin);
    runtime.RegisterBuiltinAt(builtin, spec.descriptor, spec.native_entry);
}

void ActorManagerScriptBuiltinRegistration::Cleanup(
    const StaticBuiltin builtin, IScriptBuiltinStaticRuntime& runtime) {
    runtime.DeallocateDescriptor(SpecFor(builtin).descriptor);
}

}  // namespace pal4
