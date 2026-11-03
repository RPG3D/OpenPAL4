#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "pal4/equipment_window.h"

namespace pal4 {

struct EquipmentPreviewVector {
    float x;
    float y;
    float z;
};

struct EquipmentPreviewEffectHandle {
    std::uint32_t first;
    std::uint32_t second;
};

class IEquipmentModelPreviewRuntime {
public:
    virtual ~IEquipmentModelPreviewRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string ReadText(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string ItemName(std::uint32_t item) = 0;
    [[nodiscard]] virtual std::uint32_t FindObject(std::uint32_t id) = 0;
    [[nodiscard]] virtual std::string ObjectName(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t ObjectId(std::uint32_t object) = 0;
    virtual void RemoveObject(std::uint32_t id) = 0;
    virtual void LoadItemScript(bool weapon) = 0;
    virtual void LoadFlowerScript() = 0;
    virtual void CleanupScripts() = 0;
    [[nodiscard]] virtual std::uint32_t CreateObject(
        std::string_view directory, std::string_view resource,
        std::uint32_t mode) = 0;
    virtual void SetTexture(std::uint32_t object,
                            std::string_view texture) = 0;
    virtual void SetScaleFromMember(std::uint32_t object,
                                    std::uint32_t offset) = 0;
    virtual void SetScale(std::uint32_t object,
                          EquipmentPreviewVector scale) = 0;
    virtual void SetTransform(std::uint32_t object,
                              EquipmentPreviewVector value,
                              std::uint32_t mode) = 0;
    virtual void SetPosition(std::uint32_t object,
                             EquipmentPreviewVector position) = 0;
    virtual void SetRotation(std::uint32_t object, std::uint32_t axis,
                             EquipmentPreviewVector rotation) = 0;
    [[nodiscard]] virtual EquipmentPreviewEffectHandle CreateEffect(
        std::string_view name, EquipmentPreviewVector position,
        std::int32_t parent, std::uint32_t mode) = 0;
    virtual void ResetTemporaryEffect() = 0;
    virtual void SetEffectScale(EquipmentPreviewEffectHandle handle,
                                EquipmentPreviewVector scale,
                                bool enabled) = 0;
    virtual void MarkEffectDirty(EquipmentPreviewEffectHandle handle) = 0;
    virtual void CloseEffect(EquipmentPreviewEffectHandle handle,
                             std::uint32_t event) = 0;
};

class EquipmentModelPreviewRuntimeStub : public IEquipmentModelPreviewRuntime {
public:
    std::uint32_t ReadWord(std::uint32_t) override { return 0; }
    std::string ReadText(std::uint32_t) override { return {}; }
    std::string ItemName(std::uint32_t) override { return {}; }
    std::uint32_t FindObject(std::uint32_t) override { return 0; }
    std::string ObjectName(std::uint32_t) override { return {}; }
    std::uint32_t ObjectId(std::uint32_t) override { return 0; }
    void RemoveObject(std::uint32_t) override {}
    void LoadItemScript(bool) override {}
    void LoadFlowerScript() override {}
    void CleanupScripts() override {}
    std::uint32_t CreateObject(std::string_view, std::string_view,
                               std::uint32_t) override { return 0; }
    void SetTexture(std::uint32_t, std::string_view) override {}
    void SetScaleFromMember(std::uint32_t, std::uint32_t) override {}
    void SetScale(std::uint32_t, EquipmentPreviewVector) override {}
    void SetTransform(std::uint32_t, EquipmentPreviewVector,
                      std::uint32_t) override {}
    void SetPosition(std::uint32_t, EquipmentPreviewVector) override {}
    void SetRotation(std::uint32_t, std::uint32_t,
                     EquipmentPreviewVector) override {}
    EquipmentPreviewEffectHandle CreateEffect(
        std::string_view, EquipmentPreviewVector, std::int32_t,
        std::uint32_t) override { return {}; }
    void ResetTemporaryEffect() override {}
    void SetEffectScale(EquipmentPreviewEffectHandle,
                        EquipmentPreviewVector, bool) override {}
    void MarkEffectDirty(EquipmentPreviewEffectHandle) override {}
    void CloseEffect(EquipmentPreviewEffectHandle, std::uint32_t) override {}
};

class EquipmentModelPreview final {
public:
    static void UpdateItem(EquipmentWindowState& state, std::uint32_t item,
                           IEquipmentModelPreviewRuntime& runtime);
    static void SetVisible(EquipmentWindowState& state, bool visible,
                           IEquipmentModelPreviewRuntime& runtime);
};

}  // namespace pal4
