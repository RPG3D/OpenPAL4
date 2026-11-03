#pragma once
#include <cstdint>
#include <cstddef>
#include <iosfwd>
#include <memory>
#include <string_view>
namespace pal4 {struct PlayerPositionWaitState;}
namespace pal4::portable {
// Owns names and records shared by position/rotation adapters. The original
// 32-bit fields contain checked tokens, never truncated host addresses.
class PlayerNotifications {
public:
    static constexpr std::uint32_t executor=0x50000,queue=executor+0x54;
    PlayerNotifications();
    ~PlayerNotifications();
    PlayerNotifications(const PlayerNotifications&)=delete;
    PlayerNotifications& operator=(const PlayerNotifications&)=delete;
    pal4::PlayerPositionWaitState* Allocate(std::uint32_t size);
    void Construct(pal4::PlayerPositionWaitState& state,std::uint32_t mode,std::uint32_t queue);
    bool AllocateText(pal4::PlayerPositionWaitState& state,std::uint32_t size);
    void AssignName(pal4::PlayerPositionWaitState& state,std::string_view name,std::uint32_t phase,std::uint32_t vtable);
    void Close(pal4::PlayerPositionWaitState& state);
    void Add(std::uint32_t queue,pal4::PlayerPositionWaitState* state);
    void Process();
    std::size_t Count(std::uint32_t phase) const;
    void WriteReport(std::ostream& output) const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
