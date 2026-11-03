#include "pal4/track_point_array.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {
void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
}  // namespace

int main() {
    pal4::TrackPointArray points;
    Check(points.Size() == 0u && points.Capacity() == 0u,
          "empty 12-byte point vector");
    Check(points.InsertOne(0u, {1.0f, 2.0f, 3.0f}) == 0u &&
          points.Size() == 1u && points.Capacity() == 1u,
          "first insertion allocates one record");
    Check(points.InsertOne(1u, {4.0f, 5.0f, 6.0f}) == 1u &&
          points.Size() == 2u && points.Capacity() == 2u,
          "end insertion doubles capacity");
    Check(points.InsertOne(1u, {7.0f, 8.0f, 9.0f}) == 1u &&
          points.Size() == 3u && points.Capacity() == 4u &&
          points.At(0u) == pal4::TrackPoint{1.0f, 2.0f, 3.0f} &&
          points.At(1u) == pal4::TrackPoint{7.0f, 8.0f, 9.0f} &&
          points.At(2u) == pal4::TrackPoint{4.0f, 5.0f, 6.0f},
          "middle insertion shifts suffix across reallocation");
    Check(points.InsertOne(0u, {10.0f, 11.0f, 12.0f}) == 0u &&
          points.Size() == 4u && points.Capacity() == 4u &&
          points.At(0u)[0] == 10.0f && points.At(3u)[0] == 4.0f,
          "front insertion shifts without reallocation");
    bool caught = false;
    try { static_cast<void>(points.InsertOne(5u, {})); }
    catch (const std::out_of_range&) { caught = true; }
    Check(caught && points.Size() == 4u,
          "invalid insertion index leaves points untouched");
    Check(points.InsertOne(points.Size(), points.At(0u)) == 4u &&
          points.Size() == 5u && points.Capacity() == 8u &&
          points.At(4u) == points.At(0u),
          "insertion from owned storage survives reallocation");
    points.ClearAndRelease();
    Check(points.Size() == 0u && points.Capacity() == 0u,
          "cleanup releases owned point storage");
}
