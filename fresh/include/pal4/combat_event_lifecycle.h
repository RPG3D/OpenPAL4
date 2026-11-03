#pragma once

#include <cstdint>

namespace pal4 {

class ICombatEventCallbackRuntime {
public:
    virtual ~ICombatEventCallbackRuntime()=default;
    virtual void WriteWord(std::uint32_t address,std::uint32_t value)=0;
};
class ICombatEventValueInvokeRuntime {
public:
    virtual ~ICombatEventValueInvokeRuntime()=default;
    virtual std::uint32_t ReadWord(std::uint32_t address)=0;
    virtual std::uint32_t InvokeValue(std::uint32_t owner,std::uint32_t method,std::uint32_t value)=0;
};
class ICombatEventLifecycleRuntime : public ICombatEventCallbackRuntime {
public:
    virtual ~ICombatEventLifecycleRuntime() = default;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void InitializeEmptyString(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupString(
        std::uint32_t address) = 0;
};

class CombatEventLifecycle final {
public:
    static constexpr std::uint32_t kCallbackVtable = 0x8480ecu;
    // IDA 0x57be80.
    [[nodiscard]] static std::uint32_t InitializeObject(
        std::uint32_t object, std::uint32_t first,
        std::uint32_t second, std::uint32_t third,
        std::uint32_t fourth, ICombatEventCallbackRuntime& runtime) {
        runtime.WriteWord(object+4u,first);runtime.WriteWord(object+8u,second);
        runtime.WriteWord(object,kCallbackVtable);runtime.WriteWord(object+12u,third);
        runtime.WriteWord(object+16u,fourth);return object;
    }
    // IDA57BEB0: adjusted receiver, function+8, one value at+16.
    [[nodiscard]] static std::uint32_t InvokeValue(std::uint32_t object,ICombatEventValueInvokeRuntime& runtime) {
        return runtime.InvokeValue(runtime.ReadWord(object+4u)+runtime.ReadWord(object+12u),
            runtime.ReadWord(object+8u),runtime.ReadWord(object+16u));
    }
    // IDA 0x57bf00.
    [[nodiscard]] static std::uint32_t InitializeRouter(
        std::uint32_t router, ICombatEventLifecycleRuntime& runtime);
    // IDA 0x57bf30.
    [[nodiscard]] static std::uint32_t CleanupRouter(
        std::uint32_t router, ICombatEventLifecycleRuntime& runtime);
};

}  // namespace pal4
