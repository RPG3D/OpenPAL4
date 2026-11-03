#include "pal4/d3d_line2.h"

#include "pal4/d3d_vector2_operations.h"

namespace pal4 {

void D3dLine2::ClearNormalCache(D3dLine2State& line) {
    line.normal_cached = 0;
}

D3dLine2State& D3dLine2::SetEndpoints(
    D3dLine2State& line, const D3dTriangleVector2& start,
    const D3dTriangleVector2& end) {
    D3dVector2Operations::Copy(line.start, start);
    D3dVector2Operations::Copy(line.end, end);
    ClearNormalCache(line);
    return line;
}

D3dTriangleVector2& D3dLine2::SetStart(
    D3dLine2State& line, const D3dTriangleVector2& start) {
    D3dVector2Operations::Copy(line.start, start);
    ClearNormalCache(line);
    return line.start;
}

D3dTriangleVector2& D3dLine2::SetEnd(
    D3dLine2State& line, const D3dTriangleVector2& end) {
    D3dVector2Operations::Copy(line.end, end);
    ClearNormalCache(line);
    return line.end;
}

D3dTriangleVector2& D3dLine2::Normal(D3dLine2State& line) {
    if (line.normal_cached == 0) {
        ComputeNormal(line);
    }
    return line.normal;
}

D3dTriangleVector2& D3dLine2::Reinitialize(
    D3dLine2State& line, const D3dTriangleVector2& start,
    const D3dTriangleVector2& end) {
    D3dVector2Operations::Copy(line.start, start);
    auto& result = D3dVector2Operations::Copy(line.end, end);
    ClearNormalCache(line);
    return result;
}

double D3dLine2::SignedDistance(D3dLine2State& line,
                                const D3dTriangleVector2& point) {
    Normal(line);
    D3dTriangleVector2 offset{};
    D3dVector2Operations::Subtract(offset, point, line.start);
    return D3dVector2Operations::Dot(offset, line.normal);
}

int D3dLine2::Classify(D3dLine2State& line,
                       const D3dTriangleVector2& point,
                       const float tolerance) {
    const auto distance = SignedDistance(line, point);
    if (distance > tolerance) return 2;
    if (distance < -static_cast<double>(tolerance)) return 1;
    return 0;
}

int D3dLine2::Intersect(const D3dLine2State& first,
                        const D3dLine2State& second,
                        D3dTriangleVector2* intersection) {
    const float start_delta_x = first.start.x - second.start.x;
    const float start_delta_z = first.start.z - second.start.z;
    const float first_delta_x = first.end.x - first.start.x;
    const float first_delta_z = first.end.z - first.start.z;
    const float second_delta_x = second.end.x - second.start.x;
    const float second_delta_z = second.end.z - second.start.z;

    const double first_numerator =
        static_cast<double>(second_delta_x) * start_delta_z -
        static_cast<double>(second_delta_z) * start_delta_x;
    const double denominator =
        static_cast<double>(first_delta_x) * second_delta_z -
        static_cast<double>(first_delta_z) * second_delta_x;
    if (denominator == 0.0) {
        return first_numerator == 0.0 ? 0 : 5;
    }

    const float inverse = static_cast<float>(1.0 / denominator);
    const float first_parameter =
        static_cast<float>(inverse * first_numerator);
    const float second_parameter = static_cast<float>(
        (static_cast<double>(first_delta_x) * start_delta_z -
         static_cast<double>(first_delta_z) * start_delta_x) * inverse);

    if (intersection != nullptr) {
        intersection->x = static_cast<float>(
            static_cast<double>(first_parameter) * first_delta_x +
            first.start.x);
        intersection->z = static_cast<float>(
            static_cast<double>(first_parameter) * first_delta_z +
            first.start.z);
    }

    const bool first_inside = first_parameter >= 0.0f &&
                              first_parameter <= 1.0f;
    const bool second_inside = second_parameter >= 0.0f &&
                               second_parameter <= 1.0f;
    if (first_inside && second_inside) return 2;
    if (second_inside) return 3;
    if (first_inside) return 4;
    return 1;
}

D3dTriangleVector2& D3dLine2::End(D3dLine2State& line) {
    return line.end;
}

D3dTriangleVector2& D3dLine2::ComputeUnitDirection(
    D3dLine2State& line, D3dTriangleVector2& output) {
    D3dTriangleVector2 difference{};
    D3dVector2Operations::Subtract(difference, line.end, line.start);
    D3dVector2Operations::Copy(output, difference);
    return D3dVector2Operations::NormalizeInPlace(output);
}

D3dTriangleVector2& D3dLine2::ComputeNormal(D3dLine2State& line) {
    ComputeUnitDirection(line, line.normal);
    const D3dTriangleVector2 direction = line.normal;
    line.normal = {direction.z, -direction.x};
    line.normal_cached = 1;
    return line.normal;
}

}  // namespace pal4
