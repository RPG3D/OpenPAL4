#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

class MinimapIconState final {
public:
    static constexpr std::uint32_t kBytes = 0x48u;
    [[nodiscard]] std::array<std::uint8_t, kBytes>& Bytes() noexcept {
        return bytes_;
    }
    [[nodiscard]] const std::array<std::uint8_t, kBytes>& Bytes()
        const noexcept { return bytes_; }
    [[nodiscard]] std::uint32_t ReadDword(std::uint32_t offset) const;
    void WriteDword(std::uint32_t offset, std::uint32_t value);

private:
    std::array<std::uint8_t, kBytes> bytes_{};
};

class IMinimapIconRuntime {
public:
    virtual ~IMinimapIconRuntime() = default;
    [[nodiscard]] virtual MinimapIconState& Access(
        std::uint32_t address) = 0;
    virtual void ConstructString(std::uint32_t address) = 0;
    [[nodiscard]] virtual int CleanupString(std::uint32_t address) = 0;
    virtual void InitializeString(std::uint32_t address,
                                  std::string_view value) = 0;
    [[nodiscard]] virtual int DestroyTexture(std::uint32_t texture) = 0;
    virtual void ProcessPackageScripts() = 0;
    virtual void CleanupPackageScripts() = 0;
    [[nodiscard]] virtual std::uint32_t LookupTexture(
        std::string_view path) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual int Free(std::uint32_t address) = 0;
};

class IMinimapIconDrawRuntime {
public:
    virtual ~IMinimapIconDrawRuntime() = default;
    [[nodiscard]] virtual MinimapIconState& Access(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual bool Intersects(
        std::uint32_t address,
        const std::array<std::int32_t, 4>& bounds) = 0;
    [[nodiscard]] virtual std::uint32_t IntersectionAlpha(
        std::uint32_t address,
        const std::array<std::int32_t, 4>& bounds) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void LogMissingRaster(std::uint32_t name_pointer) = 0;
    virtual void ReloadTexture(std::uint32_t address,
                               std::uint32_t name_pointer,
                               std::uint32_t property) = 0;
    virtual void DrawTransformed(
        std::uint32_t address, std::uint32_t context,
        const std::array<std::uint32_t, 4>& rectangle,
        std::uint32_t alpha) = 0;
    virtual void DrawPlain(
        std::uint32_t context,
        const std::array<std::uint32_t, 4>& rectangle,
        std::uint32_t texture,
        const std::array<std::uint8_t, 3>& rgb,
        std::uint32_t alpha) = 0;
};

class IMinimapIconFloatConverter {
public:
    virtual ~IMinimapIconFloatConverter() = default;
    [[nodiscard]] virtual std::int32_t FloatToInt(double value) = 0;
};

struct MinimapVector3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct MinimapMatrix {
    MinimapVector3 right{1.0f, 0.0f, 0.0f};
    std::uint32_t flags = 0x20003u;
    MinimapVector3 up{0.0f, 1.0f, 0.0f};
    std::uint32_t pad1 = 0u;
    MinimapVector3 at{0.0f, 0.0f, 1.0f};
    std::uint32_t pad2 = 0u;
    MinimapVector3 pos{};
    std::uint32_t pad3 = 0u;
};

struct MinimapIm2DVertex {
    float x;
    float y;
    float z;
    float reciprocal_z;
    std::uint32_t color;
    float u;
    float v;
};

class IMinimapIconTransformRuntime {
public:
    virtual ~IMinimapIconTransformRuntime() = default;
    [[nodiscard]] virtual float NearClip(std::uint32_t context) = 0;
    [[nodiscard]] virtual float VertexDepth() = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    virtual void Translate(MinimapMatrix& matrix,
                           const MinimapVector3& offset,
                           std::uint32_t combine) = 0;
    virtual void Rotate(MinimapMatrix& matrix,
                        const MinimapVector3& axis, float angle,
                        std::uint32_t combine) = 0;
    virtual void TransformPoints(std::array<MinimapVector3, 4>& points,
                                 const MinimapMatrix& matrix) = 0;
    virtual void SetRenderState(std::uint32_t state,
                                std::uint32_t value) = 0;
    virtual void DrawPrimitive(
        std::uint32_t primitive,
        const std::array<MinimapIm2DVertex, 4>& vertices) = 0;
};

class MinimapIconTransformStub final : public IMinimapIconTransformRuntime {
public:
    [[nodiscard]] float NearClip(std::uint32_t) override;
    [[nodiscard]] float VertexDepth() override;
    [[nodiscard]] std::uint32_t ReadDword(std::uint32_t) override;
    void Translate(MinimapMatrix&, const MinimapVector3&,
                   std::uint32_t) override;
    void Rotate(MinimapMatrix&, const MinimapVector3&, float,
                std::uint32_t) override;
    void TransformPoints(std::array<MinimapVector3, 4>&,
                         const MinimapMatrix&) override;
    void SetRenderState(std::uint32_t, std::uint32_t) override;
    void DrawPrimitive(
        std::uint32_t,
        const std::array<MinimapIm2DVertex, 4>&) override;
};

class MinimapIconDrawStub final : public IMinimapIconDrawRuntime {
public:
    [[nodiscard]] MinimapIconState& Access(std::uint32_t) override;
    [[nodiscard]] bool Intersects(
        std::uint32_t, const std::array<std::int32_t, 4>&) override;
    [[nodiscard]] std::uint32_t IntersectionAlpha(
        std::uint32_t, const std::array<std::int32_t, 4>&) override;
    [[nodiscard]] std::uint32_t ReadDword(std::uint32_t) override;
    void LogMissingRaster(std::uint32_t) override;
    void ReloadTexture(std::uint32_t, std::uint32_t,
                       std::uint32_t) override;
    void DrawTransformed(
        std::uint32_t, std::uint32_t,
        const std::array<std::uint32_t, 4>&,
        std::uint32_t) override;
    void DrawPlain(
        std::uint32_t, const std::array<std::uint32_t, 4>&,
        std::uint32_t, const std::array<std::uint8_t, 3>&,
        std::uint32_t) override;

private:
    MinimapIconState state_{};
};

class MinimapIconObject final {
public:
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t address, IMinimapIconRuntime& runtime);
    [[nodiscard]] static int ReleaseTexture(
        std::uint32_t address, IMinimapIconRuntime& runtime);
    [[nodiscard]] static int Destruct(
        std::uint32_t address, IMinimapIconRuntime& runtime);
    [[nodiscard]] static int Destroy(
        std::uint32_t address, IMinimapIconRuntime& runtime);
    [[nodiscard]] static bool LoadTexture(
        std::uint32_t address, std::string_view filename,
        std::uint32_t property, IMinimapIconRuntime& runtime);
    [[nodiscard]] static std::int32_t SetCenteredCoordinates(
        MinimapIconState& state, std::int32_t x, std::int32_t y);
    [[nodiscard]] static std::uint32_t CopyTriplet(
        MinimapIconState& state,
        const std::array<std::uint32_t, 3>& values);
    [[nodiscard]] static bool Draw(
        std::uint32_t address, std::uint32_t context,
        std::int32_t x, std::int32_t y,
        const std::array<std::int32_t, 4>& bounds,
        bool bypass_intersection, std::uint32_t alpha,
        IMinimapIconDrawRuntime& runtime);
    [[nodiscard]] static bool ContainsPoint(
        const MinimapIconState& state,
        std::int32_t x, std::int32_t y);
    [[nodiscard]] static bool FitsWithin(
        const MinimapIconState& state,
        const std::array<std::int32_t, 4>& bounds);
    [[nodiscard]] static std::int32_t CalculateClipAlpha(
        const MinimapIconState& state,
        const std::array<std::int32_t, 4>& bounds,
        IMinimapIconFloatConverter& converter);
    [[nodiscard]] static bool DrawTransformed(
        const MinimapIconState& state, std::uint32_t context,
        const std::array<std::uint32_t, 4>& rectangle,
        std::uint32_t alpha, IMinimapIconTransformRuntime& runtime);
};

}  // namespace pal4
