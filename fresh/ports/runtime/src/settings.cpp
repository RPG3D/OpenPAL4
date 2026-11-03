#include "pal4/portable/settings.h"
#include "pal4/movie_flag_value.h"
#include "pal4/script_pal_test_result.h"
#include <bit>
#include <algorithm>
#include <fstream>
#include <stdexcept>

namespace pal4::portable {
namespace {
void Word(std::span<std::uint8_t> bytes,std::uint32_t value) {
    for(unsigned i=0;i<4;++i)bytes[i]=static_cast<std::uint8_t>(value>>(8*i));
}
std::uint32_t Word(std::span<const std::uint8_t> bytes) {
    std::uint32_t value{};for(unsigned i=0;i<4;++i)value|=std::uint32_t(bytes[i])<<(8*i);return value;
}
std::uint32_t Crc(std::span<const std::uint8_t> bytes) {
    std::uint32_t crc=0xffffffffu;
    for(const auto byte:bytes) {
        crc^=byte;
        for(unsigned bit=0;bit<8;++bit)crc=(crc>>1)^((crc&1)?0xedb88320u:0u);
    }
    return ~crc;
}
}
bool Settings::HelpSeen(std::uint32_t id) const {
    return id<256 && (help_seen[id/32] & (1u<<(id%32)))!=0;
}
std::array<std::uint8_t,64> EncodeSettings(const Settings& settings) {
    std::array<std::uint8_t,64> bytes{'P','4','S','T'};
    Word(std::span(bytes).subspan(4,4),4);Word(std::span(bytes).subspan(8,4),settings.movie_flags);
    Word(std::span(bytes).subspan(12,4),settings.tutorials_enabled?1u:0u);
    for(unsigned i=0;i<3;++i)Word(std::span(bytes).subspan(16+i*4,4),settings.pal_test_results[i]);
    for(unsigned i=0;i<8;++i)Word(std::span(bytes).subspan(28+i*4,4),settings.help_seen[i]);
    Word(std::span(bytes).subspan(60,4),Crc(std::span(bytes).first(60)));return bytes;
}
Settings DecodeSettings(std::span<const std::uint8_t> bytes) {
    constexpr std::array<std::uint8_t,4> magic{'P','4','S','T'};
    if(bytes.size()<8 || !std::equal(magic.begin(),magic.end(),bytes.begin()))throw std::runtime_error("invalid portable settings header/length");
    const auto version=Word(bytes.subspan(4,4));
    if((version<1 || version>4) || bytes.size()!=(version==1?16u:version==2?20u:version==3?32u:64u))throw std::runtime_error("unsupported portable settings version/length");
    const auto payload=bytes.size()-4;
    if(Word(bytes.subspan(payload,4))!=Crc(bytes.first(payload)))throw std::runtime_error("portable settings checksum mismatch");
    Settings result{Word(bytes.subspan(8,4))};
    if(version>=2){const auto enabled=Word(bytes.subspan(12,4));if(enabled>1)throw std::runtime_error("invalid tutorial option");result.tutorials_enabled=enabled!=0;}
    if(version>=3)for(unsigned i=0;i<3;++i)result.pal_test_results[i]=Word(bytes.subspan(16+i*4,4));
    if(version>=4)for(unsigned i=0;i<8;++i)result.help_seen[i]=Word(bytes.subspan(28+i*4,4));
    return result;
}
Settings FileSettingsStore::Load() {
    auto committed=path_;auto backup=path_;backup+=".bak";
    if(!std::filesystem::exists(committed)){
        if(!std::filesystem::exists(backup))return {};
        committed=backup;
    }
    std::ifstream file(committed,std::ios::binary);if(!file)throw std::runtime_error("cannot open portable settings");
    std::array<std::uint8_t,65> bytes{};file.read(reinterpret_cast<char*>(bytes.data()),bytes.size());
    if(file.bad())throw std::runtime_error("cannot read portable settings");
    return DecodeSettings(std::span(bytes).first(static_cast<std::size_t>(file.gcount())));
}
void FileSettingsStore::Save(const Settings& settings) {
    auto temporary=path_;temporary+=".tmp";const auto bytes=EncodeSettings(settings);
    {
        std::ofstream file(temporary,std::ios::binary|std::ios::trunc);
        if(!file)throw std::runtime_error("cannot stage portable settings");
        file.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());file.flush();file.close();
        if(!file)throw std::runtime_error("cannot flush portable settings");
    }
    auto backup=path_;backup+=".bak";
    if(std::filesystem::exists(path_)){
        // FAT/libnx不能覆盖已有文件；保留可恢复的旧提交再移动新文件。
        std::filesystem::remove(backup);
        std::filesystem::rename(path_,backup);
    }
    try{std::filesystem::rename(temporary,path_);}
    catch(...){
        if(!std::filesystem::exists(path_) && std::filesystem::exists(backup)){
            std::error_code restore_error;std::filesystem::rename(backup,path_,restore_error);
        }
        throw;
    }
    // 新文件已经提交，清理旧备份失败不能把成功误报为未提交。
    std::error_code cleanup_error;std::filesystem::remove(backup,cleanup_error);
}
void SettingsServices::SaveTutorial(std::uint32_t key) {
    // 原5CE210仅在有效的128位教程索引范围触发设置保存；已读位留在游戏owner。
    if(key>=128)return;
    store_.Save(state_);++writes_;
}
void SettingsServices::SetTutorialsEnabled(bool enabled) {
    auto pending=state_;pending.tutorials_enabled=enabled;store_.Save(pending);state_=pending;++writes_;
}
bool SettingsServices::MarkHelpSeen(std::uint32_t id) {
    if(id>=256 || state_.HelpSeen(id))return false;
    auto pending=state_;pending.help_seen[id/32]|=1u<<(id%32);
    store_.Save(pending);state_=pending;++writes_;return true;
}
ScriptResponse SettingsServices::Invoke(const ScriptRequest& request) {
    const auto& call=request.call;
    if(call.system_id==220) {
        if(call.arguments.size()!=1 || !std::holds_alternative<std::int32_t>(call.arguments[0]))
            throw std::invalid_argument("invalid PalTest result arguments");
        // 数值句柄仅适配原 getter 的偏移协议，不存储或截断宿主指针。
        struct Runtime final:pal4::IScriptPalTestResultRuntime {
            const Settings& state;
            explicit Runtime(const Settings& value):state(value){}
            std::uint32_t GlobalData() override{return 0x100;}
            std::uint32_t ReadWord(std::uint32_t address) override {
                if(address<0x11c || address>0x124 || (address-0x11c)%4)
                    throw std::logic_error("invalid PalTest result address");
                return state.pal_test_results.at((address-0x11c)/4);
            }
        } runtime(state_);
        return {std::bit_cast<std::int32_t>(pal4::ScriptPalTestResult::Get(std::get<std::int32_t>(call.arguments[0]),runtime)),false};
    }
    if(call.system_id!=110)return next_.Invoke(request);
    if(call.arguments.size()!=1)throw std::invalid_argument("invalid movie flag arguments");
    const auto flags=pal4::OpenMovieFlagValue(state_.movie_flags,static_cast<std::uint32_t>(std::get<std::int32_t>(call.arguments[0])));
    if(!flags)return {};
    auto next=state_;next.movie_flags=*flags;store_.Save(next);state_=next;++writes_;return {};
}
} // namespace pal4::portable
