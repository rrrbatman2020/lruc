#include "http_request_builder.h"

std::string THttpRequestBuilder::BuildGetRequest(const std::string& host, const std::string& path) {
    std::string data;
    {
        AddRequestLine("GET", path, data);
        AddHost(host, data);
        AddKeepAlive(data);

        data.append("\r\n");
    }

    return data;
}

std::string THttpRequestBuilder::BuildHeadRequest(const std::string& host, const std::string& path) {
    std::string data;
    {
        AddRequestLine("HEAD", path, data);
        AddHost(host, data);
        AddKeepAlive(data);

        data.append("\r\n");
    }

    return data;
}

std::string THttpRequestBuilder::BuildGetWithRangeRequest(const std::string& host, const std::string& path, const int firstRangeByte, const int lastRangeByte) {
    std::string data;
    {
        AddRequestLine("GET", path, data);
        AddHost(host, data);
        AddKeepAlive(data);

        data.append("Range: bytes=");
        data.append(std::to_string(firstRangeByte));
        data.append("-");
        data.append(std::to_string(lastRangeByte));
        data.append("\r\n");
        data.append("\r\n");
    }

    return data;
}

void THttpRequestBuilder::AddKeepAlive(std::string& request) {
    request.append("Connection: keep-alive");
    request.append("\r\n");
    request.append("Keep-Alive: timeout=60, max=6000");
    request.append("\r\n");
}

void THttpRequestBuilder::AddRequestLine(const std::string& requestType, const std::string& path, std::string& request) {
    request.append(requestType);
    request.append(" ");
    request.append(path);
    request.append(" HTTP/1.1");
    request.append("\r\n");
}

void THttpRequestBuilder::AddHost(const std::string& host, std::string& request) {
    request.append("Host: ");
    request.append(host);
    request.append("\r\n");
}
