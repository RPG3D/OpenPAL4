#include "pal4/portable/game_clock.h"
#include "pal4/timed_callback_collection.h"
#include "pal4/game_timer_help_window_callback.h"
#include <bit>
#include <cmath>
#include <stdexcept>
namespace pal4::portable {
struct GameClock::Impl final:pal4::ITimedCallbackCollectionRuntime,pal4::IGameTimerHelpWindowRuntime {
    static constexpr unsigned event_head=0x1000,timer_head=0x2000,node=0x3000,callback=0x4000,game=0x5000,manager=0x6000;
    unsigned elapsed{},offset{};float remaining{1};
    pal4::TimedCallbackCollectionState state{0,timer_head,1,0,event_head,0};
    unsigned ReadWord(unsigned a)override{
        if(a==event_head)return event_head;
        if(a==timer_head)return node;
        if(a==node)return timer_head;
        if(a==node+0x24)return callback;
        if(a==node+0x18)return std::bit_cast<unsigned>(remaining);
        if(a==node+0x1c)return std::bit_cast<unsigned>(1.f);
        throw std::out_of_range("game clock timer read");
    }
    void WriteWord(unsigned a,unsigned v)override{if(a!=node+0x18)throw std::out_of_range("game clock timer write");remaining=std::bit_cast<float>(v);}
    unsigned ReadDword(unsigned a)override{if(a!=game)throw std::out_of_range("game clock counter read");return elapsed;}
    void WriteDword(unsigned a,unsigned v)override{if(a!=game)throw std::out_of_range("game clock counter write");elapsed=v;}
    void InvokeTimer(unsigned c)override{if(c!=callback)throw std::invalid_argument("game clock callback");(void)pal4::GameTimerHelpWindowCallback(game).Tick(*this);}
    unsigned UiFrameManager()override{return manager;}
    unsigned FindFrame(unsigned m,std::string_view name)override{if(m!=manager||name!="gameInfo")throw std::invalid_argument("game clock info lookup");return 0;} // Presentation owner refreshes on a reported tick.
    std::uint8_t UpdateClock(unsigned)override{throw std::logic_error("game clock presentation is external");}
    std::uint8_t ReadByte(unsigned)override{throw std::logic_error("game clock timer has no owned slot release");}
    void WriteByte(unsigned,std::uint8_t)override{throw std::logic_error("game clock timer lifetime is fixed");}
    void DeleteOwned(unsigned,unsigned)override{throw std::logic_error("game clock timer lifetime is fixed");}
    void InvokeEvent(unsigned)override{throw std::logic_error("game clock has no one-shot events");}
    void CleanupEventData(unsigned)override{throw std::logic_error("game clock has no one-shot events");}
    void CleanupTimerData(unsigned)override{throw std::logic_error("game clock timer lifetime is fixed");}
    void FreeNode(unsigned)override{throw std::logic_error("game clock timer lifetime is fixed");}
    int CompareBytes(unsigned,unsigned,unsigned)override{throw std::logic_error("game clock timer name lookup not requested");}
};
GameClock::GameClock():impl_(std::make_unique<Impl>()){}GameClock::~GameClock()=default;
void GameClock::Reset(){impl_->elapsed=impl_->offset=0;impl_->remaining=1;}
bool GameClock::Advance(float delta){if(!std::isfinite(delta)||delta<0)throw std::invalid_argument("invalid game clock delta");const auto old=impl_->elapsed;pal4::TimedCallbackCollection(*impl_).Update(impl_->state,delta);return old!=impl_->elapsed;}
std::uint32_t GameClock::Elapsed()const{return impl_->elapsed;}std::uint32_t GameClock::Offset()const{return impl_->offset;}
}
