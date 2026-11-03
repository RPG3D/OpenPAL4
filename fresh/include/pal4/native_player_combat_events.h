#pragma once

#include <array>
#include <cstdint>
#include <memory>

namespace pal4 {
struct ScriptBindingSpec;
// Per-call property receiver; it borrows no mutable registry state. Implementers
// validate actor identity and preserve their actual getter/setter event semantics.
class INativeCombatEventAttributes {
public:
    virtual ~INativeCombatEventAttributes() = default;
    [[nodiscard]] virtual float ReadScriptAttribute(std::uint32_t actor,const ScriptBindingSpec&) = 0;
    virtual int WriteScriptAttribute(std::uint32_t actor,const ScriptBindingSpec&,std::uint32_t float_bits) = 0;
};
// Owns the original event registry projected onto typed BASIC/Player fields.
// Descriptors are native adapter handles, never original callable addresses.
class NativePlayerCombatEvents final {
public:
    using Record = std::array<std::uint32_t,3u>;
    NativePlayerCombatEvents();
    ~NativePlayerCombatEvents();
    NativePlayerCombatEvents(const NativePlayerCombatEvents&) = delete;
    NativePlayerCombatEvents& operator=(const NativePlayerCombatEvents&) = delete;
    //581760 may be repeated: retain guarded object identities, refresh table.
    void Initialize();
    [[nodiscard]] std::uint32_t Lookup(std::uint32_t index);
    void Build(Record& record,std::uint32_t actor,std::uint32_t index,float scale,std::uint32_t mode);
    void Process(Record& record,std::uint32_t target);
    // Actual Combat actors retain the original BASIC layout; their owner must
    // validate and pin the actor. Writes use its explicit native event receiver.
    void BuildForNativeActor(Record&,std::uint32_t actor,std::uint32_t index,float scale,std::uint32_t mode);
    void Process(Record&,std::uint32_t target,INativeCombatEventAttributes&);
    [[nodiscard]] bool Matches(Record& record,std::uint32_t multiplier_bits);
    [[nodiscard]] double ReadValue(std::uint32_t event,std::uint32_t source);
    void Increment(std::uint32_t event,std::uint32_t target,std::uint32_t bits);
    // Uses the same owned descriptor and shared event algorithm with an explicit
    // native receiver adapter. No Player fallback or registry rebinding occurs.
    void Increment(std::uint32_t event,std::uint32_t target,std::uint32_t bits,INativeCombatEventAttributes&);
    void Set(std::uint32_t event,std::uint32_t target,std::uint32_t bits);
    void Set(std::uint32_t event,std::uint32_t target,std::uint32_t bits,INativeCombatEventAttributes&);
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}  // namespace pal4
