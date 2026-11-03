#include "pal4/combat_event_log_entry.h"
#include <array>
#include <iomanip>
#include <sstream>
namespace pal4 {
std::string FormatCombatLogEntry(const CombatLogTime& t,std::uint32_t level,
    std::string_view message,std::optional<std::string_view> detail) {
    constexpr std::array<std::string_view,4> labels{
        "(Error)\t","(InfL1)\t","(InfL2)\t","(InfL3)\t"};
    std::ostringstream entry;
    entry << std::setfill('0') << std::setw(2) << t.day << '/'
          << std::setw(2) << t.month_zero_based+1u << '/'
          << std::setw(4) << t.year_since_1900+1900u << ' '
          << std::setw(2) << t.hour << ':' << std::setw(2) << t.minute << ':'
          << std::setw(2) << t.second << ' '
          << (level<labels.size() ? labels[level] : std::string_view{"(Unkwn)\t"}) << message;
    if(detail) entry << '\t' << *detail;
    entry << '\n';
    return entry.str();
}
}
