#pragma once
#include "pal4/native_camera_task_orientation_math.h"
#include "pal4/native_renderware_camera_task_geometry.h"
#include "pal4/comparison_camera_euler_trace.h"
#include <array>
#include <bit>
#include <cstring>
#include <stdexcept>

namespace pal4::test {
inline void VerifyNativeCameraTaskOrientationMath(){
    const auto require=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    const auto capture=[] {return ComparisonCameraEulerTrace::CaptureControlWord();};
    // These captures are actual original 5930D0 input/output words, request
    // 193122518, PID117576. Their observed raw control word was 007F.
    // Require the native resource/game host's real mode; never change it here.
    const auto initial=capture();
    require(initial==0x007fu,"camera orientation probe requires actual original PC24 control word");
    NativeCameraTaskOrientationMath math;
    require(capture()==initial,"legacy CRT binding leaves x87 control word unchanged");
    const auto radians=math.Arcsine(std::bit_cast<float>(0x3e77ba94u));
    require(std::bit_cast<std::uint32_t>(radians)==0x3e7a3614u && capture()==initial,
        "public legacy asin preserves captured one-ULP-sensitive float result and CW");
    struct Case{
        std::array<std::uint32_t,4> input;
        std::array<std::uint32_t,3> expected;
    };
    constexpr std::array cases{
        Case{{1049366727u,3186616570u,1064583965u,1024038152u},
             {1096810546u,1107295821u,1127481322u}},
        Case{{1049645835u,3186576804u,1064543131u,1024312315u},
             {1096810571u,1107558201u,1127481327u}}};
    NativeRenderWareCameraTaskGeometry geometry;
    for(const auto& test:cases){
        const CameraTaskQuaternion q{std::bit_cast<float>(test.input[0]),std::bit_cast<float>(test.input[1]),
            std::bit_cast<float>(test.input[2]),std::bit_cast<float>(test.input[3])};
        CameraTaskVector value;
        const auto before=capture();
        require(&CameraTaskOrientation::ToEuler(q,value,math)==&value,
            "injected orientation math preserves destination identity");
        const auto after=capture();
        std::array<std::uint32_t,3> actual;
        static_assert(sizeof(value)==sizeof(actual));
        std::memcpy(actual.data(),&value,sizeof(actual));
        require(actual==test.expected && before==initial && after==before,
            "legacy math matches all three captured original Euler words without changing CW");
        const auto native=geometry.ToEuler(q);
        std::memcpy(actual.data(),&native,sizeof(actual));
        require(actual==test.expected && capture()==initial,
            "native RenderWare geometry uses injected legacy boundary and preserves CW");
    }
}
}
