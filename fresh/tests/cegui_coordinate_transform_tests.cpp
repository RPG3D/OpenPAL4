#include "pal4/cegui_system_initializer.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void GeneralScaleAndInverse() {
    pal4::CeguiSystemState state;
    state.WriteFloat(0x14u, 0.5f);
    state.WriteFloat(0x18u, 0.25f);
    state.WriteFloat(0x1cu, 10.0f);
    const auto ui = state.TransformMouseCoordinates({100.0f, 200.0f});
    Check(ui[0] == 55.0f && ui[1] == 50.0f,
          "mouse transform offsets X before scaling and scales Y directly");
    const auto screen = state.TransformUiCoordinates(ui);
    Check(screen[0] == 100.0f && screen[1] == 200.0f,
          "inverse divides by independent scales then removes X offset");
}

void SpecialDisplayOffsetAndFractionalPrecision() {
    pal4::CeguiSystemState state;
    state.WriteFloat(0x14u, 0.75f);
    state.WriteFloat(0x18u, 0.75f);
    state.WriteFloat(0x1cu, -106.66669f);
    const auto ui = state.TransformMouseCoordinates({200.0f, 400.0f});
    Check(std::fabs(ui[0] - 70.0f) < 0.001f && ui[1] == 300.0f,
          "1280 by 800 branch shifts X before three-quarter scaling");
    const auto screen = state.TransformUiCoordinates(ui);
    Check(std::fabs(screen[0] - 200.0f) < 0.001f &&
              std::fabs(screen[1] - 400.0f) < 0.001f,
          "special offset survives inverse transform within float precision");
}

void ZeroScaleRetainsFloatingPointDivision() {
    pal4::CeguiSystemState state;
    state.WriteFloat(0x14u, 0.0f);
    state.WriteFloat(0x18u, 2.0f);
    state.WriteFloat(0x1cu, 0.0f);
    const auto point = state.TransformUiCoordinates({1.0f, 4.0f});
    Check(std::isinf(point[0]) && point[1] == 2.0f,
          "zero X scale is not silently clamped");
}

}  // namespace

int main() {
    GeneralScaleAndInverse();
    SpecialDisplayOffsetAndFractionalPrecision();
    ZeroScaleRetainsFloatingPointDivision();
    std::cout << "3 CEGUI coordinate transform test cases passed\n";
}
