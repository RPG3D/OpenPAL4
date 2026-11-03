#pragma once

#include "pal4/clump_object.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IClumpObjectConfigRuntime {
public:
    virtual ~IClumpObjectConfigRuntime() = default;
    [[nodiscard]] virtual std::uint32_t SystemInstance() = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::int32_t GetInt(
        std::uint32_t document, std::string_view key,
        std::int32_t fallback) = 0;
    [[nodiscard]] virtual float GetFloat(
        std::uint32_t document, std::string_view key,
        float fallback) = 0;
    [[nodiscard]] virtual std::string GetString(
        std::uint32_t document, std::string_view key,
        std::string_view fallback) = 0;

    [[nodiscard]] virtual std::uint32_t CreateManager(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual bool LoadClump(
        std::uint32_t manager, std::string_view name,
        std::string_view directory, std::uint32_t world,
        std::uint32_t first_flag, std::uint32_t second_flag,
        std::uint32_t third_flag) = 0;
    virtual void DestroyManager(std::uint32_t manager,
                                std::uint32_t deletion_flag) = 0;
    [[nodiscard]] virtual std::uint32_t ReadAnimation(
        std::string_view path) = 0;
    [[nodiscard]] virtual std::uint32_t FindHierarchy(
        std::uint32_t clump) = 0;
    // Reset60A7C0 uses the first Atomic's bound skin hierarchy (410270),
    // whereas initial configuration traverses frame children.
    [[nodiscard]] virtual std::uint32_t FindSkinHierarchy(std::uint32_t clump) = 0;
    virtual void AttachHierarchy(std::uint32_t hierarchy) = 0;
    virtual void SetSkinHierarchy(std::uint32_t clump,
                                  std::uint32_t hierarchy) = 0;
    virtual void SetCurrentAnimation(std::uint32_t interpolator,
                                     std::uint32_t animation) = 0;
    virtual void SetCurrentTime(std::uint32_t interpolator,
                                float time) = 0;
    virtual void UpdateHierarchyMatrices(std::uint32_t hierarchy) = 0;

    virtual void Rotate(std::uint32_t manager,
                        const std::array<float, 3>& rotation,
                        std::uint32_t mode) = 0;
    virtual void Scale(std::uint32_t manager,
                       const std::array<float, 3>& scale,
                       std::uint32_t mode) = 0;
    virtual void Translate(std::uint32_t manager,
                           const std::array<float, 3>& position,
                           std::uint32_t mode) = 0;
    virtual void SetColor(std::uint32_t manager,
                          std::uint32_t rgba) = 0;
    virtual void CleanupTexture(std::uint32_t manager) = 0;
    virtual void SetTexture(std::uint32_t manager,
                            std::uint32_t texture) = 0;
    virtual void LoadDualTexture(std::uint32_t manager,
                                 std::string_view texture,
                                 std::uint32_t first,
                                 std::uint32_t second) = 0;
    virtual void LoadReflectionAndNormal(
        std::uint32_t manager, std::string_view normal,
        std::string_view reflection, std::uint32_t first,
        std::uint32_t second, std::uint32_t third,
        float reflection_coefficient, float normal_coefficient) = 0;
    virtual void LoadReflection(std::uint32_t manager,
                                std::string_view texture,
                                float coefficient,
                                std::uint32_t flag) = 0;
    virtual void LoadNormal(std::uint32_t manager,
                            std::string_view texture,
                            float coefficient) = 0;
    virtual void AssignXml(ClumpObjectState& state,
                           std::uint32_t offset,
                           std::uint32_t document) = 0;
};

class ClumpObjectConfigRuntimeStub : public IClumpObjectConfigRuntime {
public:
    [[nodiscard]] std::uint32_t SystemInstance() override { return 0u; }
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::int32_t GetInt(std::uint32_t, std::string_view,
                                      std::int32_t fallback) override {
        return fallback;
    }
    [[nodiscard]] float GetFloat(std::uint32_t, std::string_view,
                                 float fallback) override {
        return fallback;
    }
    [[nodiscard]] std::string GetString(std::uint32_t, std::string_view,
                                        std::string_view fallback) override {
        return std::string(fallback);
    }
    [[nodiscard]] std::uint32_t CreateManager(std::uint32_t) override {
        return 0u;
    }
    [[nodiscard]] bool LoadClump(std::uint32_t, std::string_view,
                                 std::string_view, std::uint32_t,
                                 std::uint32_t, std::uint32_t,
                                 std::uint32_t) override { return false; }
    void DestroyManager(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t ReadAnimation(std::string_view) override {
        return 0u;
    }
    [[nodiscard]] std::uint32_t FindHierarchy(std::uint32_t) override {
        return 0u;
    }
    [[nodiscard]] std::uint32_t FindSkinHierarchy(std::uint32_t) override {return 0u;}
    void AttachHierarchy(std::uint32_t) override {}
    void SetSkinHierarchy(std::uint32_t, std::uint32_t) override {}
    void SetCurrentAnimation(std::uint32_t, std::uint32_t) override {}
    void SetCurrentTime(std::uint32_t, float) override {}
    void UpdateHierarchyMatrices(std::uint32_t) override {}
    void Rotate(std::uint32_t, const std::array<float, 3>&,
                std::uint32_t) override {}
    void Scale(std::uint32_t, const std::array<float, 3>&,
               std::uint32_t) override {}
    void Translate(std::uint32_t, const std::array<float, 3>&,
                   std::uint32_t) override {}
    void SetColor(std::uint32_t, std::uint32_t) override {}
    void CleanupTexture(std::uint32_t) override {}
    void SetTexture(std::uint32_t, std::uint32_t) override {}
    void LoadDualTexture(std::uint32_t, std::string_view,
                         std::uint32_t, std::uint32_t) override {}
    void LoadReflectionAndNormal(std::uint32_t, std::string_view,
                                 std::string_view, std::uint32_t,
                                 std::uint32_t, std::uint32_t,
                                 float, float) override {}
    void LoadReflection(std::uint32_t, std::string_view,
                        float, std::uint32_t) override {}
    void LoadNormal(std::uint32_t, std::string_view,
                    float) override {}
    void AssignXml(ClumpObjectState&, std::uint32_t,
                   std::uint32_t) override {}
};

}  // namespace pal4
