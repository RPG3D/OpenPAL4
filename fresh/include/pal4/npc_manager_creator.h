#pragma once

#include "pal4/monster_manager_creator.h"

namespace pal4 {

struct NpcCreationArguments {
    std::uint32_t name{};
    std::uint32_t config{};
    std::uint32_t position{};
    std::uint32_t facing{};
    std::uint32_t source_record{};
    std::uint32_t enabled{};
};

class INpcManagerCreateRuntime : public MonsterManagerCreateRuntimeStub {
public:
    [[nodiscard]] virtual std::uint32_t FindChild(
        std::uint32_t document, const char* name) = 0;
    virtual void ReadStringAttribute(
        std::uint32_t node, const char* name, const char* fallback,
        ModeStringBufferRecord& result) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyTextAddress() = 0;
    virtual void SetDefaultAction(std::uint32_t object,
                                  std::uint32_t text) = 0;
    virtual void SetupBehavior(std::uint32_t object,
                               std::uint32_t source_record,
                               std::uint32_t enabled) = 0;
};

class NpcManagerCreateRuntimeStub : public INpcManagerCreateRuntime {
public:
    std::uint32_t FindChild(std::uint32_t, const char*) override;
    void ReadStringAttribute(std::uint32_t, const char*, const char*,
                             ModeStringBufferRecord&) override;
    std::uint32_t EmptyTextAddress() override;
    void SetDefaultAction(std::uint32_t, std::uint32_t) override;
    void SetupBehavior(std::uint32_t, std::uint32_t,
                       std::uint32_t) override;
};

class NpcManagerCreator final {
public:
    static constexpr std::uint32_t kObjectBytes = 0x6ecu;
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t manager, const NpcCreationArguments& arguments,
        INpcManagerCreateRuntime& runtime);
};

}  // namespace pal4
