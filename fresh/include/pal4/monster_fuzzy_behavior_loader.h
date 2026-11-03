#pragma once

#include <cstdint>

namespace pal4 {

class IMonsterFuzzyBehaviorRuntime {
public:
    virtual ~IMonsterFuzzyBehaviorRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Attribute(
        std::uint32_t node, const char* name) = 0;
    [[nodiscard]] virtual std::int32_t ParseInt(
        std::uint32_t attribute, std::int32_t fallback) = 0;
    [[nodiscard]] virtual float ParseFloat(
        std::uint32_t attribute, float fallback) = 0;
    [[nodiscard]] virtual std::int32_t ChildCount(
        std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t ChildByIndex(
        std::uint32_t node, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t CreateEvent(
        std::uint32_t manager_address, const char* name) = 0;
    [[nodiscard]] virtual std::uint32_t AddClassProportion(
        std::uint32_t event_address, std::int32_t class_id,
        std::int32_t proportion) = 0;
    virtual void SetIntAttribute(std::uint32_t record_address,
                                 const char* name, std::int32_t value) = 0;
    virtual void SetFloatAttribute(std::uint32_t record_address,
                                   const char* name, float value) = 0;
    virtual void SetGroupMode(std::uint32_t event_address,
                              std::int32_t mode) = 0;
};

class MonsterFuzzyBehaviorRuntimeStub final
    : public IMonsterFuzzyBehaviorRuntime {
public:
    std::uint32_t Attribute(std::uint32_t node, const char* name) override;
    std::int32_t ParseInt(std::uint32_t attribute,
                          std::int32_t fallback) override;
    float ParseFloat(std::uint32_t attribute, float fallback) override;
    std::int32_t ChildCount(std::uint32_t node) override;
    std::uint32_t ChildByIndex(std::uint32_t node,
                               std::uint32_t index) override;
    std::uint32_t CreateEvent(std::uint32_t manager_address,
                              const char* name) override;
    std::uint32_t AddClassProportion(std::uint32_t event_address,
                                     std::int32_t class_id,
                                     std::int32_t proportion) override;
    void SetIntAttribute(std::uint32_t record_address,
                         const char* name, std::int32_t value) override;
    void SetFloatAttribute(std::uint32_t record_address,
                           const char* name, float value) override;
    void SetGroupMode(std::uint32_t event_address,
                      std::int32_t mode) override;
};

class MonsterFuzzyBehaviorLoader final {
public:
    static void Load(std::uint32_t manager_address,
                     std::uint32_t config_address,
                     IMonsterFuzzyBehaviorRuntime& runtime);
    [[nodiscard]] static std::int32_t ResolveObjectType(
        std::int32_t object_type);
};

}  // namespace pal4
