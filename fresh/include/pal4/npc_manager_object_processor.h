#pragma once

#include "pal4/npc_manager_access.h"

namespace pal4 {

class INpcManagerProcessRuntime {
public:
    virtual ~INpcManagerProcessRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void Dispatch(std::uint32_t npc,
                          std::uint32_t vtable_slot,
                          std::uint32_t first,
                          std::uint32_t second) = 0;
    virtual void UpdateGrid(std::uint32_t grid,
                            std::uint32_t first) = 0;
};

class NpcManagerProcessRuntimeStub : public INpcManagerProcessRuntime {
public:
    std::uint32_t ReadDword(std::uint32_t) override;
    void Dispatch(std::uint32_t, std::uint32_t,
                  std::uint32_t, std::uint32_t) override;
    void UpdateGrid(std::uint32_t, std::uint32_t) override;
};

class NpcManagerObjectProcessor final {
public:
    // IDA 0x43c7e0: active NPC virtual slot +0x34.
    [[nodiscard]] static std::uint32_t Process(
        std::uint32_t manager, std::uint32_t argument,
        const NpcManagerTreeGlobals& globals,
        INpcManagerProcessRuntime& runtime);
    // IDA 0x43c850: active NPC virtual slot +0x24.
    [[nodiscard]] static std::uint32_t ProcessAlternative(
        std::uint32_t manager, std::uint32_t argument,
        const NpcManagerTreeGlobals& globals,
        INpcManagerProcessRuntime& runtime);
    // IDA 0x43c8c0: grid update, then active NPC virtual slot +0x38.
    [[nodiscard]] static std::uint32_t ProcessExtended(
        std::uint32_t manager, std::uint32_t first,
        std::uint32_t second,
        const NpcManagerTreeGlobals& globals,
        INpcManagerProcessRuntime& runtime);
};

class INpcManagerForEachRuntime {
public:
    virtual ~INpcManagerForEachRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ForEachNpc(
        std::uint32_t manager, std::uint32_t callback,
        std::uint32_t context) = 0;
};

class NpcManagerForEachRuntimeStub : public INpcManagerForEachRuntime {
public:
    std::uint32_t ForEachNpc(std::uint32_t, std::uint32_t,
                             std::uint32_t) override;
};

class NpcManagerForEach final {
public:
    // IDA 0x43c940: forwards callback and context to the NPC tree walker.
    [[nodiscard]] static std::uint32_t Apply(
        std::uint32_t manager, std::uint32_t callback,
        std::uint32_t context,
        INpcManagerForEachRuntime& runtime);
};

class INpcManagerCallbackRuntime {
public:
    virtual ~INpcManagerCallbackRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Invoke(
        std::uint32_t callback, std::uint32_t npc,
        std::uint32_t context) = 0;
};

class NpcManagerCallbackRuntimeStub : public INpcManagerCallbackRuntime {
public:
    std::uint32_t ReadDword(std::uint32_t) override;
    std::uint32_t Invoke(std::uint32_t, std::uint32_t,
                         std::uint32_t) override;
};

class NpcManagerCallbackWalker final {
public:
    // IDA 0x43d090: visit all NPC nodes until the callback returns zero.
    static void Walk(std::uint32_t manager, std::uint32_t callback,
                     std::uint32_t context,
                     const NpcManagerTreeGlobals& globals,
                     INpcManagerCallbackRuntime& runtime);
};

}  // namespace pal4
