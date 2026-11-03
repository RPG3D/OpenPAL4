#pragma once
#include "pal4/comparison_camera_euler_trace.h"
#include <filesystem>
#include <fstream>
#include <locale>
#include <stdexcept>
#include <string>
#include <string_view>
#include <windows.h>

namespace pal4 {
// Called on the game thread by the observation message; never converts bits to
// floats, invokes game callbacks, clears the trace, or changes the FPU mode.
inline void PublishComparisonCameraEulerTrace(std::string_view side, std::uint32_t request) {
    if ((side != "original" && side != "native") || !request)
        throw std::invalid_argument("camera Euler trace result identity");
    std::array<wchar_t,32768> root{};
    const auto length = GetEnvironmentVariableW(L"PAL4_COMPARE_STATE_DIR",root.data(),static_cast<DWORD>(root.size()));
    if (!length || length >= root.size()) throw std::runtime_error("camera Euler trace output directory is absent");
    const auto directory = std::filesystem::path(root.data());
    if (!directory.is_absolute()) throw std::runtime_error("camera Euler trace output directory must be absolute");
    const auto path = directory / ("compare-" + std::string(side) + "-camera-euler-" +
        std::to_string(GetCurrentProcessId()) + "-" + std::to_string(request) + ".json");
    const auto temporary = std::filesystem::path(path.wstring() + L".tmp");
    const auto& trace = GetComparisonCameraEulerTrace();
    {
        std::ofstream out(temporary,std::ios::binary|std::ios::trunc);
        out.imbue(std::locale::classic());
        out << "{\"pid\":" << GetCurrentProcessId() << ",\"request\":" << request
            << ",\"enabled\":" << (trace.Enabled()?"true":"false")
            << ",\"capacity\":" << ComparisonCameraEulerTrace::kCapacity
            << ",\"count\":" << trace.Count() << ",\"sequence\":" << trace.Sequence()
            << ",\"dropped\":" << trace.Dropped() << ",\"entries\":[";
        for (std::uint32_t i=0; i<trace.Count(); ++i) {
            const auto& entry = trace.Entries()[i];
            if (i) out << ',';
            out << "{\"sequence\":" << entry.sequence << ",\"input\":[";
            for (unsigned axis=0; axis<4; ++axis) { if(axis)out<<','; out<<entry.input[axis]; }
            out << "],\"output\":[";
            for (unsigned axis=0; axis<3; ++axis) { if(axis)out<<','; out<<entry.output[axis]; }
            out << "],\"cw_before\":" << entry.cw_before << ",\"cw_after\":" << entry.cw_after << '}';
        }
        out << "]}\n"; out.flush();
        if (!out) throw std::runtime_error("camera Euler trace result write failed");
    }
    if (!MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_WRITE_THROUGH))
        throw std::runtime_error("camera Euler trace result publication failed");
}
}
