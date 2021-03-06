// Copyright (c) 2015-2015 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include "SoapySocketDefs.hpp"
#include "SoapyRPCSocket.hpp"
#include "SoapyURLUtils.hpp"
#include <SoapySDR/Logger.hpp>
#include <cstring> //strerror
#include <cerrno> //errno
#include <mutex>

static std::mutex sessionMutex;
static size_t sessionCount = 0;

SoapySocketSession::SoapySocketSession(void)
{
    std::lock_guard<std::mutex> lock(sessionMutex);
    sessionCount++;
    if (sessionCount > 1) return;

    #ifdef _MSC_VER
    WORD wVersionRequested;
    WSADATA wsaData;
    wVersionRequested = MAKEWORD(2, 2);
    int ret = WSAStartup(wVersionRequested, &wsaData);
    if (ret != 0)
    {
        SoapySDR::logf(SOAPY_SDR_ERROR, "SoapySocketSession::WSAStartup: %d", ret);
    }
    #endif
}

SoapySocketSession::~SoapySocketSession(void)
{
    std::lock_guard<std::mutex> lock(sessionMutex);
    sessionCount--;
    if (sessionCount > 0) return;

    #ifdef _MSC_VER
    WSACleanup();
    #endif
}

static void defaultTcpSockOpts(int sock)
{
    if (sock == INVALID_SOCKET) return;

    int one = 1;
    int ret = ::setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&one, sizeof(one));
    if (ret != 0)
    {
        SoapySDR::logf(SOAPY_SDR_ERROR, "SoapySocketSession::setsockopt(TCP_NODELAY) -- %d", ret);
    }

    #ifdef TCP_QUICKACK
    ret = ::setsockopt(sock, IPPROTO_TCP, TCP_QUICKACK, (const char *)&one, sizeof(one));
    if (ret != 0)
    {
        SoapySDR::logf(SOAPY_SDR_ERROR, "SoapySocketSession::setsockopt(TCP_QUICKACK) -- %d", ret);
    }
    #endif //TCP_QUICKACK

    ret = ::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&one, sizeof(one));
    if (ret != 0)
    {
        SoapySDR::logf(SOAPY_SDR_ERROR, "SoapySocketSession::setsockopt(SO_REUSEADDR) -- %d", ret);
    }
}

SoapyRPCSocket::SoapyRPCSocket(void):
    _sock(INVALID_SOCKET)
{
    return;
}

SoapyRPCSocket::~SoapyRPCSocket(void)
{
    if (this->close() != 0)
    {
        SoapySDR::logf(SOAPY_SDR_ERROR, "SoapyRPCSocket::~SoapyRPCSocket: %s", this->lastErrorMsg());
    }
}

bool SoapyRPCSocket::null(void)
{
    return _sock == INVALID_SOCKET;
}

int SoapyRPCSocket::close(void)
{
    if (this->null()) return 0;
    int ret = ::closesocket(_sock);
    _sock = INVALID_SOCKET;
    return ret;
}

int SoapyRPCSocket::bind(const std::string &url)
{
    struct sockaddr_storage addr;
    int addrlen = sizeof(addr);

    const auto errorMsg = lookupURL(url, addr, addrlen);
    if (not errorMsg.empty())
    {
        SoapySDR::logf(SOAPY_SDR_ERROR, "SoapyRPCSocket::bind(%s): %s", url.c_str(), errorMsg.c_str());
        return -1;
    }

    std::string scheme, node, service;
    splitURL(url, scheme, node, service);
    const int type = (scheme == "udp")?SOCK_DGRAM:SOCK_STREAM;

    if (this->null()) _sock = ::socket(addr.ss_family, type, 0);
    if (this->null()) return -1;
    if (type == SOCK_STREAM) defaultTcpSockOpts(_sock);
    return ::bind(_sock, (const struct sockaddr*)&addr, addrlen);
}

int SoapyRPCSocket::listen(int backlog)
{
    return ::listen(_sock, backlog);
}

SoapyRPCSocket *SoapyRPCSocket::accept(void)
{
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);
    int client = ::accept(_sock, (struct sockaddr*)&addr, &addrlen);
    if (client == INVALID_SOCKET) return NULL;
    defaultTcpSockOpts(client);
    SoapyRPCSocket *clientSock = new SoapyRPCSocket();
    clientSock->_sock = client;
    return clientSock;
}

int SoapyRPCSocket::connect(const std::string &url)
{
    struct sockaddr_storage addr;
    int addrlen = sizeof(addr);

    const auto errorMsg = lookupURL(url, addr, addrlen);
    if (not errorMsg.empty())
    {
        SoapySDR::logf(SOAPY_SDR_ERROR, "SoapyRPCSocket::connect(%s): %s", url.c_str(), errorMsg.c_str());
        return -1;
    }

    std::string scheme, node, service;
    splitURL(url, scheme, node, service);
    const int type = (scheme == "udp")?SOCK_DGRAM:SOCK_STREAM;

    if (this->null()) _sock = ::socket(addr.ss_family, type, 0);
    if (this->null()) return -1;
    if (type == SOCK_STREAM) defaultTcpSockOpts(_sock);
    return ::connect(_sock, (const struct sockaddr*)&addr, addrlen);
}

int SoapyRPCSocket::send(const void *buf, size_t len, int flags)
{
    return ::send(_sock, (const char *)buf, int(len), flags);
}

int SoapyRPCSocket::recv(void *buf, size_t len, int flags)
{
    return ::recv(_sock, (char *)buf, int(len), flags);
}

bool SoapyRPCSocket::selectRecv(const long timeoutUs)
{
    struct timeval tv;
    tv.tv_sec = timeoutUs / 1000000;
    tv.tv_usec = timeoutUs % 1000000;

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(_sock, &readfds);
    return ::select(_sock+1, &readfds, NULL, NULL, &tv) == 1;
}

const char *SoapyRPCSocket::lastErrorMsg(void)
{
    #ifdef _MSC_VER
    static __declspec(thread) char buff[1024];
    int err = WSAGetLastError();
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&buff, sizeof(buff), NULL);
    return buff;
    #else
    static __thread char buff[1024];
    strerror_r(errno, buff, sizeof(buff));
    return buff;
    #endif
}

std::string SoapyRPCSocket::getsockname(void)
{
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);
    int ret = ::getsockname(_sock, (struct sockaddr *)&addr, &addrlen);
    if (ret != 0) return "";
    return sockaddrToURL(addr);
}

std::string SoapyRPCSocket::getpeername(void)
{
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);
    int ret = ::getpeername(_sock, (struct sockaddr *)&addr, &addrlen);
    if (ret != 0) return "";
    return sockaddrToURL(addr);
}

static int setBuffSizeHelper(const int sock, int level, int option_name, const size_t numBytes)
{
    int opt = int(numBytes);
    int ret = setsockopt(sock, level, option_name, (const char *)&opt, sizeof(opt));
    if (ret != 0) return ret;

    socklen_t optlen = sizeof(opt);
    ret = getsockopt(sock, level, option_name, (char *)&opt, &optlen);
    if (ret != 0) return ret;

    //adjustment for linux kernel socket buffer doubling for bookkeeping
    #ifdef __linux
    opt = opt/2;
    #endif

    return opt;
}

int SoapyRPCSocket::setRecvBuffSize(const size_t numBytes)
{
    return setBuffSizeHelper(_sock, SOL_SOCKET, SO_RCVBUF, numBytes);
}

int SoapyRPCSocket::setSendBuffSize(const size_t numBytes)
{
    return setBuffSizeHelper(_sock, SOL_SOCKET, SO_SNDBUF, numBytes);
}
