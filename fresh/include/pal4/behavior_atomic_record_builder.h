#pragma once

#include "pal4/script_event_data.h"

#include <cstdint>

namespace pal4 {

struct BehaviorAtomicAttributePosition {
    std::uint32_t current = 0u;
    std::uint32_t end = 0u;
};

class IBehaviorAtomicRecordRuntime {
public:
    virtual ~IBehaviorAtomicRecordRuntime() = default;
    [[nodiscard]] virtual std::uint32_t InitializeNode(const char* name) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeClassKey(
        const char* name, std::uint8_t incoming_byte) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeProportionKey(
        const char* name, std::uint8_t incoming_byte) = 0;
    [[nodiscard]] virtual BehaviorAtomicAttributePosition FindAttribute(
        std::uint32_t node, std::uint32_t key) = 0;
    virtual void CleanupKey(std::uint32_t key) = 0;
    [[nodiscard]] virtual std::uint8_t SetAttributeInt(
        std::uint32_t attribute, std::int32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeAttribute(
        const char* name, std::int32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t AddAttribute(
        std::uint32_t node, std::uint32_t attribute) = 0;
    virtual void DestroyAttribute(std::uint32_t attribute) = 0;
    virtual void InsertNode(ScriptEventDataState& state,
                            std::uint32_t node) = 0;
    virtual void DestroyNode(std::uint32_t node) = 0;
};

class BehaviorAtomicRecordRuntimeStub final
    : public IBehaviorAtomicRecordRuntime {
public:
    std::uint32_t InitializeNode(const char* name) override;
    std::uint32_t InitializeClassKey(
        const char* name, std::uint8_t incoming_byte) override;
    std::uint32_t InitializeProportionKey(
        const char* name, std::uint8_t incoming_byte) override;
    BehaviorAtomicAttributePosition FindAttribute(
        std::uint32_t node, std::uint32_t key) override;
    void CleanupKey(std::uint32_t key) override;
    std::uint8_t SetAttributeInt(
        std::uint32_t attribute, std::int32_t value) override;
    std::uint32_t InitializeAttribute(
        const char* name, std::int32_t value) override;
    std::uint8_t AddAttribute(
        std::uint32_t node, std::uint32_t attribute) override;
    void DestroyAttribute(std::uint32_t attribute) override;
    void InsertNode(ScriptEventDataState& state,
                    std::uint32_t node) override;
    void DestroyNode(std::uint32_t node) override;
};

class BehaviorAtomicRecordBuilder final {
public:
    [[nodiscard]] static std::uint32_t AddClassProportion(
        ScriptEventDataState& state, std::int32_t class_id,
        std::int32_t proportion, std::uint8_t class_key_byte,
        std::uint8_t proportion_key_byte,
        IBehaviorAtomicRecordRuntime& runtime);
};

}  // namespace pal4
