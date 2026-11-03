#include "pal4/track_point_array.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace pal4 {

std::size_t TrackPointArray::InsertOne(std::size_t index,
                                       const TrackPoint& point) {
    if (index > points_.size())
        throw std::out_of_range("track point insertion index");
    const TrackPoint value = point;
    if (points_.size() == points_.capacity())
        points_.reserve(points_.size() +
                        std::max(points_.size(), std::size_t{1u}));
    points_.insert(points_.begin() + static_cast<std::ptrdiff_t>(index), value);
    return index;
}

void TrackPointArray::ClearAndRelease() {
    std::vector<TrackPoint> empty;
    points_.swap(empty);
}

}  // namespace pal4
