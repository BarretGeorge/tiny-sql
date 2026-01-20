#pragma once

#include "tiny_sql/network/event_loop.h"
#include <vector>

#ifdef __linux__

// 在命名空间外包含系统头文件
#include <sys/epoll.h>

namespace tiny_sql {

class EpollEventLoop : public EventLoop {
public:
    explicit EpollEventLoop(int max_events = 1024);
    ~EpollEventLoop() override;

    bool init() override;
    bool addFd(int fd, uint32_t events) override;
    bool modifyFd(int fd, uint32_t events) override;
    bool removeFd(int fd) override;
    int wait(int timeout = -1) override;
    int getReadyFd(int index) const override;
    uint32_t getReadyEvents(int index) const override;
    void close() override;

private:
    // 将我们的事件类型转换为epoll事件
    uint32_t toEpollEvents(uint32_t events) const;

    // 将epoll事件转换为我们的事件类型
    uint32_t fromEpollEvents(uint32_t events) const;

    int epoll_fd_;
    int max_events_;
    std::vector<::epoll_event> events_;  // 使用全局epoll_event
    int ready_count_;
};

} // namespace tiny_sql

#endif // __linux__
