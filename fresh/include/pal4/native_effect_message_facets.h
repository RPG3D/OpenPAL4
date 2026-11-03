#pragma once
#include <cstdint>
#include <string_view>
#include "pal4/effect_handle.h"

namespace pal4 {
// Borrowed synchronous group override. Text and handle are read-only views;
// consumers copy anything retained after the callback returns.
class INativeEffectOwnerMessages {
public:
    virtual ~INativeEffectOwnerMessages()=default;
    virtual void EffectMessage(std::uint32_t owner,
        const EffectSystemHandleWords&,std::string_view text)=0;
};
// Message facets notify the effect's parent group through slot+12. They do
// not represent the rendering queues or the game's script callback service.
class INativeEffectMessageFacets {
public:
    virtual ~INativeEffectMessageFacets()=default;
    virtual std::int32_t ReadMessageId(std::uint32_t effect,
        std::string_view key,std::int32_t fallback)=0;
    virtual void SendMessage(std::uint32_t facet,std::int32_t id,
        std::uint32_t text)=0;
};
}
