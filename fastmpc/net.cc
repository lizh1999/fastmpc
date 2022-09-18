#include "net.h"

#include <cassert>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <netinet/in.h>

#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

namespace net {

namespace detials {

sockaddr_in native_handle(const std::string &ip, std::uint16_t port) {
  int a, b, c, d;
  sscanf(ip.c_str(), "%d.%d.%d.%d", &a, &b, &c, &d);
  sockaddr_in addr;
  ::bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = a | b << 8 | c << 16 | d << 24;
  return addr;
}

} // namespace detials

Socket::Socket(Socket &&other) : fd_(other.fd_) { other.fd_ = INVALID_FD; }

Socket &Socket::operator=(Socket &&other) {
  if (this != std::addressof(other)) {
    fd_ = other.fd_;
    other.fd_ = INVALID_FD;
  }
  return *this;
}

Socket Socket::creat_ipv4(int type) {
  int fd = socket(AF_INET, type | SOCK_CLOEXEC, 0);
  return Socket(fd);
}

Socket Socket::accept_ipv4(sockaddr_in *addr) {
  socklen_t len = sizeof(sockaddr_in);
  auto *storage = reinterpret_cast<sockaddr *>(addr);
  int fd = accept4(fd_, storage, &len, SOCK_CLOEXEC);
  assert(fd != -1);
  return Socket(fd);
}

std::size_t Socket::read(void *buf, std::size_t size) {
  ssize_t ret = ::read(fd_, buf, size);
  assert(ret != -1);
  return static_cast<std::size_t>(ret);
}

std::size_t Socket::write(const void *buf, std::size_t size) {
  ssize_t ret = ::write(fd_, buf, size);
  assert(ret != -1);
  return static_cast<std::size_t>(ret);
}

void Socket::set_nonblocking(bool _nonblocking) {
  int nonblocking = static_cast<int>(_nonblocking);
  int ret = ioctl(fd_, FIONBIO, &nonblocking);
  assert(ret != -1);
}

template <class T> void Socket::setsockopt(int kind, T value) {
  int ret = ::setsockopt(fd_, SOL_SOCKET, kind, &value, sizeof(T));
  assert(ret != -1);
}

template <class T> T Socket::getsockopt(int kind) {
  T value;
  socklen_t len = sizeof(T);
  int ret = ::getsockopt(fd_, SOL_SOCKET, kind, &value, &len);
  assert(ret != -1);
  return value;
}

void Socket::shutdown(Shutdown how) {
  int ret = ::shutdown(fd_, [how] {
    switch (how) {
    case Shutdown::READ:
      return SHUT_RD;
    case Shutdown::WRITE:
      return SHUT_WR;
    case Shutdown::BOTH:
      return SHUT_RDWR;
    }
  }());
  assert(ret != -1);
}

Socket::~Socket() {
  if (fd_ != INVALID_FD) {
    int ret = close(fd_);
    assert(ret != -1);
  }
}

TcpStream TcpStream::connect(const std::string &ip, std::uint16_t port) {
  auto addr = detials::native_handle(ip, port);
  auto *addr_ptr = reinterpret_cast<sockaddr *>(&addr);
  socklen_t len = sizeof(sockaddr_in);

  while(true) {
    auto socket = Socket::creat_ipv4(SOCK_STREAM);
    using std::chrono::operator""s;
    std::this_thread::sleep_for(1s);
    if (::connect(socket.fd(), addr_ptr, len) == 0) {
      return TcpStream(std::move(socket));
    }
  }
}

std::size_t TcpStream::read(void *buf, std::size_t size) {
  return inner_.read(buf, size);
}

std::size_t TcpStream::write(const void *buf, std::size_t size) {
  return inner_.write(buf, size);
}

void TcpStream::shutdown(Shutdown how) { inner_.shutdown(how); }

TcpListener TcpListener::bind(const std::string &ip, uint16_t port) {
  auto socket = Socket::creat_ipv4(SOCK_STREAM);
  socket.setsockopt(SO_REUSEPORT, 1);

  auto addr = detials::native_handle(ip, port);
  auto *addr_ptr = reinterpret_cast<sockaddr *>(&addr);
  int ret = ::bind(socket.fd(), addr_ptr, sizeof(addr));
  assert(ret != -1);

  ret = listen(socket.fd(), BackLog);
  assert(ret != -1);

  return TcpListener(std::move(socket));
}

TcpStream TcpListener::accept_ipv4(std::string &ip, std::uint16_t &port) {
  sockaddr_in addr;
  ::bzero(&addr, sizeof(sockaddr_in));
  auto socket = inner_.accept_ipv4(&addr);
  char buf[32];

  int x = addr.sin_addr.s_addr;
  auto f = [](int x, int y) { return x >> y & 255; };
  std::sprintf(buf, "%d.%d.%d.%d", f(x, 0), f(x, 8), f(x, 16), f(x, 24));
  ip = buf;
  port = addr.sin_port;
  return TcpStream(std::move(socket));
}

} // namespace net