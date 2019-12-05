#include "http_file_downloader.h"
#include "http_request_builder.h"
#include "error.h"

#include <algorithm>
#include <thread>
#include <regex>

const std::string THttpFileDownloader::DefaultPort("80");

using TDuration = std::chrono::duration<long long, std::milli>;

void DoWithRetry(const std::function<void()>& action, const int maxTryCount, const TDuration& sleepDuration = TDuration(2000)) {
    int tryCount = 1;

    while (true) {
        try {
            action();
            break;
        } catch (const TError& error) {
            if (!error.IsNeedRetry() || tryCount >= maxTryCount) {
                throw;
            }

            std::this_thread::sleep_for(sleepDuration);
            ++tryCount;
        } catch (...) {
            throw;
        }
    }
}

THttpFileDownloader::THttpFileDownloader(const std::string& url) {
    ParseUrl(url, Host, Port, Path);

    if (Port.empty()) {
        Port = DefaultPort;
    }
}

void THttpFileDownloader::Download(const std::string& outputFilePath) {
    size_t resourceSize = 0;
    bool hasByteRange = false;

    const auto getResourceInformation = [&]() {
        EnsureConnectionIsOpened();

        const THttpResponse headResponse = HttpConnection->PerformRequest(THttpRequestBuilder::BuildHeadRequest(Host, Path), false);
        CheckResponseStatusCode(headResponse);

        hasByteRange = headResponse.HasHeaderAndValue("Accept-Ranges", "bytes");
        resourceSize = *headResponse.GetContentLength();
    };

    DoWithRetry(getResourceInformation, TryCount);

    if (resourceSize >= EnableByteRangeThresholdBytes && hasByteRange) {
        DownloadWithGetRanges(outputFilePath, resourceSize);
    } else {
        DownloadWithGetSimple(outputFilePath, resourceSize);
    }
}

void THttpFileDownloader::DownloadWithGetRanges(const std::string& outputFilePath, const size_t resourceSize) {
    OpenFileStream(outputFilePath);

    size_t nextByteToFetch = 0;
    size_t fetchedDataSize = 0;

    const auto fetchChunk = [&]() {
        if (resourceSize == 0) {
            return;
        }

        EnsureConnectionIsOpened();

        const std::string request =
                THttpRequestBuilder::BuildGetWithRangeRequest(
                    Host,
                    Path,
                    nextByteToFetch,
                    std::min(nextByteToFetch + ByteRangeChunkSizeBytes, resourceSize - 1));

        const THttpResponse response = HttpConnection->PerformRequest(request, true);
        CheckResponseStatusCode(response);

        WriteDataToFile(response.BodyRawData);

        fetchedDataSize += *response.GetContentLength();
        nextByteToFetch += ByteRangeChunkSizeBytes + 1;
    };

    while (true) {
        DoWithRetry(fetchChunk, TryCount);
        if (fetchedDataSize == resourceSize) {
            break;
        }
    }

    CloseFileStream();
}

void THttpFileDownloader::DownloadWithGetSimple(const std::string& outputFilePath, const size_t resourceSize) {
    const auto fetch = [&]() {
        OpenFileStream(outputFilePath);

        const std::string request = THttpRequestBuilder::BuildGetRequest(Host, Path);

        size_t totalBodyBytesWrited = 0;
        const THttpConnection::TBufferFilledCallback writeBodyChunk = [&](const THttpResponse& response, const size_t bufferSize) {
            WriteDataToFile(std::string_view(response.BodyRawData.data(), bufferSize));

            totalBodyBytesWrited += bufferSize;
        };

        EnsureConnectionIsOpened();
        const THttpResponse response = HttpConnection->PerformRequest(request, true, writeBodyChunk);
        CheckResponseStatusCode(response);

        if (totalBodyBytesWrited == resourceSize) {
             return;
        } else {
            throw TError("...", false);
        }

        CloseFileStream();
    };

    DoWithRetry(fetch, TryCount);
}

void THttpFileDownloader::ParseUrl(const std::string& url, std::string& host, std::string& port, std::string& path) {
    // <schema>://<host>:<port><path>
    const std::regex urlRegex("^([^:/?#]+):\/\/([^:/?#]+)(:)?([^:/?#]+)?(\/.+)$", std::regex::icase | std::regex::ECMAScript);
    std::smatch pieces_match;
    std::string schema;
    if (!std::regex_match(url, pieces_match, urlRegex)) {
        std::string errorText;
        {
            errorText.append(url);
            errorText.append(" is not a valid URI");
        }

        throw TError(errorText, false);
    }

    schema = pieces_match[1].str();
    host = pieces_match[2].str();
    port = pieces_match[4].str();
    path = pieces_match[5].str();

    if (schema != "http") {
          std::string errorText;
          {
              errorText.append(schema);
              errorText.append(" does not supported");
          }

          throw TError(errorText, false);
      }
}

void THttpFileDownloader::EnsureConnectionIsOpened() {
    if (HttpConnection && !HttpConnection->IsGood()) {
        HttpConnection.reset();
    }

    if (!HttpConnection) {
        HttpConnection = std::make_unique<THttpConnection>(Host, Port);
    }
}

void THttpFileDownloader::CheckResponseStatusCode(const THttpResponse& response) {
    if (response.StatusCode / 100 != 2) {
        std::string errorText;
        {
            errorText.append("ERROR: ");
            errorText.append(std::to_string(response.StatusCode));
            errorText.append(" ");
            errorText.append(response.StatusText);
        }

        throw TError(errorText, false);
    }
}

void THttpFileDownloader::CheckFileStream() {
    if (Stream && !Stream->good()) {
        throw TError("Unable to write to file", false);
    }
}

void THttpFileDownloader::OpenFileStream(const std::string& path) {
    if (Stream) {
        CloseFileStream();
    }

    Stream = std::make_unique<std::ofstream>(path);
    CheckFileStream();
}

void THttpFileDownloader::WriteDataToFile(const std::string_view& data) {
    CheckFileStream();
    Stream->write(data.data(), data.size());
    Stream->flush();
    CheckFileStream();
}

void THttpFileDownloader::CloseFileStream() {
    CheckFileStream();
    Stream->flush();
    Stream->close();
    CheckFileStream();

    Stream.reset();
}
