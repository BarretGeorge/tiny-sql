#include "tiny_sql/common/buffer.h"
#include <unistd.h>
#include <sys/uio.h>
#include <errno.h>

namespace tiny_sql {

ssize_t Buffer::readFromFd(int fd) {
    // 使用栈上的临时缓冲区,避免频繁扩容
    char extrabuf[65536];
    struct iovec vec[2];

    size_t writable = writableBytes();
    vec[0].iov_base = beginWrite();
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;
    ssize_t n = ::readv(fd, vec, iovcnt);

    if (n < 0) {
        return n;
    } else if (static_cast<size_t>(n) <= writable) {
        write_index_ += n;
    } else {
        write_index_ = data_.capacity();
        writeBytes(reinterpret_cast<const uint8_t*>(extrabuf), n - writable);
    }

    return n;
}

ssize_t Buffer::writeToFd(int fd) {
    ssize_t n = ::write(fd, peek(), readableBytes());
    if (n > 0) {
        read_index_ += n;
    }
    return n;
}

} // namespace tiny_sql
