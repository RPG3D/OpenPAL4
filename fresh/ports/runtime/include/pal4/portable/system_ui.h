#pragma once
#include <array>
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <optional>
namespace pal4::portable {
// Shared game data, rather than a display-only copy of the permission flags.
// Uninitialized entries are explicit until the corresponding startup/save
// service has supplied them; no guessed global defaults enter script logic.
class GameDataEntries {
public:
    void InitializeSceneUiDefaults();
    void Set(std::uint32_t index,std::uint32_t value);
    std::optional<std::uint32_t> Get(std::uint32_t index) const;
    void WriteReport(std::ostream& output) const;
private:std::array<std::optional<std::uint32_t>,84> entries_{};
};
class SystemUiPermissions {
public:
    explicit SystemUiPermissions(GameDataEntries& data);
    ~SystemUiPermissions();
    SystemUiPermissions(const SystemUiPermissions&)=delete;
    SystemUiPermissions& operator=(const SystemUiPermissions&)=delete;
    void Grant(std::int32_t index,bool enabled);
    void GrantMagic(bool first,bool second);
    void WriteReport(std::ostream& output) const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
