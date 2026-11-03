#include "pal4/status_check_handler.h"

namespace pal4 {

StatusCheckHandler::StatusCheckHandler(const int conditional_argument,
                                       const int result_argument,
                                       IStatusCheckContext& context) noexcept
    : result_argument_(result_argument),
      conditional_argument_(conditional_argument),
      context_(context) {}

int StatusCheckHandler::HandleStatusCheck() const {
    if (context_.IsConditionalCallbackEnabled()) {
        context_.InvokeConditionalCallback(conditional_argument_);
    }
    return context_.InvokeResultCallback(result_argument_);
}

}  // namespace pal4
