#pragma once

#include <array>
#include <cstddef>
#include <vector>

namespace pal4 {

using TrackPoint = std::array<float, 3u>;

// Structured replacement for the native begin/end/capacity triplet of
// 12-byte point records. InsertOne reconstructs IDA 0x626270.
class TrackPointArray final {
public:
    [[nodiscard]] std::size_t Size() const { return points_.size(); }
    [[nodiscard]] std::size_t Capacity() const { return points_.capacity(); }
    [[nodiscard]] const TrackPoint& At(std::size_t index) const {
        return points_.at(index);
    }
    [[nodiscard]] TrackPoint& At(std::size_t index) {
        return points_.at(index);
    }
    [[nodiscard]] std::vector<TrackPoint>& MutablePoints() { return points_; }
    [[nodiscard]] const std::vector<TrackPoint>& Points() const {
        return points_;
    }
    [[nodiscard]] std::size_t InsertOne(std::size_t index,
                                        const TrackPoint& point);
    void ClearAndRelease();

private:
    std::vector<TrackPoint> points_;
};

}  // namespace pal4
