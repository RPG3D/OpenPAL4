#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

struct OlInfoEventWords {
    std::uint32_t first{};
    std::uint32_t second{};
};

class IOlInfoCommandRuntime {
public:
    virtual ~IOlInfoCommandRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructBaseEvent() = 0;
    virtual void DestroyBaseEvent(std::uint32_t event) = 0;
    virtual void HideWindow(std::uint32_t object,
                            std::uint32_t showing,
                            std::uint32_t parent) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void ProcessCommand(std::uint32_t tree,
                                std::string_view command,
                                OlInfoEventWords& words) = 0;
};

class OlInfoCommandRuntimeStub : public IOlInfoCommandRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t ConstructBaseEvent() override { return 0u; }
    void DestroyBaseEvent(std::uint32_t) override {}
    void HideWindow(std::uint32_t, std::uint32_t,
                    std::uint32_t) override {}
    [[nodiscard]] std::uint32_t FrameManager() override { return 0u; }
    void ProcessCommand(std::uint32_t, std::string_view,
                        OlInfoEventWords&) override {}
};

class OlInfoCommands final {
public:
    [[nodiscard]] static std::uint8_t Close(
        std::uint32_t object_address, IOlInfoCommandRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnMemberCloseEvent(
        std::uint32_t member_address, std::uint32_t event_address,
        IOlInfoCommandRuntime& runtime);
};

}  // namespace pal4
