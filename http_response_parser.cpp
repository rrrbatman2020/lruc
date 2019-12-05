#include "http_response_parser.h"
#include "error.h"

#include <charconv>

void Trim(std::string_view& data, const std::string_view::value_type value) {
    size_t entryPosition = data.find_first_not_of(value);
    if (entryPosition != std::string_view::npos) {
        data.remove_prefix(entryPosition);
    }

    entryPosition = data.find_last_not_of(value);
    if (entryPosition != std::string_view::npos) {
        data.remove_suffix(data.size() - 1 - entryPosition);
    }
}

std::optional<size_t> THttpResponse::GetContentLength() const {
    const std::optional<std::string_view> headerValue = GetHeaderValue("Content-Length");
    if (headerValue) {
        return std::strtoul(headerValue->data(), nullptr, 10);
    }

    return {};
}

std::optional<std::string_view> THttpResponse::GetHeaderValue(const std::string_view& name) const {
    const std::unordered_map<std::string_view, std::string_view>::const_iterator header = Headers.find(name);
    if (header != Headers.end()) {
        return header->second;
    }

    return {};
}

bool THttpResponse::HasHeaderAndValue(const std::string_view& name, const std::string_view& value) const {
    const std::optional<std::string_view> headerValue = GetHeaderValue(name);
    if (!headerValue) {
        return false;
    }

    size_t position = 0;
    while (position < headerValue->size()) {
        std::string_view currentValue;
        const size_t entryPosition = headerValue->find_first_of(' ', position);
        if (entryPosition != std::string_view::npos) {
            currentValue = std::string_view(headerValue->data() + position, entryPosition - position + 1);
        } else {
            currentValue = std::string_view(headerValue->data() + position, headerValue->size() - position);
        }

        Trim(currentValue, ' ');

        if (currentValue == value) {
            return true;
        }

        if (entryPosition == std::string_view::npos) {
            break;
        }

        position = entryPosition + 1;
    }

    return false;
}

bool THttpResponseParser::ParseHttpResponse(const std::string_view& response, THttpResponse& result) {
    size_t position = 0;

    const std::optional<size_t> headersStartPosition = ParseStatusLine(response, result);
    if (!headersStartPosition) {
        return false;
    }

    const std::optional<size_t> bodyStartPosition = ParseHeaders(response, *headersStartPosition, result);
    if (!bodyStartPosition) {
        return false;
    }

    position = *bodyStartPosition;

    result.Data = std::string_view(response.data() + position, response.size() - position);
    return true;
}

std::optional<size_t> THttpResponseParser::ParseStatusLine(const std::string_view& data, THttpResponse& result) {
    size_t position = 0;
    size_t entryPosition = data.find_first_of(' ', position);
    if (entryPosition == std::string::npos) {
        return {};
    }

    result.Version = std::string_view(data.data() + position, entryPosition - position);

    position = entryPosition + 1;
    entryPosition = data.find_first_of(' ', position);
    if (entryPosition == std::string::npos) {
        return {};
    }

    const std::string_view code = std::string_view(data.data() + position, entryPosition - position);

    position = entryPosition + 1;
    entryPosition = data.find_first_of("\r\n", position);
    if (entryPosition == std::string::npos) {
        return {};
    }

    result.StatusText = std::string_view(data.data() + position, entryPosition - position);

    {
        const std::from_chars_result conversionResult = std::from_chars(code.data(), code.data() + code.size(), result.StatusCode);
        if (conversionResult.ec != std::errc()) {
            return {};
        }
    }

    return entryPosition + 2;
}

std::optional<size_t> THttpResponseParser::ParseHeaders(const std::string_view& data, const size_t position, THttpResponse& response) {
    size_t nextSearchPosition = position;
    size_t entryPosition = 0;
    while (true) {
        entryPosition = data.find_first_of("\r\n", nextSearchPosition);
        if (entryPosition == std::string::npos) {
            return {};
        }

        if (entryPosition == nextSearchPosition) {
            break;
        }

        const size_t separatorPosition = data.find_first_of(":", nextSearchPosition);
        if (separatorPosition == std::string::npos) {
            return {};
        }

        std::string_view name = std::string_view(data.data() + nextSearchPosition, separatorPosition - nextSearchPosition);
        std::string_view value = std::string_view(data.data() + separatorPosition + 1, entryPosition - (separatorPosition + 1));

        Trim(name, ' ');
        Trim(value, ' ');

        response.Headers.emplace(name, value);
        nextSearchPosition = entryPosition + 2;
    }

    return entryPosition + 2;
}
