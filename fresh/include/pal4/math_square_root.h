#pragma once

namespace pal4 {

class IMathSquareRootRuntime {
public:
    virtual ~IMathSquareRootRuntime() = default;
    [[nodiscard]] virtual double ApproximateSquareRoot(double value) = 0;
};

class MathSquareRootRuntimeStub final : public IMathSquareRootRuntime {
public:
    [[nodiscard]] double ApproximateSquareRoot(double) override {
        return 0.0;
    }
};

class MathSquareRoot final {
public:
    // IDA 0x601c30.
    [[nodiscard]] static double Approximate(
        float value, IMathSquareRootRuntime& runtime);
    // IDA 0x601c50, thunk to SSE sqrtss.
    [[nodiscard]] static double Sse(float value);
    // IDA 0x601c60.
    [[nodiscard]] static double ReciprocalApproximate(
        float value, IMathSquareRootRuntime& runtime);
    // IDA 0x601c80, thunk to SSE rsqrtss.
    [[nodiscard]] static double ReciprocalSse(float value);
};

}  // namespace pal4
