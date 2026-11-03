#include "pal4/camera_task_object.h"

#include <cmath>

namespace pal4 {
namespace {
class HostOrientationMath final : public ICameraTaskOrientationMath {
public:
    float Arcsine(float value) override {
        volatile float result = static_cast<float>(std::asin(static_cast<double>(value)));
        return result;
    }
    float Arctangent2(float y, float x) override {
        volatile float result = static_cast<float>(std::atan2(static_cast<double>(y), static_cast<double>(x)));
        return result;
    }
};
}


CameraTaskVector& CameraTaskOrientation::ToEuler(
    const CameraTaskQuaternion& quaternion,
    CameraTaskVector& destination) {
    HostOrientationMath math;
    return ToEuler(quaternion, destination, math);
}

CameraTaskVector& CameraTaskOrientation::ToEuler(
    const CameraTaskQuaternion& quaternion,
    CameraTaskVector& destination, ICameraTaskOrientationMath& math) {
    const float x = -quaternion.x;
    const float y = -quaternion.y;
    const float z = -quaternion.z;
    const float w = quaternion.w;
    // IDA5930F4..59310C: each x87 operation rounds under PC24 before
    // the separate CRT asin boundary switches to its internal PC53 precision.
    const float zy = z * y;
    const float xw = x * w;
    const float sum = zy + xw;
    const float sine = sum * -2.0f;
    const float stored_sine = sine;
    if (std::fabs(sine) <= 0.9998999834060669) {
        destination.x = math.Arcsine(stored_sine);
        // Original PC24 arithmetic rounds each product and subtraction before
        // FPATAN; promoting these operands to double changes actual camera bits.
        const float xx = x * x;
        const float half = 0.5f - xx;
        const float zx = z * x;
        const float yw = y * w;
        const float yy = y * y;
        const float yaw_numerator = zx - yw;
        const float yaw_denominator = half - yy;
        destination.y = math.Arctangent2(yaw_numerator, yaw_denominator);
        const float yx = y * x;
        const float zw = z * w;
        const float zz = z * z;
        const float roll_numerator = yx - zw;
        const float roll_denominator = half - zz;
        destination.z = math.Arctangent2(roll_numerator, roll_denominator);
    } else {
        destination.z = 0.0f;
        destination.x = stored_sine * 1.570796012878418f;
        const float negative_zx = -z * x;
        const float yw = y * w;
        const float yy = y * y;
        const float zz = z * z;
        const float numerator = negative_zx - yw;
        const float half = 0.5f - yy;
        const float denominator = half - zz;
        destination.y = math.Arctangent2(numerator, denominator);
    }
    constexpr float degrees_per_radian = 57.295780181884766f;
    destination.x *= degrees_per_radian;
    destination.y *= degrees_per_radian;
    destination.z *= degrees_per_radian;
    return destination;
}

}  // namespace pal4
