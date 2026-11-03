#pragma once
namespace pal4 {
// Shared public legacy CRT boundary; callers retain their actual float stores.
class NativeLegacyCrtMath final {
public:
    static void Initialize();
    [[nodiscard]] static double Asin(double);
    [[nodiscard]] static double Atan2(double y,double x);
    [[nodiscard]] static double Acos(double);
};
}
