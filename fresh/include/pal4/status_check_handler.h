#pragma once

namespace pal4 {

class IStatusCheckContext {
public:
    virtual ~IStatusCheckContext() = default;

    [[nodiscard]] virtual bool IsConditionalCallbackEnabled() const = 0;
    virtual void InvokeConditionalCallback(int argument) = 0;
    [[nodiscard]] virtual int InvokeResultCallback(int argument) = 0;
};

class StatusCheckHandler final {
public:
    StatusCheckHandler(int conditional_argument, int result_argument,
                       IStatusCheckContext& context) noexcept;

    [[nodiscard]] int HandleStatusCheck() const;

private:
    int result_argument_;
    int conditional_argument_;
    IStatusCheckContext& context_;
};

}  // namespace pal4
