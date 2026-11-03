#include "pal4/concurrency_message_target.h"

namespace pal4 {

void ConcurrencyMessageTarget::DeclineIncomingMessages(
    ConcurrencyMessageTargetState& state) {
    state.declining = 1u;
}

}  // namespace pal4
