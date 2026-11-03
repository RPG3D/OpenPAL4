#pragma once

#include "pal4/load_window_lifecycle.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

struct LoadWindowAttributePosition {
    std::uint32_t current{};
    std::uint32_t end{};
};

class ILoadWindowSaveSelectionRuntime {
public:
    virtual ~ILoadWindowSaveSelectionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t HoverItem(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t GlobalData() = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ScrollbarOf(
        std::uint32_t list) = 0;
    virtual void SaveDialogSettings(std::uint32_t global_data) = 0;
    virtual void HideWindow(LoadWindowState& state,
                            std::uint32_t show,
                            std::uint32_t parent) = 0;
    virtual void ProcessUiEvents(std::string_view command,
                                 std::uint32_t first,
                                 std::uint32_t second) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeXmlDocument() = 0;
    [[nodiscard]] virtual std::uint32_t InitializeKey(
        const char* name, std::uint8_t slot_low_byte) = 0;
    [[nodiscard]] virtual LoadWindowAttributePosition FindAttribute(
        std::uint32_t document, std::uint32_t key) = 0;
    virtual void ReleaseKey(std::uint32_t key) = 0;
    virtual void SetAttributeInt(std::uint32_t value_field,
                                 std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeAttribute(
        const char* name, std::uint32_t value) = 0;
    virtual void AddAttribute(std::uint32_t document,
                              std::uint32_t attribute) = 0;
    virtual void DestroyAttribute(std::uint32_t attribute) = 0;
    virtual void EnterGameState(std::uint32_t state_id,
                                std::uint32_t document) = 0;
    virtual void DestroyXmlDocument(std::uint32_t document) = 0;
};

class LoadWindowSaveSelectionRuntimeStub :
    public ILoadWindowSaveSelectionRuntime {
public:
    [[nodiscard]] std::uint32_t HoverItem(std::uint32_t) override {
        return 0u;
    }
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override {
        return 0u;
    }
    [[nodiscard]] std::uint32_t GlobalData() override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t ScrollbarOf(std::uint32_t) override {
        return 0u;
    }
    void SaveDialogSettings(std::uint32_t) override {}
    void HideWindow(LoadWindowState&, std::uint32_t,
                    std::uint32_t) override {}
    void ProcessUiEvents(std::string_view, std::uint32_t,
                         std::uint32_t) override {}
    [[nodiscard]] std::uint32_t InitializeXmlDocument() override {
        return 0u;
    }
    [[nodiscard]] std::uint32_t InitializeKey(
        const char*, std::uint8_t) override { return 0u; }
    [[nodiscard]] LoadWindowAttributePosition FindAttribute(
        std::uint32_t, std::uint32_t) override { return {}; }
    void ReleaseKey(std::uint32_t) override {}
    void SetAttributeInt(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t InitializeAttribute(
        const char*, std::uint32_t) override { return 0u; }
    void AddAttribute(std::uint32_t, std::uint32_t) override {}
    void DestroyAttribute(std::uint32_t) override {}
    void EnterGameState(std::uint32_t, std::uint32_t) override {}
    void DestroyXmlDocument(std::uint32_t) override {}
};

class LoadWindowSaveSelection final {
public:
    [[nodiscard]] static std::uint8_t Confirm(
        LoadWindowState& state, std::uint32_t response,
        ILoadWindowSaveSelectionRuntime& runtime);
};

}  // namespace pal4
