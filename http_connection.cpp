#include "http_connection.h"

#include "error.h"

THttpConnection::THttpConnection(const std::string& host, const std::string& port) {
    TcpConnection = std::make_unique<TTcpConnection>(host, port);
}

THttpResponse THttpConnection::PerformRequest(
        const std::string& request,
        const bool isNeedWaitBody,
        const std::optional<TBufferFilledCallback>& processBodyChunkCallback) {
    CheckConnectionIsGood();

    SendRequest(request);
    return GetResponse(isNeedWaitBody, processBodyChunkCallback);
}

bool THttpConnection::IsGood() const {
    return Good && TcpConnection->IsGood();
}

void THttpConnection::SendRequest(const std::string& data) {
    CheckConnectionIsGood();

    TcpConnection->Send(data);
}

THttpResponse THttpConnection::GetResponse(
        const bool isNeedWaitBody,
        const std::optional<TBufferFilledCallback>& processBodyChunkCallback) {
    CheckConnectionIsGood();

    THttpResponse response;
    TryReadHead(response);

    const bool isServerClosedConnection = response.HasHeaderAndValue("Connection", "close");
    const std::optional<size_t> contentLength = response.GetContentLength();
    if (!contentLength) {
        Good = false;
        throw TError("Fetching response without Content-Length header is't supported.", false);
    }

    if (!isNeedWaitBody) {
        if (isServerClosedConnection) {
            Good = false;
        }

        return response;
    }

    TryReadBody(response, *contentLength, processBodyChunkCallback);
    if (isServerClosedConnection) {
        Good = false;
    }

    return response;
}

void THttpConnection::TryReadHead(THttpResponse& response) {
    CheckConnectionIsGood();

    std::string& buffer = response.HeadRawData;
    buffer.resize(DefaultHeadBufferSizeBytes);

    void* bufferPointer = reinterpret_cast<void*>(&buffer.front());
    int estimated = buffer.size();
    int totalPeeked = 0;

    while (true) {
        const int peeked = TcpConnection->PeekChunk(bufferPointer, estimated);
        if (peeked == 0) {
            Good = false;
            throw TError("Connection closed", true);
        }

        totalPeeked += peeked;
        estimated -= peeked;

        int needRead = peeked;
        const int headSize = TryParseHead(buffer, totalPeeked - peeked, totalPeeked, response);
        if (headSize != -1) {
            needRead = headSize - (totalPeeked - peeked);
        }

        const int read = TcpConnection->ReceiveChunk(bufferPointer, needRead);
        if (read == 0) {
            Good = false;
            throw TError("Connection closed", true);
        }

        if (headSize != -1) {
            break;
        }

        if (estimated == 0) {
            if (buffer.size() * 2 > MaxHeadSizeBytes) {
                throw TError("Head is too big", false);
            }

            estimated = buffer.size();
            buffer.resize(buffer.size() * 2);
        }

        bufferPointer = &(buffer.front()) + totalPeeked;
    }
}

void THttpConnection::TryReadBody(
        THttpResponse& response,
        const int expectedSize,
        const std::optional<TBufferFilledCallback>& processBodyChunkCallback) {
    CheckConnectionIsGood();

    const bool isPartialMode = !!processBodyChunkCallback;

    size_t bufferSize = expectedSize;
    if (isPartialMode) {
        bufferSize = PartialModeBufferSizeBytes;
    }

    std::string& result = response.BodyRawData;
    result.resize(bufferSize);

    void* bufferPointer = reinterpret_cast<void*>(&result.front());
    int estimatedSize = result.size();
    int totalReceived = 0;
    int currentBufferSize = 0;

    while (true) {
        const int received = TcpConnection->ReceiveChunk(bufferPointer, estimatedSize);

        if (received == 0) {
            Good = false;
            throw TError("Connection closed", true);
        }

        bufferPointer = static_cast<char*>(bufferPointer) + received;
        estimatedSize -= received;
        totalReceived += received;
        currentBufferSize += received;

        if (isPartialMode && (estimatedSize == 0 || totalReceived == expectedSize)) {
            (*processBodyChunkCallback)(response, currentBufferSize);

            currentBufferSize = 0;
            estimatedSize = result.size();
            bufferPointer = reinterpret_cast<void*>(&result.front());
        }

        if (totalReceived == expectedSize) {
            break;
        }
    }
}

int THttpConnection::TryParseHead(
        const std::string& data,
        const int start,
        const int end,
        THttpResponse& response) {
    // todo rnrn may overlap chunk border
    size_t position = std::string_view(data.data() + start, end - start).find("\r\n\r\n");
    if (position != std::string::npos) {
        position += start + 4;

        if (!THttpResponseParser::ParseHttpResponse(std::string_view(data.data(), position), response)) {
            throw TError("Cannot parse response", false);
        }

        return position;
    }

    return -1;
}

void THttpConnection::CheckConnectionIsGood() const {
    if (!IsGood()) {
        throw TError("Attempt to use bad connection", true);
    }
}
