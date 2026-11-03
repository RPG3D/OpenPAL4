#pragma once

#include <cstdint>
#include <string_view>

#include "pal4/pet_manager_state_updater.h"

namespace pal4 {

struct PetManagerAccessState {
    std::uint8_t guard{};
    std::uint32_t instance{};
};

class IPetManagerRuntime {
public:
    virtual ~IPetManagerRuntime() = default;
    [[nodiscard]] virtual std::uint32_t SingletonManager() = 0;
    [[nodiscard]] virtual std::uint32_t Register(
        std::uint32_t manager, std::string_view name, std::uint32_t id,
        std::uint32_t factory, std::uint32_t type_info) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void BaseConstruct(std::uint32_t object) = 0;
    virtual void BaseDestroy(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ResetGameObject(
        std::uint32_t object) = 0;
    virtual void ProcessScript(std::uint32_t script) = 0;
    virtual void InitializeGameObject(std::uint32_t object,
                                      std::uint32_t first,
                                      std::uint32_t second,
                                      std::uint32_t third) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupScripts() = 0;
    [[nodiscard]] virtual std::uint32_t UpdateGameObject(
        std::uint32_t object, std::uint32_t argument) = 0;
    [[nodiscard]] virtual std::uint32_t SetProperty(
        std::uint32_t object, std::uint32_t value) = 0;
    [[nodiscard]] virtual double GetDoubleProperty(
        std::uint32_t source) = 0;
    virtual void SetPetPosition(std::uint32_t pet,
                                const PetVector3& position,
                                std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t SetPetFacing(
        std::uint32_t pet, std::uint32_t source_vector,
        std::uint32_t mode) = 0;
};

class PetManagerRuntimeStub : public IPetManagerRuntime {
public:
    std::uint32_t SingletonManager() override;
    std::uint32_t Register(std::uint32_t, std::string_view,
                           std::uint32_t, std::uint32_t,
                           std::uint32_t) override;
    std::uint32_t Allocate(std::uint32_t) override;
    void Free(std::uint32_t) override;
    void BaseConstruct(std::uint32_t) override;
    void BaseDestroy(std::uint32_t) override;
    std::uint32_t ReadDword(std::uint32_t) override;
    void WriteDword(std::uint32_t, std::uint32_t) override;
    std::uint32_t ResetGameObject(std::uint32_t) override;
    void ProcessScript(std::uint32_t) override;
    void InitializeGameObject(std::uint32_t, std::uint32_t,
                              std::uint32_t, std::uint32_t) override;
    std::uint32_t CleanupScripts() override;
    std::uint32_t UpdateGameObject(std::uint32_t,
                                   std::uint32_t) override;
    std::uint32_t SetProperty(std::uint32_t,
                              std::uint32_t) override;
    double GetDoubleProperty(std::uint32_t) override;
    void SetPetPosition(std::uint32_t, const PetVector3&,
                        std::uint32_t) override;
    std::uint32_t SetPetFacing(std::uint32_t, std::uint32_t,
                               std::uint32_t) override;
};

class PetManager final {
public:
    static constexpr std::uint32_t kObjectBytes = 0x674u;
    static constexpr std::uint32_t kGuardAddress = 0x8e4344u;
    static constexpr std::uint32_t kInstanceAddress = 0x8e4340u;
    static constexpr std::uint32_t kSingletonId = 0x412u;
    static constexpr std::uint32_t kFactoryAddress = 0x43dea0u;
    static constexpr std::uint32_t kTypeInfoAddress = 0x7de5d0u;
    static constexpr std::uint32_t kVtable = 0x842e98u;
    static constexpr std::uint32_t kSubVtable170 = 0x842e8cu;
    static constexpr std::uint32_t kSubVtable1ec = 0x842e80u;

    [[nodiscard]] static std::uint32_t GetInstance(
        PetManagerAccessState& state, IPetManagerRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create(IPetManagerRuntime& runtime);
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IPetManagerRuntime& runtime);
    static void Destroy(std::uint32_t object, IPetManagerRuntime& runtime);
    [[nodiscard]] static std::uint32_t Reset(
        std::uint32_t object, IPetManagerRuntime& runtime);
    [[nodiscard]] static std::uint32_t ExecuteScriptAndInitialize(
        std::uint32_t object, std::uint32_t first, std::uint32_t second,
        std::uint32_t third, IPetManagerRuntime& runtime);
    [[nodiscard]] static std::uint32_t ConditionalUpdate(
        std::uint32_t object, std::uint32_t argument,
        IPetManagerRuntime& runtime);
    // IDA 0x43e250: direct thunk to GameObject_SetProperty.
    [[nodiscard]] static std::uint32_t ForwardProperty(
        std::uint32_t object, std::uint32_t value,
        IPetManagerRuntime& runtime);
    // IDA 0x43e260 and 0x43e2e0: initialize from a source object.
    [[nodiscard]] static std::uint32_t InitializePetData(
        std::uint32_t object, std::uint32_t source,
        IPetManagerRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeStoredPetData(
        std::uint32_t object, IPetManagerRuntime& runtime);
    // IDA 0x43e360: clear source and restore defaults.
    [[nodiscard]] static std::uint32_t ResetState(
        std::uint32_t object, IPetManagerRuntime& runtime);

private:
    [[nodiscard]] static std::uint32_t ApplyPetData(
        std::uint32_t object, std::uint32_t source,
        IPetManagerRuntime& runtime);
};

}  // namespace pal4
