#pragma once

#include "pal4/property_window_lifecycle.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IPropertyWindowDiscardRuntime {
public:
    virtual ~IPropertyWindowDiscardRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string ReadText(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CreateCommonDialog() = 0;
    virtual void InitializeBuffer() = 0;
    [[nodiscard]] virtual std::uint32_t TriggerTextAddress(
        std::uint32_t trigger) = 0;
    [[nodiscard]] virtual std::string FormatBuffer(
        std::string_view format, std::string_view argument) = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        std::string_view text) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ObjectAddress(
        const PropertyWindowState& state) = 0;
    virtual void ShowWithFlags(std::uint32_t dialog, std::string_view text,
                               float x, float y, std::uint32_t flags,
                               std::uint32_t callback) = 0;
    virtual void ReleaseCallback(std::uint32_t callback,
                                 std::uint32_t flag) = 0;
    virtual void CleanupBuffer() = 0;
    [[nodiscard]] virtual std::uint32_t HoveredRecord(
        PropertyWindowState& state) = 0;
    virtual void PlayMusic(std::string_view file,
                           std::uint32_t mode) = 0;
    virtual void RemoveItem(std::uint32_t id,
                            std::uint32_t count) = 0;
    virtual void PopulateList(PropertyWindowState& state,
                              std::uint32_t category,
                              bool preserve_scroll) = 0;
};

class PropertyWindowDiscard final {
public:
    static constexpr std::uint32_t kEmptyTextAddress = 0x841964u;
    static constexpr std::uint32_t kConfirmationCallback = 0x473ae0u;
    static constexpr std::uint32_t kCallbackVtable = 0x843a6cu;

    [[nodiscard]] static bool Show(PropertyWindowState& state,
                                   std::uint32_t event,
                                   IPropertyWindowDiscardRuntime& runtime);
    [[nodiscard]] static bool Confirm(
        PropertyWindowState& state, std::uint32_t choice,
        IPropertyWindowDiscardRuntime& runtime);
};

}  // namespace pal4
