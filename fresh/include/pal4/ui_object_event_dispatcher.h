#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace pal4 {

struct UiEventArgsRecord {
    std::uint32_t vtable{};
    std::uint8_t handled{};
    std::array<std::uint8_t, 11> padding{};
    std::uint32_t tail{};
};

static_assert(sizeof(UiEventArgsRecord) == 20u);
static_assert(offsetof(UiEventArgsRecord, tail) == 16u);

class IUiObjectEventRuntime {
public:
    virtual ~IUiObjectEventRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadEventCode(
        std::uint32_t event_member) = 0;
    [[nodiscard]] virtual std::uint32_t EventArgsVtable() = 0;
    virtual void DispatchAction(std::uint32_t object_address,
                                const UiEventArgsRecord& args) = 0;
    [[nodiscard]] virtual bool ForwardKeyEvent(
        std::uint32_t subobject_address,
        std::uint32_t event_address) = 0;
};

class UiObjectEventRuntimeStub : public IUiObjectEventRuntime {
public:
    [[nodiscard]] std::uint32_t ReadEventCode(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t EventArgsVtable() override { return 0u; }
    void DispatchAction(std::uint32_t,
                        const UiEventArgsRecord&) override {}
    [[nodiscard]] bool ForwardKeyEvent(
        std::uint32_t, std::uint32_t) override { return false; }
};

class UiObjectEventDispatcher final {
public:
    [[nodiscard]] static bool Handle(std::uint32_t subobject_address,
                                     std::uint32_t event_address,
                                     IUiObjectEventRuntime& runtime);
};

}  // namespace pal4
