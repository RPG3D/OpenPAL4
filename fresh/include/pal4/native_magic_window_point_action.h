#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <string_view>
namespace pal4 {
class NativeBaseObjectMemory;class NativeMagicWindowLayout;
class NativeMagicWindowPresentation;class NativeMagicWindowRecords;
struct PlayerManagerObjectState;
struct NativeMagicPointServices {
    std::function<void(std::string_view name,std::uint32_t trigger,float x,float y)> hint;
    std::function<void(std::string_view)> sound;
    // The gameplay owner must supply a real target-selection/cast controller.
    std::function<void(std::uint32_t magic_key)> select_magic;
};
// IDA459780 adapter: mutates actual party collections using their persistent
// allocation owner. UI, records, players and memory must outlive this adapter.
class NativeMagicWindowPointAction final {
public:
    NativeMagicWindowPointAction(NativeMagicWindowLayout&,NativeMagicWindowPresentation&,
        PlayerManagerObjectState&,NativeBaseObjectMemory&,const NativeMagicWindowRecords&,
        NativeMagicPointServices);
    ~NativeMagicWindowPointAction();
    NativeMagicWindowPointAction(const NativeMagicWindowPointAction&)=delete;
    NativeMagicWindowPointAction& operator=(const NativeMagicWindowPointAction&)=delete;
    bool Click(std::uint32_t button);
private:
    class Runtime;std::shared_ptr<Runtime> runtime_;
};
}
