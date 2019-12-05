#pragma once

#include <exception>
#include <string>

class TError : public std::exception {
public:
    TError(std::string what, const bool needRetry = true);
    virtual const char* what() const noexcept override;

    bool IsNeedRetry() const;

private:
    const std::string What;
    const bool NeedRetry;
};
