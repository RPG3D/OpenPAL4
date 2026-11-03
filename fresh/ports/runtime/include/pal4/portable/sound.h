#pragma once
#include "pal4/portable/script_stack.h"
#include <optional>

namespace pal4::portable {
struct WaveInfo {std::uint32_t codec{},channels{},sample_rate{},bits{},data_bytes{};};
WaveInfo InspectWave(std::span<const std::uint8_t> bytes);
enum class SoundCommandKind {play,stop};
struct SoundCommand {
    SoundCommandKind kind{};
    std::uint64_t generation{};
    std::uint32_t handle{};
    std::string member;
    std::int32_t loops{}; // 原Miles次数：0无限，正数为总播放次数。
    std::vector<std::uint8_t> bytes;
};
using SoundReader=std::function<std::vector<std::uint8_t>(std::string_view)>;
class SoundServices final:public IScriptContextServices {
public:
    SoundServices(IScriptContextServices& next,std::uint64_t generation,SoundReader read);
    ~SoundServices();
    void OnCall(const ScriptRequest& request) override;
    ScriptResponse Invoke(const ScriptRequest& request) override;
    std::uint32_t PlayUiSound(std::string_view name,std::uint32_t repeats);
    std::optional<SoundCommand> TakeCommand();
    bool Event(std::uint64_t generation,std::uint32_t handle,std::string_view event,std::string_view error={});
    void WriteReport(std::ostream& output) const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
