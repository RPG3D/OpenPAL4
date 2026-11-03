#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace pal4 {

class ICameraTaskObjectRuntime {
public:
    virtual ~ICameraTaskObjectRuntime() = default;
    virtual void BaseConstruct(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t BaseDestruct(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
};

class ICameraTaskFactoryRuntime : public ICameraTaskObjectRuntime {
public:
    [[nodiscard]] virtual std::uint32_t AllocateMemory(
        std::uint32_t bytes) = 0;
    virtual void FreeMemory(std::uint32_t address) = 0;
};

class CameraTaskFactory final {
public:
    // IDA 0x594590.
    [[nodiscard]] static std::uint32_t Create(
        ICameraTaskFactoryRuntime& runtime);
};

struct CameraTaskVector {
    float x{};
    float y{};
    float z{};
};

struct CameraTaskBasis {
    CameraTaskVector right;
    CameraTaskVector up;
    CameraTaskVector forward;
};

struct CameraTaskQuaternion {
    float x{};
    float y{};
    float z{};
    float w{};
};

class ICameraTaskOrientationMath {
public:
    virtual ~ICameraTaskOrientationMath() = default;
    virtual float Arcsine(float value) = 0;
    virtual float Arctangent2(float y, float x) = 0;
};

class CameraTaskOrientation final {
public:
    // IDA 0x5930d0. Returns the same destination reference.
    [[nodiscard]] static CameraTaskVector& ToEuler(
        const CameraTaskQuaternion& quaternion,
        CameraTaskVector& destination);
    [[nodiscard]] static CameraTaskVector& ToEuler(
        const CameraTaskQuaternion& quaternion,
        CameraTaskVector& destination, ICameraTaskOrientationMath& math);
};

class ICameraTaskGeometry {
public:
    virtual ~ICameraTaskGeometry() = default;
    virtual void ClampDirection(CameraTaskVector& direction) = 0;
    virtual void Normalize(CameraTaskVector& vector) = 0;
    [[nodiscard]] virtual CameraTaskQuaternion ToQuaternion(
        const CameraTaskBasis& basis) = 0;
    [[nodiscard]] virtual CameraTaskVector ToEuler(
        const CameraTaskQuaternion& quaternion) = 0;
};

// Immediate records do not use geometry; interpolated records require an adapter.
class CameraTaskGeometryStub final : public ICameraTaskGeometry {
public:
    void ClampDirection(CameraTaskVector& direction) override;
    void Normalize(CameraTaskVector& vector) override;
    [[nodiscard]] CameraTaskQuaternion ToQuaternion(
        const CameraTaskBasis& basis) override;
    [[nodiscard]] CameraTaskVector ToEuler(
        const CameraTaskQuaternion& quaternion) override;
};

class ICameraTaskPlaybackRuntime : public ICameraTaskObjectRuntime {
public:
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t FindXmlChild(
        std::uint32_t document, std::string_view name) = 0;
    [[nodiscard]] virtual std::optional<float> ConfigFloat(
        std::uint32_t child, std::string_view name) = 0;
    virtual void SetLookAt(std::uint32_t camera,
                           CameraTaskVector position,
                           std::uint32_t flag) = 0;
    virtual void SetSecondAngle(std::uint32_t camera, float angle,
                                std::uint32_t flag) = 0;
    virtual void SetAngle(std::uint32_t camera, float angle,
                          std::uint32_t flag) = 0;
    virtual void SetThirdAngle(std::uint32_t camera, float angle,
                               std::uint32_t flag) = 0;
    virtual void SetScalar(std::uint32_t camera, float scalar,
                           std::uint32_t flag) = 0;
    [[nodiscard]] virtual float InterpolateFloat(
        float start, float end, float fraction,
        std::uint32_t mode) = 0;
    virtual void SetPosition(std::uint32_t camera,
                             CameraTaskVector position,
                             std::uint32_t flag) = 0;
    virtual void SetEye(std::uint32_t camera,
                        CameraTaskVector position,
                        std::uint32_t flag) = 0;
};

class CameraTaskObject final {
public:
    // IDA 0x592600/0x592650/0x592660.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, ICameraTaskObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t object, ICameraTaskObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t Configure(
        std::uint32_t object, std::uint32_t property,
        std::uint32_t camera_record, ICameraTaskObjectRuntime& runtime);
    // IDA 0x592680.
    [[nodiscard]] static std::uint8_t Enter(
        std::uint32_t object, ICameraTaskPlaybackRuntime& runtime,
        ICameraTaskGeometry& geometry);
    // IDA 0x592f50. Only the low return byte is stable in assembly.
    [[nodiscard]] static std::uint8_t Update(
        std::uint32_t object, float delta,
        ICameraTaskPlaybackRuntime& runtime);
};

}  // namespace pal4
