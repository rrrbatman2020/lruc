#pragma once

#include "http_connection.h"

#include <memory>
#include <string>
#include <fstream>

class THttpFileDownloader {
public:
    THttpFileDownloader(const std::string& url);

    void Download(const std::string& outputFilePath);

private:
    void DownloadWithGetRanges(const std::string& outputFilePath, const size_t resourceSize);
    void DownloadWithGetSimple(const std::string& outputFilePath, const size_t resourceSize);

    void ParseUrl(const std::string& url, std::string& host, std::string& port, std::string& path);

    void EnsureConnectionIsOpened();

    void CheckResponseStatusCode(const THttpResponse& response);

    void CheckFileStream();
    void OpenFileStream(const std::string& path);
    void WriteDataToFile(const std::string_view& data);
    void CloseFileStream();

private:
    std::string Host;
    std::string Port;
    std::string Path;

    std::unique_ptr<THttpConnection> HttpConnection;
    std::unique_ptr<std::ofstream> Stream;

    static const std::string DefaultPort;
    static const size_t EnableByteRangeThresholdBytes = 32 * 1024 * 1024;
    static const size_t ByteRangeChunkSizeBytes = 8 * 1024 * 1024;
    static const size_t TryCount = 5;
};
