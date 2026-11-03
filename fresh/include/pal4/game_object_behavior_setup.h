#pragma once

#include "pal4/game_object_lifecycle.h"

#include <cstdint>

namespace pal4 {

class IGameObjectBehaviorSetupRuntime {
public:
    virtual ~IGameObjectBehaviorSetupRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    virtual void ClearProcessorEvents(std::uint32_t processor) = 0;
    virtual void CleanupScriptCollection(std::uint32_t script) = 0;
    virtual void SetProcessorValue(std::uint32_t processor,
                                   std::uint32_t callback) = 0;
    virtual void SetProcessorFlag(std::uint32_t processor,
                                  std::uint32_t flag) = 0;
    [[nodiscard]] virtual std::uint32_t GetChildByIndex(
        std::uint32_t document, std::uint32_t& index) = 0;
    virtual void ProcessNpcBehaviorConfig(GameObjectState& state,
                                          std::uint32_t address,
                                          std::uint32_t child) = 0;
    [[nodiscard]] virtual std::uint32_t CallObjectVirtual(
        GameObjectState& state, std::uint32_t address,
        std::uint32_t slot, std::uint32_t source,
        std::uint32_t flag) = 0;
};

class GameObjectBehaviorSetupRuntimeStub : public IGameObjectBehaviorSetupRuntime {
public:
    std::uint32_t ReadDword(std::uint32_t) override;
    void ClearProcessorEvents(std::uint32_t) override;
    void CleanupScriptCollection(std::uint32_t) override;
    void SetProcessorValue(std::uint32_t, std::uint32_t) override;
    void SetProcessorFlag(std::uint32_t, std::uint32_t) override;
    std::uint32_t GetChildByIndex(std::uint32_t,
                                  std::uint32_t&) override;
    void ProcessNpcBehaviorConfig(GameObjectState&, std::uint32_t,
                                  std::uint32_t) override;
    std::uint32_t CallObjectVirtual(GameObjectState&, std::uint32_t,
                                    std::uint32_t, std::uint32_t,
                                    std::uint32_t) override;
};

class GameObjectBehaviorSetup final {
public:
    static constexpr std::uint32_t kBehaviorDocumentOffset = 0x4cu;
    static constexpr std::uint32_t kBehaviorChildOffset = 0x6d8u;
    static constexpr std::uint32_t kTaskReturnTrue = 0x594e00u;

    // IDA 0x439470.
    static void Setup(GameObjectState& state, std::uint32_t address,
                      std::uint32_t npc_record, std::uint32_t attach_fields,
                      IGameObjectBehaviorSetupRuntime& runtime);
    // IDA 0x4394f0.
    static void ProcessEventsAndClear(
        GameObjectState& state, std::uint32_t address,
        std::uint32_t npc_record, std::uint32_t attach_fields,
        IGameObjectBehaviorSetupRuntime& runtime);
    // IDA 0x439590: returns the final EAX value.
    [[nodiscard]] static std::uint32_t CleanupEventsAndScripts(
        GameObjectState& state, std::uint32_t address,
        std::uint32_t detach_field,
        IGameObjectBehaviorSetupRuntime& runtime);
};

}  // namespace pal4
