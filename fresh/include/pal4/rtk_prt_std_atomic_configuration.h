#pragma once

#include "pal4/rtk_prt_std_atomic_object.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IRtkPrtStdAtomicConfigurationRuntime {
public:
    virtual ~IRtkPrtStdAtomicConfigurationRuntime() = default;
    [[nodiscard]] virtual std::uint32_t AllocateResource(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructResource(std::uint32_t memory) = 0;
    virtual void DestroyResource(std::uint32_t resource) = 0;
    virtual void FreeResource(std::uint32_t memory) = 0;
    [[nodiscard]] virtual std::int32_t ReadInt(
        std::uint32_t document, std::string_view key,
        std::int32_t fallback) = 0;
    [[nodiscard]] virtual float ReadFloat(
        std::uint32_t document, std::string_view key, float fallback) = 0;
    [[nodiscard]] virtual std::string ReadString(
        std::uint32_t document, std::string_view key,
        std::string_view fallback) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void WriteFloat(std::uint32_t address, float value) = 0;
    virtual void UpdateFrame(std::uint32_t resource) = 0;
    [[nodiscard]] virtual std::uint32_t FindTexture(
        std::string_view path) = 0;
    [[nodiscard]] virtual std::uint32_t LoadTexture(
        std::string_view path) = 0;
    virtual void DestroyTexture(std::uint32_t texture) = 0;
    virtual void SetProperties(std::uint32_t resource,
                               bool alpha_blend) = 0;
    [[nodiscard]] virtual bool InitializeResource(
        std::uint32_t resource, bool not_point, std::uint32_t texture,
        std::uint32_t emitter_type, float size_x, float size_y,
        float size_z, bool axis_xz) = 0;
    virtual void SetCollectionState(std::uint32_t resource,
                                    bool face_point) = 0;
};

class RtkPrtStdAtomicConfigurationRuntimeStub final
    : public IRtkPrtStdAtomicConfigurationRuntime {
public:
    [[nodiscard]] std::uint32_t AllocateResource(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t ConstructResource(std::uint32_t) override { return 0u; }
    void DestroyResource(std::uint32_t) override {}
    void FreeResource(std::uint32_t) override {}
    [[nodiscard]] std::int32_t ReadInt(std::uint32_t, std::string_view,
                                       std::int32_t fallback) override { return fallback; }
    [[nodiscard]] float ReadFloat(std::uint32_t, std::string_view,
                                  float fallback) override { return fallback; }
    [[nodiscard]] std::string ReadString(std::uint32_t, std::string_view,
                                         std::string_view fallback) override {
        return std::string(fallback);
    }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    void WriteByte(std::uint32_t, std::uint8_t) override {}
    void WriteFloat(std::uint32_t, float) override {}
    void UpdateFrame(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t FindTexture(std::string_view) override { return 0u; }
    [[nodiscard]] std::uint32_t LoadTexture(std::string_view) override { return 0u; }
    void DestroyTexture(std::uint32_t) override {}
    void SetProperties(std::uint32_t, bool) override {}
    [[nodiscard]] bool InitializeResource(std::uint32_t, bool, std::uint32_t,
                                           std::uint32_t, float, float, float,
                                           bool) override { return false; }
    void SetCollectionState(std::uint32_t, bool) override {}
};

class RtkPrtStdAtomicConfiguration final {
public:
    // IDA 0x6129b0.
    [[nodiscard]] static bool Load(
        RtkPrtStdAtomicObjectState& state, std::uint32_t document,
        IRtkPrtStdAtomicConfigurationRuntime& runtime);
};

}  // namespace pal4
