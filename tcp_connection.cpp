#include "tcp_connection.h"
#include "error.h"

#include <arpa/inet.h>
#include <cstring>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

TTcpConnection::TTcpConnection(const std::string& host, const std::string& port) {
    Establish(host, port);
}

TTcpConnection::~TTcpConnection() {
    Close();
}

void TTcpConnection::Send(const std::string& data) {
    CheckConnectionIsGood();

    int totalBytesSended = 0;
    while (totalBytesSended < data.size()) {
        const int bytesSended = send(SocketDecriptor, data.c_str(), data.size(), 0);
        if (bytesSended < 0) {
            Good = false;
            throw TError("Cannot send data", true);
        }

        totalBytesSended += bytesSended;
    }
}

int TTcpConnection::ReceiveChunk(void* result, const int estimatedSize) {
    CheckConnectionIsGood();

    const int bytesReceived = recv(SocketDecriptor, result, estimatedSize, 0);
    if (bytesReceived < 0) {
        Good = false;
        throw TError("Cannot receive data", true);
    }

    return bytesReceived;
}

int TTcpConnection::PeekChunk(void* result, const int estimatedSize) {
    CheckConnectionIsGood();

    const int bytesReceived = recv(SocketDecriptor, result, estimatedSize, MSG_PEEK);
    if (bytesReceived < 0) {
        Good = false;
        throw TError("Cannot peek data", true);
    }

    return bytesReceived;
}

bool TTcpConnection::IsGood() const {
    return Good;
}

void TTcpConnection::Establish(const std::string& host, const std::string& port) {
    struct addrinfo* result = nullptr;
    try {
        struct addrinfo addressHints;
        {
            memset(&addressHints, 0, sizeof(struct addrinfo));
            addressHints.ai_family = AF_UNSPEC;
            addressHints.ai_socktype = SOCK_STREAM;
            addressHints.ai_flags = 0;
            addressHints.ai_protocol = IPPROTO_TCP;
        }

        const int getAddrInfoResult = getaddrinfo(host.c_str(), port.c_str(), &addressHints, &result);
        if (getAddrInfoResult != 0) {
            std::string errorText;
            {
                errorText.append("getaddrinfo: ");
                errorText.append(gai_strerror(getAddrInfoResult));
            }

            throw TError(errorText, false);
        }

        struct addrinfo* address;
        for (address = result; address; address = address->ai_next) {
            SocketDecriptor = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
            if (SocketDecriptor == -1) {
                continue;
            }

            if (connect(SocketDecriptor, address->ai_addr, address->ai_addrlen) != -1) {
                break;
            }

            close(SocketDecriptor);
        }

        if (!address) {
            throw TError("Could not connect", false);
        }
    } catch (...) {
        if (result) {
            freeaddrinfo(result);
        }

        throw;
    }
}

void TTcpConnection::Close() {
    close(SocketDecriptor);
    Good = false;
}

void TTcpConnection::CheckConnectionIsGood() const {
    if (!IsGood()) {
        throw TError("Attempt to use bad connection", true);
    }
}

