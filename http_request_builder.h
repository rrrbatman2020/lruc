#pragma once

#include <string>

class THttpRequestBuilder {
public:
    static std::string BuildGetRequest(const std::string& host, const std::string& path);
    static std::string BuildHeadRequest(const std::string& host, const std::string& path);
    static std::string BuildGetWithRangeRequest(const std::string& host, const std::string& path, const int firstRangeByte, const int lastRangeByte);

private:
    static void AddKeepAlive(std::string& request);
    static void AddRequestLine(const std::string& requestType, const std::string& path, std::string& request);
    static void AddHost(const std::string& host, std::string& request);
};
