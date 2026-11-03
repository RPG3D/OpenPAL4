#include "support/native_test_display.h"
#include <windows.h>
#include <rtintsec.h>
#include <rtnormmap.h>

#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string_view>
#include <type_traits>

static_assert(sizeof(void*) == 4);
static_assert(std::is_same_v<decltype(&RtNormMapCreateFromImage),
    RwImage* (__cdecl*)(RwImage*, RwBool, RwReal)>);
static_assert(std::is_same_v<decltype(&RtIntersectionSphereTriangle),
    RwBool (__cdecl*)(const RwSphere*, const RwV3d*, const RwV3d*,
                      const RwV3d*, RwV3d*, RwReal*)>);

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Session {
    bool initialized = false;
    bool opened = false;
    bool started = false;
    HWND window = nullptr;

    Session() {
        Check(RwEngineInit(nullptr, 0, 0) != 0, "engine init");
        initialized = true;
        window = pal4::test::CreateTestWindow("STATIC", "PAL4 toolkit boundary",
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

struct Image {
    RwImage* value;
    explicit Image(RwImage* image) : value(image) { Check(value != nullptr, "image create"); }
    ~Image() { if (value) RwImageDestroy(value); }
};

void NormalMap() {
    Session session;
    Image source(RwImageCreate(4, 4, 32));
    Check(RwImageAllocatePixels(source.value) == source.value, "pixels allocate");
    std::memset(source.value->cpPixels, 128, source.value->stride * source.value->height);
    Image result(RtNormMapCreateFromImage(source.value, TRUE, 1.0f));
    Check(result.value->width == 4 && result.value->height == 4 &&
          result.value->depth == 32 && result.value->cpPixels != nullptr,
          "normal image dimensions");
    for (int row = 0; row < result.value->height; ++row) {
        for (int column = 0; column < result.value->width; ++column) {
            Check(std::memcmp(result.value->cpPixels,
                result.value->cpPixels + row * result.value->stride + column * 4,
                4) == 0, "flat height has uniform normals");
        }
    }
}

void Intersections() {
    Session session;
    const RwV3d a{0, 0, 0}, b{2, 0, 0}, c{0, 2, 0};
    const RwSphere near_sphere{{0.5f, 0.5f, 0.2f}, 0.4f};
    const RwSphere far_sphere{{10, 10, 10}, 0.4f};
    RwV3d normal{};
    RwReal distance = -1;
    Check(RtIntersectionSphereTriangle(&near_sphere, &a, &b, &c,
        &normal, &distance) != 0, "sphere intersects triangle");
    Check(std::isfinite(distance), "finite sphere distance");
    Check(RtIntersectionSphereTriangle(&far_sphere, &a, &b, &c,
        &normal, &distance) == 0, "far sphere misses triangle");

    const RwBBox near_box{{1, 1, 1}, {-1, -1, -1}};
    const RwBBox far_box{{11, 11, 11}, {9, 9, 9}};
    Check(RtIntersectionBBoxTriangle(&near_box, &a, &b, &c) != 0,
        "box intersects triangle");
    Check(RtIntersectionBBoxTriangle(&far_box, &a, &b, &c) == 0,
        "far box misses triangle");
}

}  // namespace

int main(int argc, char** argv) {
    try {
        Check(argc == 2, "expected one case");
        const std::string_view name(argv[1]);
        if (name == "normal_map") NormalMap();
        else if (name == "intersections") Intersections();
        else Check(false, "unknown case");
        return 0;
    } catch (const std::exception& error) {
        OutputDebugStringA(error.what());
        return 1;
    }
}
