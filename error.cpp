#include "error.h"

TError::TError(std::string what, const bool needRetry)
    : What(std::move(what))
    , NeedRetry(needRetry)
{
}

const char* TError::what() const noexcept {
    return What.data();
}

bool TError::IsNeedRetry() const {
    return NeedRetry;
}
