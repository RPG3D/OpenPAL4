#include "support/native_test_display.h"
#include <windows.h>
#include <rpspline.h>

#include <cmath>
#include <stdexcept>
#include <string_view>
#include <type_traits>

static_assert(sizeof(void*) == 4);
static_assert(std::is_same_v<decltype(&RpSplinePluginAttach), RwBool (__cdecl*)()>);
static_assert(std::is_same_v<decltype(&RpSplineCreate),
    RpSpline* (__cdecl*)(RwInt32, RwInt32, RwV3d*)>);
static_assert(std::is_same_v<decltype(&RpSplineFindPosition),
    RwV3d* (__cdecl*)(RpSpline*, RwInt32, RwReal, RwV3d*, RwV3d*)>);

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

bool Finite(const RwV3d& point) {
    return std::isfinite(point.x) && std::isfinite(point.y) &&
           std::isfinite(point.z);
}

struct Session {
    bool initialized = false;
    bool opened = false;
    bool started = false;
    HWND window = nullptr;

    Session() {
        Check(RwEngineInit(nullptr, 0, 0) != 0, "engine init");
        initialized = true;
        Check(RpSplinePluginAttach() != 0, "spline plugin attach");
        window = pal4::test::CreateTestWindow("STATIC", "PAL4 spline boundary",
            WS_OVERLAPPEDWINDOW, 64, 64, nullptr, nullptr,
            GetModuleHandleA(nullptr), nullptr);
        Check(window != nullptr, "hidden window");
        RwEngineOpenParams params{window};
        Check(RwEngineOpen(&params) != 0, "engine open");
        opened = true;
        Check(RwEngineStart() != 0, "engine start");
        started = true;
    }

    ~Session() {
        if (started) RwEngineStop();
        if (opened) RwEngineClose();
        if (initialized) RwEngineTerm();
        if (window) DestroyWindow(window);
    }
};

struct Spline {
    RpSpline* value;
    Spline(RwInt32 type, RwV3d* points) : value(RpSplineCreate(4, type, points)) {
        Check(value != nullptr, "spline create");
    }
    ~Spline() { if (value) RpSplineDestroy(value); }
};

void Sample(RpSpline* spline, RwInt32 path, RwReal where) {
    RwV3d position{}, tangent{};
    Check(RpSplineFindPosition(spline, path, where, &position, &tangent) ==
        &position, "position return");
    Check(Finite(position) && Finite(tangent), "finite spline sample");
    const auto magnitude = tangent.x * tangent.x + tangent.y * tangent.y +
                           tangent.z * tangent.z;
    Check(magnitude > 0.8f && magnitude < 1.2f, "normalized tangent");
}

void OpenSpline() {
    Session session;
    RwV3d points[]{{0, 0, 0}, {1, 0, 0}, {2, 0, 0}, {3, 0, 0}};
    Spline spline(rpSPLINETYPEOPENLOOPBSPLINE, points);
    Sample(spline.value, rpSPLINEPATHSMOOTH, 0.0f);
    Sample(spline.value, rpSPLINEPATHSMOOTH, 0.5f);
    Sample(spline.value, rpSPLINEPATHNICEENDS, 0.5f);
    Sample(spline.value, rpSPLINEPATHNICEENDS, 1.0f);
}

void ClosedSpline() {
    Session session;
    RwV3d points[]{{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}};
    Spline spline(rpSPLINETYPECLOSEDLOOPBSPLINE, points);
    Sample(spline.value, rpSPLINEPATHSMOOTH, 0.0f);
    Sample(spline.value, rpSPLINEPATHSMOOTH, 0.5f);
    Sample(spline.value, rpSPLINEPATHNICEENDS, 0.25f);
}

}  // namespace

int main(int argc, char** argv) {
    try {
        Check(argc == 2, "expected one case");
        const std::string_view name(argv[1]);
        if (name == "open") OpenSpline();
        else if (name == "closed") ClosedSpline();
        else Check(false, "unknown case");
        return 0;
    } catch (const std::exception& error) {
        OutputDebugStringA(error.what());
        return 1;
    }
}
