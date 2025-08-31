#pragma once

#include <stdexcept>
#include <string>
#include <utility>
#include <cstdint>
#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
#else
  #include <arpa/inet.h>
  #include <netinet/in.h>
  #include <sys/socket.h>
  #include <unistd.h>
#endif

// Test networking helpers for picking a free localhost port.
// Note: This is a best-effort helper. To truly reserve the port, keep the
// socket open until the server successfully starts. For most local CI cases,
// picking and starting immediately is sufficient.
namespace testnet {

// Binds a socket to 127.0.0.1:0 (ephemeral) to discover a free port.
// Closes the socket and returns the discovered port number.
// Throws std::runtime_error on failure.
inline uint16_t pick_free_port_ipv4() {
#ifdef _WIN32
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
    throw std::runtime_error("WSAStartup failed");
  }
  SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
  if (s == INVALID_SOCKET) {
    WSACleanup();
    throw std::runtime_error("socket() failed");
  }
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1
  addr.sin_port = htons(0); // ephemeral
  if (bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
    closesocket(s);
    WSACleanup();
    throw std::runtime_error("bind() failed");
  }
  int addrlen = sizeof(addr);
  if (getsockname(s, reinterpret_cast<sockaddr*>(&addr), &addrlen) == SOCKET_ERROR) {
    closesocket(s);
    WSACleanup();
    throw std::runtime_error("getsockname() failed");
  }
  uint16_t port = ntohs(addr.sin_port);
  closesocket(s);
  WSACleanup();
  return port;
#else
  int s = ::socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0) {
    throw std::runtime_error("socket() failed");
  }
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1
  addr.sin_port = htons(0); // ephemeral
  if (::bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
    ::close(s);
    throw std::runtime_error("bind() failed");
  }
  socklen_t addrlen = sizeof(addr);
  if (::getsockname(s, reinterpret_cast<sockaddr*>(&addr), &addrlen) < 0) {
    ::close(s);
    throw std::runtime_error("getsockname() failed");
  }
  uint16_t port = ntohs(addr.sin_port);
  ::close(s);
  return port;
#endif
}

} // namespace testnet
