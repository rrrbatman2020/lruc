#pragma once

#include "http_response_parser.h"
#include "tcp_connection.h"

#include <functional>
#include <memory>
#include <string>

class THttpConnection {
public:
    using TBufferFilledCallback = std::function<void(const THttpResponse&, const size_t)>;

public:
    THttpConnection(const std::string& host, const std::string& port);

    THttpResponse PerformRequest(
            const std::string& request,
            const bool isNeedWaitBody,
            const std::optional<TBufferFilledCallback>& processBodyChunkCallback = std::optional<TBufferFilledCallback>());

    bool IsGood() const;

private:
    void SendRequest(const std::string& data);

    THttpResponse GetResponse(
            const bool isNeedWaitBody,
            const std::optional<TBufferFilledCallback>& processBodyChunkCallback);

    void TryReadHead(THttpResponse& response);

    void TryReadBody(
            THttpResponse& response,
            const int expectedSize,
            const std::optional<TBufferFilledCallback>& processBodyChunkCallback);

    int TryParseHead(
            const std::string& data,
            const int start,
            const int end,
            THttpResponse& response);

    void CheckConnectionIsGood() const;

private:
    std::unique_ptr<TTcpConnection> TcpConnection;
    bool Good = true;

    static const size_t DefaultHeadBufferSizeBytes = 10 * 1024;
    static const size_t MaxHeadSizeBytes = 1 * 1024 * 1024;
    static const size_t PartialModeBufferSizeBytes = 8 * 1024 * 1024;
};

