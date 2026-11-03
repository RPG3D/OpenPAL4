#pragma once

#include "pal4/monster_manager_access.h"

#include <cstdint>

namespace pal4 {

class IMonsterManagerTreeWalkRuntime {
public:
    virtual ~IMonsterManagerTreeWalkRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t InvokeCallback(
        std::uint32_t callback, std::uint32_t object,
        std::uint32_t context) = 0;
    virtual void StopPursuit(std::uint32_t object,
                             float incoming_st0) = 0;
    virtual void ClearStopPursuit(std::uint32_t object) = 0;
};

class MonsterManagerTreeWalkRuntimeStub : public IMonsterManagerTreeWalkRuntime {
public:
    std::uint32_t ReadDword(std::uint32_t) override;
    std::uint32_t InvokeCallback(std::uint32_t, std::uint32_t,
                                 std::uint32_t) override;
    void StopPursuit(std::uint32_t, float) override;
    void ClearStopPursuit(std::uint32_t) override;
};

class MonsterManagerTreeWalker final {
public:
    [[nodiscard]] static std::uint32_t ForEach(
        std::uint32_t manager, std::uint32_t callback,
        std::uint32_t context,
        const MonsterManagerTreeGlobals& globals,
        IMonsterManagerTreeWalkRuntime& runtime);
    [[nodiscard]] static std::uint32_t StopPursuitAll(
        std::uint32_t manager, float incoming_st0,
        const MonsterManagerTreeGlobals& globals,
        IMonsterManagerTreeWalkRuntime& runtime);
    [[nodiscard]] static std::uint32_t ClearStopPursuitAll(
        std::uint32_t manager, const MonsterManagerTreeGlobals& globals,
        IMonsterManagerTreeWalkRuntime& runtime);
};

}  // namespace pal4
