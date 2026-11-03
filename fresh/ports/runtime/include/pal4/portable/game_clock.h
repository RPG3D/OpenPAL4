#pragma once
#include <cstdint>
#include <memory>
namespace pal4::portable {
// Original1-second UI timer; advanced by application/UI frame delta.
class GameClock final {
public:
    GameClock();~GameClock();
    GameClock(const GameClock&)=delete;GameClock& operator=(const GameClock&)=delete;
    void Reset();bool Advance(float delta);
    std::uint32_t Elapsed()const;std::uint32_t Offset()const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
