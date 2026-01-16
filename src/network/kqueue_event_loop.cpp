#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)

#include "tiny_sql/network/kqueue_event_loop.h"
#include "tiny_sql/common/logger.h"
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>
#include <cstring>
#include <errno.h>

namespace tiny_sql {

KqueueEventLoop::KqueueEventLoop(int max_events)
    : kqueue_fd_(-1), max_events_(max_events), ready_count_(0) {
    events_.resize(max_events_);
}

KqueueEventLoop::~KqueueEventLoop() {
    close();
}

bool KqueueEventLoop::init() {
    kqueue_fd_ = ::kqueue();
    if (kqueue_fd_ < 0) {
        LOG_ERROR("Failed to create kqueue: " << strerror(errno));
        return false;
    }
    LOG_DEBUG("Kqueue event loop initialized (fd=" << kqueue_fd_ << ")");
    return true;
}

bool KqueueEventLoop::addFd(int fd, uint32_t events) {
    if (!updateEvents(fd, events, true)) {
        return false;
    }
    fd_events_[fd] = events;
    LOG_DEBUG("Added fd " << fd << " to kqueue with events " << events);
    return true;
}

bool KqueueEventLoop::modifyFd(int fd, uint32_t events) {
    auto it = fd_events_.find(fd);
    if (it == fd_events_.end()) {
        LOG_ERROR("Cannot modify fd " << fd << " that is not in kqueue");
        return false;
    }

    uint32_t old_events = it->second;

    // 先删除旧事件
    if (!updateEvents(fd, old_events, false)) {
        return false;
    }

    // 添加新事件
    if (!updateEvents(fd, events, true)) {
        // 如果失败，尝试恢复旧事件
        updateEvents(fd, old_events, true);
        return false;
    }

    fd_events_[fd] = events;
    LOG_DEBUG("Modified fd " << fd << " in kqueue with events " << events);
    return true;
}

bool KqueueEventLoop::removeFd(int fd) {
    auto it = fd_events_.find(fd);
    if (it == fd_events_.end()) {
        return true;  // 已经不存在
    }

    if (!updateEvents(fd, it->second, false)) {
        return false;
    }

    fd_events_.erase(it);
    LOG_DEBUG("Removed fd " << fd << " from kqueue");
    return true;
}

int KqueueEventLoop::wait(int timeout) {
    struct timespec ts;
    struct timespec* ts_ptr = nullptr;

    if (timeout >= 0) {
        ts.tv_sec = timeout / 1000;
        ts.tv_nsec = (timeout % 1000) * 1000000;
        ts_ptr = &ts;
    }

    ready_count_ = ::kevent(kqueue_fd_, nullptr, 0,
                            events_.data(), max_events_, ts_ptr);

    if (ready_count_ < 0) {
        if (errno == EINTR) {
            return 0;  // 被信号中断，不是错误
        }
        LOG_ERROR("kevent failed: " << strerror(errno));
        return -1;
    }

    return ready_count_;
}

int KqueueEventLoop::getReadyFd(int index) const {
    if (index < 0 || index >= ready_count_) {
        return -1;
    }
    return static_cast<int>(events_[index].ident);
}

uint32_t KqueueEventLoop::getReadyEvents(int index) const {
    if (index < 0 || index >= ready_count_) {
        return 0;
    }

    uint32_t events = 0;
    const struct kevent& kev = events_[index];

    if (kev.filter == EVFILT_READ) {
        events |= static_cast<uint32_t>(EventType::READ);
    }
    if (kev.filter == EVFILT_WRITE) {
        events |= static_cast<uint32_t>(EventType::WRITE);
    }
    if (kev.flags & EV_ERROR) {
        events |= static_cast<uint32_t>(EventType::ERROR);
    }
    if (kev.flags & EV_EOF) {
        events |= static_cast<uint32_t>(EventType::CLOSE);
    }

    return events;
}

void KqueueEventLoop::close() {
    if (kqueue_fd_ >= 0) {
        ::close(kqueue_fd_);
        kqueue_fd_ = -1;
        fd_events_.clear();
        LOG_DEBUG("Kqueue event loop closed");
    }
}

bool KqueueEventLoop::updateEvents(int fd, uint32_t events, bool enable) {
    struct kevent changes[2];
    int n_changes = 0;

    // READ事件
    if (events & static_cast<uint32_t>(EventType::READ)) {
        EV_SET(&changes[n_changes], fd, EVFILT_READ,
               enable ? EV_ADD | EV_CLEAR : EV_DELETE,
               0, 0, nullptr);
        n_changes++;
    }

    // WRITE事件
    if (events & static_cast<uint32_t>(EventType::WRITE)) {
        EV_SET(&changes[n_changes], fd, EVFILT_WRITE,
               enable ? EV_ADD | EV_CLEAR : EV_DELETE,
               0, 0, nullptr);
        n_changes++;
    }

    if (n_changes == 0) {
        return true;
    }

    // 应用更改
    if (::kevent(kqueue_fd_, changes, n_changes, nullptr, 0, nullptr) < 0) {
        LOG_ERROR("kevent update failed for fd " << fd << ": " << strerror(errno));
        return false;
    }

    return true;
}

} // namespace tiny_sql

#endif // __APPLE__ || BSD
