#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "pal4/ol_info_lifecycle.h"

namespace pal4 {

class IOlInfoVisibilityRuntime {
public:
    virtual ~IOlInfoVisibilityRuntime() = default;
    virtual void SetParent(OlInfoState& state, std::uint8_t showing,
                           std::uint32_t parent) = 0;
    [[nodiscard]] virtual std::string_view TriggerText(
        std::uint32_t id) = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        std::string_view text) = 0;
    [[nodiscard]] virtual std::uint32_t CreateCeguiString(
        std::string_view text) = 0;
    virtual void SetText(std::uint32_t window,
                         std::uint32_t text) = 0;
    virtual void DestroyCeguiString(std::uint32_t text) = 0;
    virtual void SetWindowVisible(std::uint32_t window,
                                  bool visible) = 0;
    [[nodiscard]] virtual std::int32_t ProgressCount() = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t InsertActive(
        std::uint32_t array, std::uint32_t member) = 0;
    [[nodiscard]] virtual std::uint32_t RemoveActive(
        std::uint32_t array, std::uint32_t member) = 0;
};

class OlInfoVisibilityRuntimeStub : public IOlInfoVisibilityRuntime {
public:
    void SetParent(OlInfoState&, std::uint8_t,
                   std::uint32_t) override {}
    [[nodiscard]] std::string_view TriggerText(
        std::uint32_t) override { return {}; }
    [[nodiscard]] std::string ConvertToUtf8(
        std::string_view text) override { return std::string(text); }
    [[nodiscard]] std::uint32_t CreateCeguiString(
        std::string_view) override { return 0u; }
    void SetText(std::uint32_t, std::uint32_t) override {}
    void DestroyCeguiString(std::uint32_t) override {}
    void SetWindowVisible(std::uint32_t, bool) override {}
    [[nodiscard]] std::int32_t ProgressCount() override { return 0; }
    [[nodiscard]] std::uint32_t FrameManager() override { return 0u; }
    [[nodiscard]] std::uint32_t InsertActive(
        std::uint32_t, std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t RemoveActive(
        std::uint32_t, std::uint32_t) override { return 0u; }
};

class OlInfoVisibility final {
public:
    [[nodiscard]] static std::uint32_t SetVisible(
        OlInfoState& state, std::uint32_t object_address,
        std::uint8_t showing, std::uint32_t parent,
        IOlInfoVisibilityRuntime& runtime);
};

}  // namespace pal4
