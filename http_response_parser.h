#pragma once

#include <optional>
#include <string>
#include <unordered_map>

class THttpResponse {
public:
    std::optional<size_t> GetContentLength() const;
    std::optional<std::string_view> GetHeaderValue(const std::string_view& name) const;
    bool HasHeaderAndValue(const std::string_view& name, const std::string_view& value) const;

public:
    std::unordered_map<std::string_view, std::string_view> Headers;
    std::string_view StatusText;
    std::string_view Data;
    std::string_view Version;

    int StatusCode = 0;

    std::string HeadRawData;
    std::string BodyRawData;
};

class THttpResponseParser {
public:
    static bool ParseHttpResponse(const std::string_view& response, THttpResponse& result);

private:
    static std::optional<size_t> ParseStatusLine(const std::string_view& data, THttpResponse& response);
    static std::optional<size_t> ParseHeaders(const std::string_view& data, const size_t position, THttpResponse& response);
};
