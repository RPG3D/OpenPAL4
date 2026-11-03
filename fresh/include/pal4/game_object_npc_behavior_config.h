#pragma once

#include <cstdint>

namespace pal4 {

class IGameObjectNpcBehaviorRuntime {
public:
    virtual ~IGameObjectNpcBehaviorRuntime() = default;
    [[nodiscard]] virtual std::uint32_t FindAttribute(
        std::uint32_t node, const char* name) = 0;
    [[nodiscard]] virtual std::int32_t AttributeToInt(
        std::uint32_t attribute) = 0;
    [[nodiscard]] virtual float AttributeToFloat(
        std::uint32_t attribute) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t GetChildByIndex(
        std::uint32_t node, std::uint32_t& index) = 0;
    [[nodiscard]] virtual std::uint32_t AddEvent(
        std::uint32_t processor, const char* name) = 0;
    [[nodiscard]] virtual std::int32_t ResolveClassId(
        std::int32_t object_type) = 0;
    [[nodiscard]] virtual std::uint32_t AddClassProportion(
        std::uint32_t event, std::int32_t class_id,
        std::int32_t proportion) = 0;
    virtual void WriteInt(std::uint32_t node, const char* name,
                          std::int32_t value) = 0;
    virtual void WriteFloat(std::uint32_t node, const char* name,
                            float value) = 0;
    virtual void SetGroupMode(std::uint32_t event,
                              std::int32_t mode) = 0;
};

class GameObjectNpcBehaviorRuntimeStub : public IGameObjectNpcBehaviorRuntime {
public:
    std::uint32_t FindAttribute(std::uint32_t, const char*) override;
    std::int32_t AttributeToInt(std::uint32_t) override;
    float AttributeToFloat(std::uint32_t) override;
    std::uint32_t ReadDword(std::uint32_t) override;
    std::uint32_t GetChildByIndex(std::uint32_t,
                                  std::uint32_t&) override;
    std::uint32_t AddEvent(std::uint32_t, const char*) override;
    std::int32_t ResolveClassId(std::int32_t) override;
    std::uint32_t AddClassProportion(std::uint32_t,
                                     std::int32_t, std::int32_t) override;
    void WriteInt(std::uint32_t, const char*, std::int32_t) override;
    void WriteFloat(std::uint32_t, const char*, float) override;
    void SetGroupMode(std::uint32_t, std::int32_t) override;
};

class GameObjectNpcBehaviorConfig final {
public:
    // IDA 0x4395e0. A zero config or GroupMode 3 produces no event.
    static void Process(std::uint32_t owner_address,
                        std::uint32_t config_node,
                        IGameObjectNpcBehaviorRuntime& runtime);
    // IDA 0x4397f0 and its five constant-return callees.
    [[nodiscard]] static std::int32_t ClassIdFromObjectType(
        std::int32_t object_type);
};

}  // namespace pal4
