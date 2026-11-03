#pragma once

#include <cstdint>

namespace pal4 {

class ICameraClumpIntersectionRuntime {
public:
    virtual ~ICameraClumpIntersectionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void CalculatePixelRay(std::uint32_t camera,
                                   std::uint32_t record,
                                   std::uint32_t pixel) = 0;
    [[nodiscard]] virtual std::uint32_t ForEachAtomic(
        std::uint32_t clump, std::uint32_t record) = 0;
    [[nodiscard]] virtual std::uint32_t ForEachIntersection(
        std::uint32_t atomic, std::uint32_t record,
        std::uint32_t distance) = 0;
};

class CameraClumpIntersectionRuntimeStub final
    : public ICameraClumpIntersectionRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    void CalculatePixelRay(std::uint32_t, std::uint32_t,
                           std::uint32_t) override {}
    [[nodiscard]] std::uint32_t ForEachAtomic(std::uint32_t,
                                               std::uint32_t) override {
        return 0u;
    }
    [[nodiscard]] std::uint32_t ForEachIntersection(
        std::uint32_t, std::uint32_t, std::uint32_t) override { return 0u; }
};

class CameraClumpIntersection final {
public:
    // IDA 0x5f9f30.
    [[nodiscard]] static std::uint32_t FindNearestAtomic(
        std::uint32_t record, std::uint32_t camera, std::uint32_t clump,
        std::uint32_t pixel, ICameraClumpIntersectionRuntime& runtime);
    // IDA 0x5f9f90.
    [[nodiscard]] static std::uint32_t ProcessAtomic(
        std::uint32_t atomic, std::uint32_t record,
        ICameraClumpIntersectionRuntime& runtime);
    // IDA 0x5f9fe0. Native callback returns its second argument.
    [[nodiscard]] static std::uint32_t UpdateDistance(
        std::uint32_t intersection, std::uint32_t triangle, float distance,
        std::uint32_t nearest_distance,
        ICameraClumpIntersectionRuntime& runtime);
};

}  // namespace pal4
