#pragma once

#include <string>

class TTcpConnection {
public:
    TTcpConnection(const std::string& host, const std::string& port);
    ~TTcpConnection();

    void Send(const std::string& data);
    int ReceiveChunk(void* result, const int estimatedSize);
    int PeekChunk(void* result, const int estimatedSize);

    bool IsEstablished() const;
    bool IsGood() const;

private:
    void Establish(const std::string& host, const std::string& port);
    void Close();

    void CheckConnectionIsGood() const;

private:
    int SocketDecriptor;
    bool Good = true;
};

