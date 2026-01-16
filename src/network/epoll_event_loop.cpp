#ifdef __linux__

#include "tiny_sql/network/epoll_event_loop.h"
#include "tiny_sql/common/logger.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <cstring>
#include <errno.h>

namespace tiny_sql {

EpollEventLoop::EpollEventLoop(int max_events)
    : epoll_fd_(-1), max_events_(max_events), ready_count_(0) {
    events_.resize(max_events_);
}

EpollEventLoop::~EpollEventLoop() {
    close();
}

bool EpollEventLoop::init() {
    epoll_fd_ = ::epoll_create1(0);
    if (epoll_fd_ < 0) {
        LOG_ERROR("Failed to create epoll: " << strerror(errno));
        return false;
    }
    LOG_DEBUG("Epoll event loop initialized (fd=" << epoll_fd_ << ")");
    return true;
}

bool EpollEventLoop::addFd(int fd, uint32_t events) {
    struct epoll_event ev;
    std::memset(&ev, 0, sizeof(ev));
    ev.events = toEpollEvents(events);
    ev.data.fd = fd;

    if (::epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) < 0) {
        LOG_ERROR("epoll_ctl ADD failed for fd " << fd << ": " << strerror(errno));
        return false;
    }

    LOG_DEBUG("Added fd " << fd << " to epoll with events " << events);
    return true;
}

bool EpollEventLoop::modifyFd(int fd, uint32_t events) {
    struct epoll_event ev;
    std::memset(&ev, 0, sizeof(ev));
    ev.events = toEpollEvents(events);
    ev.data.fd = fd;

    if (::epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev) < 0) {
        LOG_ERROR("epoll_ctl MOD failed for fd " << fd << ": " << strerror(errno));
        return false;
    }

    LOG_DEBUG("Modified fd " << fd << " in epoll with events " << events);
    return true;
}

bool EpollEventLoop::removeFd(int fd) {
    if (::epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr) < 0) {
        LOG_ERROR("epoll_ctl DEL failed for fd " << fd << ": " << strerror(errno));
        return false;
    }

    LOG_DEBUG("Removed fd " << fd << " from epoll");
    return true;
}

int EpollEventLoop::wait(int timeout) {
    ready_count_ = ::epoll_wait(epoll_fd_, events_.data(), max_events_, timeout);

    if (ready_count_ < 0) {
        if (errno == EINTR) {
            return 0;  // 被信号中断，不是错误
        }
        LOG_ERROR("epoll_wait failed: " << strerror(errno));
        return -1;
    }

    return ready_count_;
}

int EpollEventLoop::getReadyFd(int index) const {
    if (index < 0 || index >= ready_count_) {
        return -1;
    }
    return events_[index].data.fd;
}

uint32_t EpollEventLoop::getReadyEvents(int index) const {
    if (index < 0 || index >= ready_count_) {
        return 0;
    }
    return fromEpollEvents(events_[index].events);
}

void EpollEventLoop::close() {
    if (epoll_fd_ >= 0) {
        ::close(epoll_fd_);
        epoll_fd_ = -1;
        LOG_DEBUG("Epoll event loop closed");
    }
}

uint32_t EpollEventLoop::toEpollEvents(uint32_t events) const {
    uint32_t epoll_events = 0;

    if (events & static_cast<uint32_t>(EventType::READ)) {
        epoll_events |= EPOLLIN;
    }
    if (events & static_cast<uint32_t>(EventType::WRITE)) {
        epoll_events |= EPOLLOUT;
    }

    // 边缘触发模式
    epoll_events |= EPOLLET;

    return epoll_events;
}

uint32_t EpollEventLoop::fromEpollEvents(uint32_t epoll_events) const {
    uint32_t events = 0;

    if (epoll_events & EPOLLIN) {
        events |= static_cast<uint32_t>(EventType::READ);
    }
    if (epoll_events & EPOLLOUT) {
        events |= static_cast<uint32_t>(EventType::WRITE);
    }
    if (epoll_events & (EPOLLERR | EPOLLHUP)) {
        events |= static_cast<uint32_t>(EventType::ERROR);
        events |= static_cast<uint32_t>(EventType::CLOSE);
    }

    return events;
}

} // namespace tiny_sql

#endif // __linux__
