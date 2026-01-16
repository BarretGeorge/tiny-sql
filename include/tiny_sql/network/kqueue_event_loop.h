#pragma once

#include "tiny_sql/network/event_loop.h"
#include <vector>
#include <unordered_map>

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)

namespace tiny_sql {

class KqueueEventLoop : public EventLoop {
public:
    explicit KqueueEventLoop(int max_events = 1024);
    ~KqueueEventLoop() override;

    bool init() override;
    bool addFd(int fd, uint32_t events) override;
    bool modifyFd(int fd, uint32_t events) override;
    bool removeFd(int fd) override;
    int wait(int timeout = -1) override;
    int getReadyFd(int index) const override;
    uint32_t getReadyEvents(int index) const override;
    void close() override;

private:
    // 更新kqueue事件
    bool updateEvents(int fd, uint32_t events, bool enable);

    int kqueue_fd_;
    int max_events_;
    std::vector<struct kevent> events_;
    int ready_count_;

    // 记录每个fd当前的事件状态
    std::unordered_map<int, uint32_t> fd_events_;
};

} // namespace tiny_sql

#endif // __APPLE__ || BSD
