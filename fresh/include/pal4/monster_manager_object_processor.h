#pragma once

#include "pal4/monster_manager_access.h"

#include <cstdint>

namespace pal4 {

class IMonsterManagerObjectProcessRuntime {
public:
    virtual ~IMonsterManagerObjectProcessRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void UpdateGrid(std::uint32_t grid, std::uint32_t first) = 0;
    virtual void CallSlot34(std::uint32_t object, std::uint32_t argument) = 0;
    virtual void CallSlot38(std::uint32_t object, std::uint32_t first,
                            std::uint32_t second) = 0;
    virtual void ProcessCollision(std::uint32_t object,
                                  float incoming_st0,
                                  std::uint32_t argument) = 0;
    virtual void SetPursuitTarget(std::uint32_t object,
                                  std::uint32_t target) = 0;
};

class MonsterManagerObjectProcessRuntimeStub
    : public IMonsterManagerObjectProcessRuntime {
public:
    std::uint32_t ReadDword(std::uint32_t) override;
    std::uint8_t ReadByte(std::uint32_t) override;
    void UpdateGrid(std::uint32_t, std::uint32_t) override;
    void CallSlot34(std::uint32_t, std::uint32_t) override;
    void CallSlot38(std::uint32_t, std::uint32_t,
                    std::uint32_t) override;
    void ProcessCollision(std::uint32_t, float,
                          std::uint32_t) override;
    void SetPursuitTarget(std::uint32_t, std::uint32_t) override;
};

class MonsterManagerObjectProcessor final {
public:
    [[nodiscard]] static std::uint32_t Process(
        std::uint32_t manager, std::uint32_t argument,
        MonsterManagerTreeGlobals& globals,
        IMonsterManagerObjectProcessRuntime& runtime);
    [[nodiscard]] static std::uint32_t ProcessExtended(
        std::uint32_t manager, std::uint32_t first,
        std::uint32_t second, MonsterManagerTreeGlobals& globals,
        IMonsterManagerObjectProcessRuntime& runtime);
    [[nodiscard]] static std::uint32_t ProcessCollisions(
        std::uint32_t manager, float incoming_st0,
        std::uint32_t argument, MonsterManagerTreeGlobals& globals,
        IMonsterManagerObjectProcessRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetPursuitTargetAll(
        std::uint32_t manager, std::uint32_t target,
        MonsterManagerTreeGlobals& globals,
        IMonsterManagerObjectProcessRuntime& runtime);

private:
    enum class Mode { Basic, Extended, Collision, PursuitTarget };
    [[nodiscard]] static std::uint32_t Walk(
        std::uint32_t manager, std::uint32_t first,
        std::uint32_t second, float incoming_st0, Mode mode,
        MonsterManagerTreeGlobals& globals,
        IMonsterManagerObjectProcessRuntime& runtime);
};

}  // namespace pal4
