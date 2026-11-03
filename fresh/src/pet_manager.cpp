#include "pal4/pet_manager.h"

#include <bit>
#include <initializer_list>

namespace pal4 {

std::uint32_t PetManagerRuntimeStub::SingletonManager() { return 0u; }
std::uint32_t PetManagerRuntimeStub::Register(
    std::uint32_t, std::string_view, std::uint32_t,
    std::uint32_t, std::uint32_t) { return 0u; }
std::uint32_t PetManagerRuntimeStub::Allocate(std::uint32_t) { return 0u; }
void PetManagerRuntimeStub::Free(std::uint32_t) {}
void PetManagerRuntimeStub::BaseConstruct(std::uint32_t) {}
void PetManagerRuntimeStub::BaseDestroy(std::uint32_t) {}
std::uint32_t PetManagerRuntimeStub::ReadDword(std::uint32_t) { return 0u; }
void PetManagerRuntimeStub::WriteDword(std::uint32_t, std::uint32_t) {}
std::uint32_t PetManagerRuntimeStub::ResetGameObject(std::uint32_t) { return 0u; }
void PetManagerRuntimeStub::ProcessScript(std::uint32_t) {}
void PetManagerRuntimeStub::InitializeGameObject(
    std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t) {}
std::uint32_t PetManagerRuntimeStub::CleanupScripts() { return 0u; }
std::uint32_t PetManagerRuntimeStub::UpdateGameObject(
    std::uint32_t, std::uint32_t) { return 0u; }
std::uint32_t PetManagerRuntimeStub::SetProperty(
    std::uint32_t, std::uint32_t) { return 0u; }
double PetManagerRuntimeStub::GetDoubleProperty(std::uint32_t) {
    return 0.0;
}
void PetManagerRuntimeStub::SetPetPosition(
    std::uint32_t, const PetVector3&, std::uint32_t) {}
std::uint32_t PetManagerRuntimeStub::SetPetFacing(
    std::uint32_t, std::uint32_t, std::uint32_t) { return 0u; }

std::uint32_t PetManager::GetInstance(
    PetManagerAccessState& state, IPetManagerRuntime& runtime) {
    if ((state.guard & 1u) != 0u) return state.instance;
    state.guard |= 1u;
    const auto manager = runtime.SingletonManager();
    state.instance = runtime.Register(manager, "palPet", kSingletonId,
                                      kFactoryAddress, kTypeInfoAddress);
    return state.instance;
}

std::uint32_t PetManager::Create(IPetManagerRuntime& runtime) {
    const auto object = runtime.Allocate(kObjectBytes);
    if (object == 0u) return 0u;
    try {
        return Construct(object, runtime);
    } catch (...) {
        runtime.Free(object);
        throw;
    }
}

std::uint32_t PetManager::Construct(
    std::uint32_t object, IPetManagerRuntime& runtime) {
    runtime.BaseConstruct(object);
    runtime.WriteDword(object, kVtable);
    for (const auto offset : {0x654u, 0x658u, 0x65cu, 0x660u})
        runtime.WriteDword(object + offset, 0u);
    runtime.WriteDword(object + 0x170u, kSubVtable170);
    runtime.WriteDword(object + 0x1ecu, kSubVtable1ec);
    runtime.WriteDword(object + 0x668u, 0x41a00000u);
    runtime.WriteDword(object + 0x670u, 0x43790000u);
    return object;
}

void PetManager::Destroy(
    std::uint32_t object, IPetManagerRuntime& runtime) {
    runtime.WriteDword(object, kVtable);
    runtime.WriteDword(object + 0x170u, kSubVtable170);
    runtime.WriteDword(object + 0x1ecu, kSubVtable1ec);
    runtime.BaseDestroy(object);
}

std::uint32_t PetManager::Reset(
    std::uint32_t object, IPetManagerRuntime& runtime) {
    runtime.WriteDword(object + 0x668u, 0x41a00000u);
    runtime.WriteDword(object + 0x658u, 0u);
    runtime.WriteDword(object + 0x65cu, 0u);
    runtime.WriteDword(object + 0x660u, 0u);
    return runtime.ResetGameObject(object);
}

std::uint32_t PetManager::ExecuteScriptAndInitialize(
    std::uint32_t object, std::uint32_t first, std::uint32_t second,
    std::uint32_t third, IPetManagerRuntime& runtime) {
    const auto script = runtime.ReadDword(object + 0x654u);
    if (script == 0u) return 0u;
    runtime.ProcessScript(script);
    runtime.InitializeGameObject(object, first, second, third);
    return runtime.CleanupScripts();
}

std::uint32_t PetManager::ConditionalUpdate(
    std::uint32_t object, std::uint32_t argument,
    IPetManagerRuntime& runtime) {
    if (runtime.ReadDword(object + 0x654u) == 0u) return 0u;
    if (runtime.ReadDword(object + 0x1f0u) == 0u) return 0u;
    return runtime.UpdateGameObject(object, argument);
}

std::uint32_t PetManager::ForwardProperty(
    std::uint32_t object, std::uint32_t value,
    IPetManagerRuntime& runtime) {
    return runtime.SetProperty(object, value);
}

std::uint32_t PetManager::InitializePetData(
    std::uint32_t object, std::uint32_t source,
    IPetManagerRuntime& runtime) {
    runtime.WriteDword(object + 0x658u, source);
    return ApplyPetData(object, source, runtime);
}

std::uint32_t PetManager::InitializeStoredPetData(
    std::uint32_t object, IPetManagerRuntime& runtime) {
    const auto source = runtime.ReadDword(object + 0x658u);
    if (source == 0u) return 0u;
    return ApplyPetData(object, source, runtime);
}

std::uint32_t PetManager::ApplyPetData(
    std::uint32_t object, std::uint32_t source,
    IPetManagerRuntime& runtime) {
    runtime.WriteDword(object + 0x65cu, 0u);
    runtime.WriteDword(object + 0x660u, 0u);
    const double property = runtime.GetDoubleProperty(source);
    runtime.WriteDword(object + 0x664u,
                       std::bit_cast<std::uint32_t>(static_cast<float>(property)));

    const auto current = runtime.ReadDword(object + 0x658u);
    const PetVector3 position{
        std::bit_cast<float>(runtime.ReadDword(current + 0xacu)),
        static_cast<float>(property + std::bit_cast<float>(
            runtime.ReadDword(current + 0xb0u))),
        std::bit_cast<float>(runtime.ReadDword(current + 0xb4u))};
    runtime.SetPetPosition(object, position, 0u);
    return runtime.SetPetFacing(
        object, runtime.ReadDword(object + 0x658u) + 0xa0u, 0u);
}

std::uint32_t PetManager::ResetState(
    std::uint32_t object, IPetManagerRuntime& runtime) {
    runtime.WriteDword(object + 0x660u, 1u);
    runtime.WriteDword(object + 0x658u, 0u);
    runtime.WriteDword(object + 0x65cu, 0u);
    runtime.WriteDword(object + 0x668u, 0x41a00000u);
    return 0u;
}

}  // namespace pal4
