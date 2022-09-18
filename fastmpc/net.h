#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include <netinet/in.h>

namespace net {

enum class Shutdown { READ, WRITE, BOTH };

static constexpr int INVALID_FD = -1;

class Socket {
public:
  Socket(const Socket &) = delete;
  Socket &operator=(const Socket &) = delete;

  Socket(Socket &&other);
  Socket &operator=(Socket &&other);

  static Socket creat_ipv4(int type);

  Socket accept_ipv4(sockaddr_in *addr);

  int fd() { return fd_; }

  std::size_t read(void *buf, std::size_t size);

  std::size_t write(const void *buf, std::size_t size);

  void set_nonblocking(bool _nonblocking);

  template <class T> void setsockopt(int kind, T value);

  template <class T> T getsockopt(int kind);

  void shutdown(Shutdown how);

  ~Socket();

private:
  explicit Socket(int fd) : fd_(fd) {}

  int fd_;
};

class TcpStream {
public:
  static TcpStream connect(const std::string &ip, std::uint16_t port);

  std::size_t read(void *buf, std::size_t size);

  std::size_t write(const void *buf, std::size_t size);

  void shutdown(Shutdown how);

private:
  friend class TcpListener;
  explicit TcpStream(Socket &&inner) : inner_(std::move(inner)) {}

  Socket inner_;
};

class TcpListener {
public:
  static const int BackLog = 128;

  static TcpListener bind(const std::string &ip, uint16_t port);

  TcpStream accept_ipv4(std::string &ip, std::uint16_t &port);

private:
  explicit TcpListener(Socket &&inner) : inner_(std::move(inner)) {}
  Socket inner_;
};

} // namespace net